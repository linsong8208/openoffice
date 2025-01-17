/**************************************************************
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *************************************************************/

#include "precompiled_vcl.hxx"

#include "vcl/throbber.hxx"
#include "vcl/svapp.hxx"

#include <com/sun/star/graphic/XGraphicProvider.hpp>
#include <com/sun/star/awt/ImageScaleMode.hpp>

#include <comphelper/componentcontext.hxx>
#include <comphelper/namedvaluecollection.hxx>
#include <comphelper/processfactory.hxx>
#include <rtl/ustrbuf.hxx>
#include <tools/diagnose_ex.h>
#include <tools/urlobj.hxx>

#include <limits>

using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::graphic::XGraphic;
using ::com::sun::star::graphic::XGraphicProvider;
using ::com::sun::star::uno::UNO_QUERY_THROW;
using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::uno::Exception;
namespace ImageScaleMode = ::com::sun::star::awt::ImageScaleMode;

//----------------------------------------------------------------------------------------------------------------------
Throbber::Throbber( Window* i_parentWindow, WinBits i_style, const ImageSet i_imageSet )
    :ImageControl( i_parentWindow, i_style )
    ,mbRepeat( sal_True )
    ,mnStepTime( 100 )
    ,mnCurStep( 0 )
    ,mnStepCount( 0 )
    ,meImageSet( i_imageSet )
{
    maWaitTimer.SetTimeout( mnStepTime );
    maWaitTimer.SetTimeoutHdl( LINK( this, Throbber, TimeOutHdl ) );

    SetScaleMode( ImageScaleMode::NONE );
    initImages();
}

//--------------------------------------------------------------------
Throbber::Throbber( Window* i_parentWindow, const ResId& i_resId, const ImageSet i_imageSet )
    :ImageControl( i_parentWindow, i_resId )
    ,mbRepeat( sal_True )
    ,mnStepTime( 100 )
    ,mnCurStep( 0 )
    ,mnStepCount( 0 )
    ,meImageSet( i_imageSet )
{
    maWaitTimer.SetTimeout( mnStepTime );
    maWaitTimer.SetTimeoutHdl( LINK( this, Throbber, TimeOutHdl ) );

    SetScaleMode( ImageScaleMode::NONE );
    initImages();
}

//----------------------------------------------------------------------------------------------------------------------
Throbber::~Throbber()
{
    maWaitTimer.Stop();
}

//----------------------------------------------------------------------------------------------------------------------
namespace
{
    ::std::vector< Image > lcl_loadImageSet( const Throbber::ImageSet i_imageSet, const bool i_isHiContrast )
    {
        ::std::vector< Image > aImages;
        ENSURE_OR_RETURN( i_imageSet != Throbber::IMAGES_NONE, "lcl_loadImageSet: illegal image set", aImages );

        const ::comphelper::ComponentContext aContext( ::comphelper::getProcessServiceFactory() );
        const Reference< XGraphicProvider > xGraphicProvider( aContext.createComponent( "com.sun.star.graphic.GraphicProvider" ), UNO_QUERY_THROW );

        ::std::vector< ::rtl::OUString > aImageURLs( Throbber::getDefaultImageURLs( i_imageSet ) );
        aImages.reserve( aImageURLs.size() );

        ::comphelper::NamedValueCollection aMediaProperties;
        for (   ::std::vector< ::rtl::OUString >::const_iterator imageURL = aImageURLs.begin();
                imageURL != aImageURLs.end();
                ++imageURL
            )
        {
            Reference< XGraphic > xGraphic;
            if ( i_isHiContrast )
            {
                INetURLObject aURL( *imageURL );
                if ( aURL.GetProtocol() != INET_PROT_PRIV_SOFFICE )
                {
                    const sal_Int32 separatorPos = imageURL->lastIndexOf( '/' );
                    if ( separatorPos != -1 )
                    {
                        ::rtl::OUStringBuffer composer;
                        composer.append( imageURL->copy( 0, separatorPos ) );
                        composer.appendAscii( RTL_CONSTASCII_STRINGPARAM( "/hicontrast" ) );
                        composer.append( imageURL->copy( separatorPos ) );

                        aMediaProperties.put( "URL", composer.makeStringAndClear() );
                        xGraphic.set( xGraphicProvider->queryGraphic( aMediaProperties.getPropertyValues() ), UNO_QUERY );
                    }
                }
            }
            if ( !xGraphic.is() )
            {
                aMediaProperties.put( "URL", *imageURL );
                xGraphic.set( xGraphicProvider->queryGraphic( aMediaProperties.getPropertyValues() ), UNO_QUERY );
            }
            aImages.push_back( Image( xGraphic ) );
        }

        return aImages;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void Throbber::Resize()
{
    ImageControl::Resize();

    if ( meImageSet == IMAGES_AUTO )
        initImages();
}

//----------------------------------------------------------------------------------------------------------------------
void Throbber::initImages()
{
    if ( meImageSet == IMAGES_NONE )
        return;

    try
    {
        ::std::vector< ::std::vector< Image > > aImageSets;
        const bool isHiContrast = GetSettings().GetStyleSettings().GetHighContrastMode();
        if ( meImageSet == IMAGES_AUTO )
        {
            aImageSets.push_back( lcl_loadImageSet( IMAGES_16_PX, isHiContrast ) );
            aImageSets.push_back( lcl_loadImageSet( IMAGES_32_PX, isHiContrast ) );
            aImageSets.push_back( lcl_loadImageSet( IMAGES_64_PX, isHiContrast ) );
        }
        else
        {
            aImageSets.push_back( lcl_loadImageSet( meImageSet, isHiContrast ) );
        }

        // find the best matching image set (size-wise)
        const ::Size aWindowSizePixel = GetSizePixel();
        size_t nPreferredSet = 0;
        if ( aImageSets.size() > 1 )
        {
            long nMinimalDistance = ::std::numeric_limits< long >::max();
            for (   ::std::vector< ::std::vector< Image > >::const_iterator check = aImageSets.begin();
                    check != aImageSets.end();
                    ++check
                )
            {
                ENSURE_OR_CONTINUE( !check->empty(), "Throbber::initImages: illegal image!" );
                const Size aImageSize = (*check)[0].GetSizePixel();

                if  (   ( aImageSize.Width() > aWindowSizePixel.Width() )
                    ||  ( aImageSize.Height() > aWindowSizePixel.Height() )
                    )
                    // do not use an image set which doesn't fit into the window
                    continue;

                const sal_Int64 distance =
                        ( aWindowSizePixel.Width() - aImageSize.Width() ) * ( aWindowSizePixel.Width() - aImageSize.Width() )
                    +   ( aWindowSizePixel.Height() - aImageSize.Height() ) * ( aWindowSizePixel.Height() - aImageSize.Height() );
                if ( distance < nMinimalDistance )
                {
                    nMinimalDistance = distance;
                    nPreferredSet = check - aImageSets.begin();
                }
            }
        }

        if ( nPreferredSet < aImageSets.size() )
            setImageList( aImageSets[nPreferredSet] );
    }
    catch( const Exception& )
    {
    	DBG_UNHANDLED_EXCEPTION();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void Throbber::start()
{
    maWaitTimer.Start();
}

//----------------------------------------------------------------------------------------------------------------------
void Throbber::stop()
{
    maWaitTimer.Stop();
}

//----------------------------------------------------------------------------------------------------------------------
bool Throbber::isRunning() const
{
    return maWaitTimer.IsActive();
}

//----------------------------------------------------------------------------------------------------------------------
void Throbber::setImageList( ::std::vector< Image > const& i_images )
{
    maImageList = i_images;

    mnStepCount = maImageList.size();
    const Image aInitialImage( mnStepCount ? maImageList[ 0 ] : Image() );
    SetImage( aInitialImage );
}

//----------------------------------------------------------------------------------------------------------------------
void Throbber::setImageList( const Sequence< Reference< XGraphic > >& rImageList )
{
    ::std::vector< Image > aImages( rImageList.getLength() );
    ::std::copy(
        rImageList.getConstArray(),
        rImageList.getConstArray() + rImageList.getLength(),
        aImages.begin()
    );
    setImageList( aImages );
}

//----------------------------------------------------------------------------------------------------------------------
::std::vector< ::rtl::OUString > Throbber::getDefaultImageURLs( const ImageSet i_imageSet )
{
    ::std::vector< ::rtl::OUString > aImageURLs;

    sal_Char const* const pResolutions[] = { "16", "32", "64" };
    size_t const nImageCounts[] = { 12, 12, 12 };

    size_t index = 0;
    switch ( i_imageSet )
    {
    case IMAGES_16_PX:  index = 0;  break;
    case IMAGES_32_PX:  index = 1;  break;
    case IMAGES_64_PX:  index = 2;  break;
    case IMAGES_NONE:
    case IMAGES_AUTO:
        OSL_ENSURE( false, "Throbber::getDefaultImageURLs: illegal image set!" );
        return aImageURLs;
    }

    aImageURLs.reserve( nImageCounts[index] );
    for ( size_t i=0; i<nImageCounts[index]; ++i )
    {
        ::rtl::OUStringBuffer aURL;
        aURL.appendAscii( "private:graphicrepository/vcl/res/spinner-" );
        aURL.appendAscii( pResolutions[index] );
        aURL.appendAscii( "-" );
        if ( i < 9 )
            aURL.appendAscii( "0" );
        aURL.append     ( sal_Int32( i + 1 ) );
        aURL.appendAscii( ".png" );

        aImageURLs.push_back( aURL.makeStringAndClear() );
    }

    return aImageURLs;
}

//----------------------------------------------------------------------------------------------------------------------
IMPL_LINK( Throbber, TimeOutHdl, void*, EMPTYARG )
{
    ::vos::OGuard aGuard( Application::GetSolarMutex() );
    if ( maImageList.empty() )
        return 0;

    if ( mnCurStep < mnStepCount - 1 )
        mnCurStep += 1;
    else
    {
        if ( mbRepeat )
        {
            // start over
            mnCurStep = 0;
        }
        else
        {
            stop();
        }
    }

	SetImage( maImageList[ mnCurStep ] );

	return 0;
}

/* vim: set noet sw=4 ts=4: */
