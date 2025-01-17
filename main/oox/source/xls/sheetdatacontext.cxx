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



#include "oox/xls/sheetdatacontext.hxx"

#include <com/sun/star/table/CellContentType.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/util/DateTime.hpp>
#include "oox/helper/attributelist.hxx"
#include "oox/helper/datetimehelper.hxx"
#include "oox/helper/propertyset.hxx"
#include "oox/xls/addressconverter.hxx"
#include "oox/xls/biffinputstream.hxx"
#include "oox/xls/formulaparser.hxx"
#include "oox/xls/richstringcontext.hxx"
#include "oox/xls/unitconverter.hxx"

namespace oox {
namespace xls {

// ============================================================================

using namespace ::com::sun::star::sheet;
using namespace ::com::sun::star::table;
using namespace ::com::sun::star::text;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;

using ::oox::core::ContextHandlerRef;
using ::rtl::OUString;

// ============================================================================

namespace {

// record constants -----------------------------------------------------------

const sal_uInt32 BIFF12_CELL_SHOWPHONETIC   = 0x01000000;

const sal_uInt8 BIFF12_DATATABLE_ROW        = 0x01;
const sal_uInt8 BIFF12_DATATABLE_2D         = 0x02;
const sal_uInt8 BIFF12_DATATABLE_REF1DEL    = 0x04;
const sal_uInt8 BIFF12_DATATABLE_REF2DEL    = 0x08;

const sal_uInt16 BIFF12_ROW_THICKTOP        = 0x0001;
const sal_uInt16 BIFF12_ROW_THICKBOTTOM     = 0x0002;
const sal_uInt16 BIFF12_ROW_COLLAPSED       = 0x0800;
const sal_uInt16 BIFF12_ROW_HIDDEN          = 0x1000;
const sal_uInt16 BIFF12_ROW_CUSTOMHEIGHT    = 0x2000;
const sal_uInt16 BIFF12_ROW_CUSTOMFORMAT    = 0x4000;
const sal_uInt8 BIFF12_ROW_SHOWPHONETIC     = 0x01;

const sal_uInt16 BIFF_DATATABLE_ROW         = 0x0004;
const sal_uInt16 BIFF_DATATABLE_2D          = 0x0008;
const sal_uInt16 BIFF_DATATABLE_REF1DEL     = 0x0010;
const sal_uInt16 BIFF_DATATABLE_REF2DEL     = 0x0020;

const sal_uInt8 BIFF_FORMULA_RES_STRING     = 0;        /// Result is a string.
const sal_uInt8 BIFF_FORMULA_RES_BOOL       = 1;        /// Result is Boolean value.
const sal_uInt8 BIFF_FORMULA_RES_ERROR      = 2;        /// Result is error code.
const sal_uInt8 BIFF_FORMULA_RES_EMPTY      = 3;        /// Result is empty cell (BIFF8 only).
const sal_uInt16 BIFF_FORMULA_SHARED        = 0x0008;   /// Shared formula cell.

const sal_uInt8 BIFF2_ROW_CUSTOMFORMAT      = 0x01;
const sal_uInt16 BIFF_ROW_DEFAULTHEIGHT     = 0x8000;
const sal_uInt16 BIFF_ROW_HEIGHTMASK        = 0x7FFF;
const sal_uInt32 BIFF_ROW_COLLAPSED         = 0x00000010;
const sal_uInt32 BIFF_ROW_HIDDEN            = 0x00000020;
const sal_uInt32 BIFF_ROW_CUSTOMHEIGHT      = 0x00000040;
const sal_uInt32 BIFF_ROW_CUSTOMFORMAT      = 0x00000080;
const sal_uInt32 BIFF_ROW_THICKTOP          = 0x10000000;
const sal_uInt32 BIFF_ROW_THICKBOTTOM       = 0x20000000;
const sal_uInt32 BIFF_ROW_SHOWPHONETIC      = 0x40000000;

const sal_Int32 BIFF2_CELL_USEIXFE          = 63;

} // namespace

// ============================================================================

SheetDataContextBase::SheetDataContextBase( const WorksheetHelper& rHelper ) :
    mrAddressConv( rHelper.getAddressConverter() ),
    mrFormulaParser( rHelper.getFormulaParser() ),
    mrSheetData( rHelper.getSheetData() ),
    mnSheet( rHelper.getSheetIndex() )
{
    maLastCellAddress.Sheet = rHelper.getSheetIndex();
    maLastCellAddress.Row = SAL_MAX_UINT32; // wraps around to 0 when incremented
}

SheetDataContextBase::~SheetDataContextBase()
{
}

// ============================================================================

SheetDataContext::SheetDataContext( WorksheetFragmentBase& rFragment ) :
    WorksheetContextBase( rFragment ),
    SheetDataContextBase( rFragment ),
    mbHasFormula( false ),
    mbValidRange( false )
{
}

ContextHandlerRef SheetDataContext::onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs )
{
    switch( getCurrentElement() )
    {
        case XLS_TOKEN( sheetData ):
            if( nElement == XLS_TOKEN( row ) ) { importRow( rAttribs ); return this; }
        break;

        case XLS_TOKEN( row ):
            // do not process cell elements with invalid (out-of-range) address
            if( nElement == XLS_TOKEN( c ) && importCell( rAttribs ) )
                return this;
        break;

        case XLS_TOKEN( c ):
            switch( nElement )
            {
                case XLS_TOKEN( is ):
                    mxInlineStr.reset( new RichString( *this ) );
                    return new RichStringContext( *this, mxInlineStr );
                case XLS_TOKEN( v ):
                    return this;    // characters contain cell value
                case XLS_TOKEN( f ):
                    importFormula( rAttribs );
                    return this;    // characters contain formula string
            }
        break;
    }
    return 0;
}

void SheetDataContext::onCharacters( const OUString& rChars )
{
    switch( getCurrentElement() )
    {
        case XLS_TOKEN( v ):
            maCellValue = rChars;
        break;
        case XLS_TOKEN( f ):
            if( maFmlaData.mnFormulaType != XML_TOKEN_INVALID )
                maTokens = mrFormulaParser.importFormula( maCellData.maCellAddr, rChars );
        break;
    }
}

void SheetDataContext::onEndElement()
{
    if( getCurrentElement() == XLS_TOKEN( c ) )
    {
        // try to create a formula cell
        if( mbHasFormula ) switch( maFmlaData.mnFormulaType )
        {
            case XML_normal:
                mrSheetData.setFormulaCell( maCellData, maTokens );
            break;
            case XML_shared:
                if( maFmlaData.mnSharedId >= 0 )
                {
                    if( mbValidRange && maFmlaData.isValidSharedRef( maCellData.maCellAddr ) )
                        mrSheetData.createSharedFormula( maFmlaData.mnSharedId, maTokens );
                    mrSheetData.setFormulaCell( maCellData, maFmlaData.mnSharedId );
                }
                else
                    // no success, set plain cell value and formatting below
                    mbHasFormula = false;
            break;
            case XML_array:
                if( mbValidRange && maFmlaData.isValidArrayRef( maCellData.maCellAddr ) )
                    mrSheetData.createArrayFormula( maFmlaData.maFormulaRef, maTokens );
                // set cell formatting, but do not set result as cell value
                mrSheetData.setBlankCell( maCellData );
            break;
            case XML_dataTable:
                if( mbValidRange )
                    mrSheetData.createTableOperation( maFmlaData.maFormulaRef, maTableData );
                // set cell formatting, but do not set result as cell value
                mrSheetData.setBlankCell( maCellData );
            break;
            default:
                OSL_ENSURE( maFmlaData.mnFormulaType == XML_TOKEN_INVALID, "SheetDataContext::onEndElement - unknown formula type" );
                mbHasFormula = false;
        }

        if( !mbHasFormula )
        {
            // no formula created: try to set the cell value
            if( maCellValue.getLength() > 0 ) switch( maCellData.mnCellType )
            {
                case XML_n:
                    mrSheetData.setValueCell( maCellData, maCellValue.toDouble() );
                break;
                case XML_d:
                {
                    DateTime dateTime;
                    if ( parseISO8601DateTime( maCellValue, dateTime ) )
                        mrSheetData.setDateTimeCell( maCellData, dateTime );
                    else
                        mrSheetData.setErrorCell( maCellData, maCellValue );
                }
                break;
                case XML_b:
                    mrSheetData.setBooleanCell( maCellData, maCellValue.toDouble() != 0.0 );
                break;
                case XML_e:
                    mrSheetData.setErrorCell( maCellData, maCellValue );
                break;
                case XML_str:
                    mrSheetData.setStringCell( maCellData, maCellValue );
                break;
                case XML_s:
                    mrSheetData.setStringCell( maCellData, maCellValue.toInt32() );
                break;
            }
            else if( (maCellData.mnCellType == XML_inlineStr) && mxInlineStr.get() )
            {
                mxInlineStr->finalizeImport();
                mrSheetData.setStringCell( maCellData, mxInlineStr );
            }
            else
            {
                // empty cell, update cell type
                maCellData.mnCellType = XML_TOKEN_INVALID;
                mrSheetData.setBlankCell( maCellData );
            }
        }
    }
}

ContextHandlerRef SheetDataContext::onCreateRecordContext( sal_Int32 nRecId, SequenceInputStream& rStrm )
{
    switch( getCurrentElement() )
    {
        case BIFF12_ID_SHEETDATA:
            if( nRecId == BIFF12_ID_ROW ) { importRow( rStrm ); return this; }
        break;

        case BIFF12_ID_ROW:
            switch( nRecId )
            {
                case BIFF12_ID_ARRAY:           importArray( rStrm );                           break;
                case BIFF12_ID_CELL_BOOL:       importCellBool( rStrm, CELLTYPE_VALUE );        break;
                case BIFF12_ID_CELL_BLANK:      importCellBlank( rStrm, CELLTYPE_VALUE );       break;
                case BIFF12_ID_CELL_DOUBLE:     importCellDouble( rStrm, CELLTYPE_VALUE );      break;
                case BIFF12_ID_CELL_ERROR:      importCellError( rStrm, CELLTYPE_VALUE );       break;
                case BIFF12_ID_CELL_RK:         importCellRk( rStrm, CELLTYPE_VALUE );          break;
                case BIFF12_ID_CELL_RSTRING:    importCellRString( rStrm, CELLTYPE_VALUE );     break;
                case BIFF12_ID_CELL_SI:         importCellSi( rStrm, CELLTYPE_VALUE );          break;
                case BIFF12_ID_CELL_STRING:     importCellString( rStrm, CELLTYPE_VALUE );      break;
                case BIFF12_ID_DATATABLE:       importDataTable( rStrm );                       break;
                case BIFF12_ID_FORMULA_BOOL:    importCellBool( rStrm, CELLTYPE_FORMULA );      break;
                case BIFF12_ID_FORMULA_DOUBLE:  importCellDouble( rStrm, CELLTYPE_FORMULA );    break;
                case BIFF12_ID_FORMULA_ERROR:   importCellError( rStrm, CELLTYPE_FORMULA );     break;
                case BIFF12_ID_FORMULA_STRING:  importCellString( rStrm, CELLTYPE_FORMULA );    break;
                case BIFF12_ID_MULTCELL_BOOL:   importCellBool( rStrm, CELLTYPE_MULTI );        break;
                case BIFF12_ID_MULTCELL_BLANK:  importCellBlank( rStrm, CELLTYPE_MULTI );       break;
                case BIFF12_ID_MULTCELL_DOUBLE: importCellDouble( rStrm, CELLTYPE_MULTI );      break;
                case BIFF12_ID_MULTCELL_ERROR:  importCellError( rStrm, CELLTYPE_MULTI );       break;
                case BIFF12_ID_MULTCELL_RK:     importCellRk( rStrm, CELLTYPE_MULTI );          break;
                case BIFF12_ID_MULTCELL_RSTRING:importCellRString( rStrm, CELLTYPE_MULTI );     break;
                case BIFF12_ID_MULTCELL_SI:     importCellSi( rStrm, CELLTYPE_MULTI );          break;
                case BIFF12_ID_MULTCELL_STRING: importCellString( rStrm, CELLTYPE_MULTI );      break;
                case BIFF12_ID_SHAREDFMLA:      importSharedFmla( rStrm );                      break;
            }
        break;
    }
    return 0;
}

// private --------------------------------------------------------------------

void SheetDataContext::importRow( const AttributeList& rAttribs )
{
    RowModel aModel;
    aModel.mnRow          = rAttribs.getInteger( XML_r, -1 );
    if ( aModel.mnRow == -1 )
        aModel.mnRow = ++maLastCellAddress.Row;
    else
        maLastCellAddress.Row = aModel.mnRow - 1;
    maLastCellAddress.Column = SAL_MAX_UINT32; // wraps around to 0 when incremented
    aModel.mfHeight       = rAttribs.getDouble( XML_ht, -1.0 );
    aModel.mnXfId         = rAttribs.getInteger( XML_s, -1 );
    aModel.mnLevel        = rAttribs.getInteger( XML_outlineLevel, 0 );
    aModel.mbCustomHeight = rAttribs.getBool( XML_customHeight, false );
    aModel.mbCustomFormat = rAttribs.getBool( XML_customFormat, false );
    aModel.mbShowPhonetic = rAttribs.getBool( XML_ph, false );
    aModel.mbHidden       = rAttribs.getBool( XML_hidden, false );
    aModel.mbCollapsed    = rAttribs.getBool( XML_collapsed, false );
    aModel.mbThickTop     = rAttribs.getBool( XML_thickTop, false );
    aModel.mbThickBottom  = rAttribs.getBool( XML_thickBot, false );

    // decode the column spans (space-separated list of colon-separated integer pairs)
    OUString aColSpansText = rAttribs.getString( XML_spans, OUString() );
    sal_Int32 nMaxCol = mrAddressConv.getMaxApiAddress().Column;
    sal_Int32 nIndex = 0;
    while( nIndex >= 0 )
    {
        OUString aColSpanToken = aColSpansText.getToken( 0, ' ', nIndex );
        sal_Int32 nSepPos = aColSpanToken.indexOf( ':' );
        if( (0 < nSepPos) && (nSepPos + 1 < aColSpanToken.getLength()) )
        {
            // OOXML uses 1-based integer column indexes, row model expects 0-based colspans
            sal_Int32 nLastCol = ::std::min( aColSpanToken.copy( nSepPos + 1 ).toInt32() - 1, nMaxCol );
            aModel.insertColSpan( ValueRange( aColSpanToken.copy( 0, nSepPos ).toInt32() - 1, nLastCol ) );
        }
    }

    // set row properties in the current sheet
    setRowModel( aModel );
}

bool SheetDataContext::importCell( const AttributeList& rAttribs )
{
    OUString r = rAttribs.getString( XML_r, OUString() );
    bool bValidAddr;
    if ( r.getLength() > 0 )
        bValidAddr = mrAddressConv.convertToCellAddress( maCellData.maCellAddr, rAttribs.getString( XML_r, OUString() ), mnSheet, true );
    else
    {
        maCellData.maCellAddr.Column = ++maLastCellAddress.Column;
        maCellData.maCellAddr.Row = maLastCellAddress.Row;
        maCellData.maCellAddr.Sheet = maLastCellAddress.Sheet;
        bValidAddr = true;
    }
    if( bValidAddr )
    {
        maLastCellAddress.Column  = maCellData.maCellAddr.Column;
        maLastCellAddress.Row     = maCellData.maCellAddr.Row;
        maLastCellAddress.Sheet   = maCellData.maCellAddr.Sheet;
        maCellData.mnCellType     = rAttribs.getToken( XML_t, XML_n );
        maCellData.mnXfId         = rAttribs.getInteger( XML_s, -1 );
        maCellData.mbShowPhonetic = rAttribs.getBool( XML_ph, false );

        // reset cell value, formula settings, and inline string
        maCellValue = OUString();
        mxInlineStr.reset();
        mbHasFormula = false;

        // update used area of the sheet
        extendUsedArea( maCellData.maCellAddr );
    }
    return bValidAddr;
}

void SheetDataContext::importFormula( const AttributeList& rAttribs )
{
    mbHasFormula = true;
    mbValidRange = mrAddressConv.convertToCellRange( maFmlaData.maFormulaRef, rAttribs.getString( XML_ref, OUString() ), mnSheet, true, true );
    
    maFmlaData.mnFormulaType = rAttribs.getToken( XML_t, XML_normal );
    maFmlaData.mnSharedId    = rAttribs.getInteger( XML_si, -1 );

    if( maFmlaData.mnFormulaType == XML_dataTable )
    {
        maTableData.maRef1        = rAttribs.getString( XML_r1, OUString() );
        maTableData.maRef2        = rAttribs.getString( XML_r2, OUString() );
        maTableData.mb2dTable     = rAttribs.getBool( XML_dt2D, false );
        maTableData.mbRowTable    = rAttribs.getBool( XML_dtr, false );
        maTableData.mbRef1Deleted = rAttribs.getBool( XML_del1, false );
        maTableData.mbRef2Deleted = rAttribs.getBool( XML_del2, false );
    }

    // clear token array, will be regenerated from element text
    maTokens = ApiTokenSequence();
}

void SheetDataContext::importRow( SequenceInputStream& rStrm )
{
    RowModel aModel;
    sal_Int32 nSpanCount;
    sal_uInt16 nHeight, nFlags1;
    sal_uInt8 nFlags2;
    rStrm >> maCurrPos.mnRow >> aModel.mnXfId >> nHeight >> nFlags1 >> nFlags2 >> nSpanCount;
    maCurrPos.mnCol = 0;

    // row index is 0-based in BIFF12, but RowModel expects 1-based
    aModel.mnRow          = maCurrPos.mnRow + 1;
    // row height is in twips in BIFF12, convert to points
    aModel.mfHeight       = nHeight / 20.0;
    aModel.mnLevel        = extractValue< sal_Int32 >( nFlags1, 8, 3 );
    aModel.mbCustomHeight = getFlag( nFlags1, BIFF12_ROW_CUSTOMHEIGHT );
    aModel.mbCustomFormat = getFlag( nFlags1, BIFF12_ROW_CUSTOMFORMAT );
    aModel.mbShowPhonetic = getFlag( nFlags2, BIFF12_ROW_SHOWPHONETIC );
    aModel.mbHidden       = getFlag( nFlags1, BIFF12_ROW_HIDDEN );
    aModel.mbCollapsed    = getFlag( nFlags1, BIFF12_ROW_COLLAPSED );
    aModel.mbThickTop     = getFlag( nFlags1, BIFF12_ROW_THICKTOP );
    aModel.mbThickBottom  = getFlag( nFlags1, BIFF12_ROW_THICKBOTTOM );

    // read the column spans
    sal_Int32 nMaxCol = mrAddressConv.getMaxApiAddress().Column;
    for( sal_Int32 nSpanIdx = 0; (nSpanIdx < nSpanCount) && !rStrm.isEof(); ++nSpanIdx )
    {
        sal_Int32 nFirstCol, nLastCol;
        rStrm >> nFirstCol >> nLastCol;
        aModel.insertColSpan( ValueRange( nFirstCol, ::std::min( nLastCol, nMaxCol ) ) );
    }

    // set row properties in the current sheet
    setRowModel( aModel );
}

bool SheetDataContext::readCellHeader( SequenceInputStream& rStrm, CellType eCellType )
{
    switch( eCellType )
    {
        case CELLTYPE_VALUE:
        case CELLTYPE_FORMULA:  rStrm >> maCurrPos.mnCol;   break;
        case CELLTYPE_MULTI:    ++maCurrPos.mnCol;          break;
    }

    sal_uInt32 nXfId;
    rStrm >> nXfId;

    bool bValidAddr = mrAddressConv.convertToCellAddress( maCellData.maCellAddr, maCurrPos, mnSheet, true );
    maCellData.mnXfId = extractValue< sal_Int32 >( nXfId, 0, 24 );
    maCellData.mbShowPhonetic = getFlag( nXfId, BIFF12_CELL_SHOWPHONETIC );

    // update used area of the sheet
    if( bValidAddr )
        extendUsedArea( maCellData.maCellAddr );
    return bValidAddr;
}

ApiTokenSequence SheetDataContext::readCellFormula( SequenceInputStream& rStrm )
{
    rStrm.skip( 2 );
    return mrFormulaParser.importFormula( maCellData.maCellAddr, FORMULATYPE_CELL, rStrm );
}

bool SheetDataContext::readFormulaRef( SequenceInputStream& rStrm )
{
    BinRange aRange;
    rStrm >> aRange;
    return mrAddressConv.convertToCellRange( maFmlaData.maFormulaRef, aRange, mnSheet, true, true );
}

void SheetDataContext::importCellBool( SequenceInputStream& rStrm, CellType eCellType )
{
    if( readCellHeader( rStrm, eCellType ) )
    {
        maCellData.mnCellType = XML_b;
        bool bValue = rStrm.readuInt8() != 0;
        if( eCellType == CELLTYPE_FORMULA )
            mrSheetData.setFormulaCell( maCellData, readCellFormula( rStrm ) );
        else
            mrSheetData.setBooleanCell( maCellData, bValue );
    }
}

void SheetDataContext::importCellBlank( SequenceInputStream& rStrm, CellType eCellType )
{
    OSL_ENSURE( eCellType != CELLTYPE_FORMULA, "SheetDataContext::importCellBlank - no formula cells supported" );
    if( readCellHeader( rStrm, eCellType ) )
        mrSheetData.setBlankCell( maCellData );
}

void SheetDataContext::importCellDouble( SequenceInputStream& rStrm, CellType eCellType )
{
    if( readCellHeader( rStrm, eCellType ) )
    {
        maCellData.mnCellType = XML_n;
        double fValue = rStrm.readDouble();
        if( eCellType == CELLTYPE_FORMULA )
            mrSheetData.setFormulaCell( maCellData, readCellFormula( rStrm ) );
        else
            mrSheetData.setValueCell( maCellData, fValue );
    }
}

void SheetDataContext::importCellError( SequenceInputStream& rStrm, CellType eCellType )
{
    if( readCellHeader( rStrm, eCellType ) )
    {
        maCellData.mnCellType = XML_e;
        sal_uInt8 nErrorCode = rStrm.readuInt8();
        if( eCellType == CELLTYPE_FORMULA )
            mrSheetData.setFormulaCell( maCellData, readCellFormula( rStrm ) );
        else
            mrSheetData.setErrorCell( maCellData, nErrorCode );
    }
}

void SheetDataContext::importCellRk( SequenceInputStream& rStrm, CellType eCellType )
{
    OSL_ENSURE( eCellType != CELLTYPE_FORMULA, "SheetDataContext::importCellRk - no formula cells supported" );
    if( readCellHeader( rStrm, eCellType ) )
    {
        maCellData.mnCellType = XML_n;
        mrSheetData.setValueCell( maCellData, BiffHelper::calcDoubleFromRk( rStrm.readInt32() ) );
    }
}

void SheetDataContext::importCellRString( SequenceInputStream& rStrm, CellType eCellType )
{
    OSL_ENSURE( eCellType != CELLTYPE_FORMULA, "SheetDataContext::importCellRString - no formula cells supported" );
    if( readCellHeader( rStrm, eCellType ) )
    {
        maCellData.mnCellType = XML_inlineStr;
        RichStringRef xString( new RichString( *this ) );
        xString->importString( rStrm, true );
        xString->finalizeImport();
        mrSheetData.setStringCell( maCellData, xString );
    }
}

void SheetDataContext::importCellSi( SequenceInputStream& rStrm, CellType eCellType )
{
    OSL_ENSURE( eCellType != CELLTYPE_FORMULA, "SheetDataContext::importCellSi - no formula cells supported" );
    if( readCellHeader( rStrm, eCellType ) )
    {
        maCellData.mnCellType = XML_s;
        mrSheetData.setStringCell( maCellData, rStrm.readInt32() );
    }
}

void SheetDataContext::importCellString( SequenceInputStream& rStrm, CellType eCellType )
{
    if( readCellHeader( rStrm, eCellType ) )
    {
        maCellData.mnCellType = XML_inlineStr;
        // always import the string, stream will point to formula afterwards, if existing
        RichStringRef xString( new RichString( *this ) );
        xString->importString( rStrm, false );
        xString->finalizeImport();
        if( eCellType == CELLTYPE_FORMULA )
            mrSheetData.setFormulaCell( maCellData, readCellFormula( rStrm ) );
        else
            mrSheetData.setStringCell( maCellData, xString );
    }
}

void SheetDataContext::importArray( SequenceInputStream& rStrm )
{
    if( readFormulaRef( rStrm ) && maFmlaData.isValidArrayRef( maCellData.maCellAddr ) )
    {
        rStrm.skip( 1 );
        ApiTokenSequence aTokens = mrFormulaParser.importFormula( maCellData.maCellAddr, FORMULATYPE_ARRAY, rStrm );
        mrSheetData.createArrayFormula( maFmlaData.maFormulaRef, aTokens );
    }
}

void SheetDataContext::importDataTable( SequenceInputStream& rStrm )
{
    if( readFormulaRef( rStrm ) )
    {
        BinAddress aRef1, aRef2;
        sal_uInt8 nFlags;
        rStrm >> aRef1 >> aRef2 >> nFlags;
        maTableData.maRef1        = FormulaProcessorBase::generateAddress2dString( aRef1, false );
        maTableData.maRef2        = FormulaProcessorBase::generateAddress2dString( aRef2, false );
        maTableData.mbRowTable    = getFlag( nFlags, BIFF12_DATATABLE_ROW );
        maTableData.mb2dTable     = getFlag( nFlags, BIFF12_DATATABLE_2D );
        maTableData.mbRef1Deleted = getFlag( nFlags, BIFF12_DATATABLE_REF1DEL );
        maTableData.mbRef2Deleted = getFlag( nFlags, BIFF12_DATATABLE_REF2DEL );
        mrSheetData.createTableOperation( maFmlaData.maFormulaRef, maTableData );
    }
}

void SheetDataContext::importSharedFmla( SequenceInputStream& rStrm )
{
    if( readFormulaRef( rStrm ) && maFmlaData.isValidSharedRef( maCellData.maCellAddr ) )
    {
        ApiTokenSequence aTokens = mrFormulaParser.importFormula( maCellData.maCellAddr, FORMULATYPE_SHAREDFORMULA, rStrm );
        mrSheetData.createSharedFormula( maCellData.maCellAddr, aTokens );
    }
}

// ============================================================================

BiffSheetDataContext::BiffSheetDataContext( const WorksheetHelper& rHelper ) :
    BiffWorksheetContextBase( rHelper ),
    SheetDataContextBase( rHelper ),
    mnBiff2XfId( 0 )
{
    switch( getBiff() )
    {
        case BIFF2:
            mnFormulaSkipSize = 9;  // double formula result, 1 byte flags
            mnArraySkipSize = 1;    // recalc-always flag
        break;
        case BIFF3:
        case BIFF4:
            mnFormulaSkipSize = 10; // double formula result, 2 byte flags
            mnArraySkipSize = 2;    // 2 byte flags
        break;
        case BIFF5:
        case BIFF8:
            mnFormulaSkipSize = 14; // double formula result, 2 byte flags, 4 bytes nothing
            mnArraySkipSize = 6;    // 2 byte flags, 4 bytes nothing
        break;
        case BIFF_UNKNOWN:
        break;
    }
}

void BiffSheetDataContext::importRecord( BiffInputStream& rStrm )
{
    sal_uInt16 nRecId = rStrm.getRecId();
    switch( nRecId )
    {
        // records in all BIFF versions
        case BIFF2_ID_ARRAY:        // #i72713#
        case BIFF3_ID_ARRAY:        importArray( rStrm );   break;
        case BIFF2_ID_BLANK:
        case BIFF3_ID_BLANK:        importBlank( rStrm );   break;
        case BIFF2_ID_BOOLERR:
        case BIFF3_ID_BOOLERR:      importBoolErr( rStrm ); break;
        case BIFF2_ID_INTEGER:      importInteger( rStrm ); break;
        case BIFF_ID_IXFE:          rStrm >> mnBiff2XfId;   break;
        case BIFF2_ID_LABEL:
        case BIFF3_ID_LABEL:        importLabel( rStrm );   break;
        case BIFF2_ID_NUMBER:
        case BIFF3_ID_NUMBER:       importNumber( rStrm );  break;
        case BIFF_ID_RK:            importRk( rStrm );      break;

        // BIFF specific records
        default: switch( getBiff() )
        {
            case BIFF2: switch( nRecId )
            {
                case BIFF2_ID_DATATABLE:    importDataTable( rStrm );   break;
                case BIFF2_ID_DATATABLE2:   importDataTable( rStrm );   break;
                case BIFF2_ID_FORMULA:      importFormula( rStrm );     break;
                case BIFF2_ID_ROW:          importRow( rStrm );         break;
            }
            break;

            case BIFF3: switch( nRecId )
            {
                case BIFF3_ID_DATATABLE:    importDataTable( rStrm );   break;
                case BIFF3_ID_FORMULA:      importFormula( rStrm );     break;
                case BIFF3_ID_ROW:          importRow( rStrm );         break;
            }
            break;

            case BIFF4: switch( nRecId )
            {
                case BIFF3_ID_DATATABLE:    importDataTable( rStrm );   break;
                case BIFF4_ID_FORMULA:      importFormula( rStrm );     break;
                case BIFF3_ID_ROW:          importRow( rStrm );         break;
            }
            break;

            case BIFF5: switch( nRecId )
            {
                case BIFF3_ID_DATATABLE:    importDataTable( rStrm );   break;
                case BIFF3_ID_FORMULA:
                case BIFF4_ID_FORMULA:
                case BIFF5_ID_FORMULA:      importFormula( rStrm );     break;
                case BIFF_ID_MULTBLANK:     importMultBlank( rStrm );   break;
                case BIFF_ID_MULTRK:        importMultRk( rStrm );      break;
                case BIFF3_ID_ROW:          importRow( rStrm );         break;
                case BIFF_ID_RSTRING:       importLabel( rStrm );       break;
                case BIFF_ID_SHAREDFMLA:    importSharedFmla( rStrm );  break;
            }
            break;

            case BIFF8: switch( nRecId )
            {
                case BIFF3_ID_DATATABLE:    importDataTable( rStrm );   break;
                case BIFF3_ID_FORMULA:
                case BIFF4_ID_FORMULA:
                case BIFF5_ID_FORMULA:      importFormula( rStrm );     break;
                case BIFF_ID_LABELSST:      importLabelSst( rStrm );    break;
                case BIFF_ID_MULTBLANK:     importMultBlank( rStrm );   break;
                case BIFF_ID_MULTRK:        importMultRk( rStrm );      break;
                case BIFF3_ID_ROW:          importRow( rStrm );         break;
                case BIFF_ID_RSTRING:       importLabel( rStrm );       break;
                case BIFF_ID_SHAREDFMLA:    importSharedFmla( rStrm );  break;
            }
            break;

            case BIFF_UNKNOWN:
            break;
        }
    }
}

// private --------------------------------------------------------------------

void BiffSheetDataContext::importRow( BiffInputStream& rStrm )
{
    RowModel aModel;
    sal_uInt16 nRow, nFirstUsedCol, nFirstFreeCol, nHeight;
    rStrm >> nRow >> nFirstUsedCol >> nFirstFreeCol >> nHeight;
    if( getBiff() == BIFF2 )
    {
        rStrm.skip( 2 );
        aModel.mbCustomFormat = rStrm.readuInt8() == BIFF2_ROW_CUSTOMFORMAT;
        if( aModel.mbCustomFormat )
        {
            rStrm.skip( 5 );
            aModel.mnXfId = rStrm.readuInt16();
        }
    }
    else
    {
        rStrm.skip( 4 );
        sal_uInt32 nFlags = rStrm.readuInt32();
        aModel.mnXfId         = extractValue< sal_Int32 >( nFlags, 16, 12 );
        aModel.mnLevel        = extractValue< sal_Int32 >( nFlags, 0, 3 );
        aModel.mbCustomFormat = getFlag( nFlags, BIFF_ROW_CUSTOMFORMAT );
        aModel.mbCustomHeight = getFlag( nFlags, BIFF_ROW_CUSTOMHEIGHT );
        aModel.mbShowPhonetic = getFlag( nFlags, BIFF_ROW_SHOWPHONETIC );
        aModel.mbHidden       = getFlag( nFlags, BIFF_ROW_HIDDEN );
        aModel.mbCollapsed    = getFlag( nFlags, BIFF_ROW_COLLAPSED );
        aModel.mbThickTop     = getFlag( nFlags, BIFF_ROW_THICKTOP );
        aModel.mbThickBottom  = getFlag( nFlags, BIFF_ROW_THICKBOTTOM );
    }

    // row index is 0-based in BIFF, but RowModel expects 1-based
    aModel.mnRow = static_cast< sal_Int32 >( nRow ) + 1;
    // row height is in twips in BIFF, convert to points
    aModel.mfHeight = (nHeight & BIFF_ROW_HEIGHTMASK) / 20.0;
    // set column spans
    if( nFirstUsedCol < nFirstFreeCol )
    {
        sal_Int32 nLastCol = ::std::min< sal_Int32 >( nFirstFreeCol - 1, mrAddressConv.getMaxApiAddress().Column );
        aModel.insertColSpan( ValueRange( nFirstUsedCol, nLastCol ) );
    }

    // set row properties in the current sheet
    setRowModel( aModel );
}

bool BiffSheetDataContext::readCellXfId( BiffInputStream& rStrm, const BinAddress& rAddr, bool bBiff2 )
{
    bool bValidAddr = mrAddressConv.convertToCellAddress( maCellData.maCellAddr, rAddr, mnSheet, true );
    if( bValidAddr )
    {
        // update used area of the sheet
        extendUsedArea( maCellData.maCellAddr );

        // load the XF identifier according to current BIFF version
        if( bBiff2 )
        {
            /*  #i71453# On first call, check if the file contains XF records
                (by trying to access the first XF with index 0). If there are
                no XFs, the explicit formatting information contained in each
                cell record will be used instead. */
            if( !mobBiff2HasXfs )
                mobBiff2HasXfs = getStyles().getCellXf( 0 ).get() != 0;
            // read formatting information (includes the XF identifier)
            sal_uInt8 nFlags1, nFlags2, nFlags3;
            rStrm >> nFlags1 >> nFlags2 >> nFlags3;
            /*  If the file contains XFs, extract and set the XF identifier,
                otherwise get the explicit formatting. */
            if( mobBiff2HasXfs.get() )
            {
                maCellData.mnXfId = extractValue< sal_Int32 >( nFlags1, 0, 6 );
                /*  If the identifier is equal to 63, then the real identifier
                    is contained in the preceding IXFE record (stored in the
                    class member mnBiff2XfId). */
                if( maCellData.mnXfId == BIFF2_CELL_USEIXFE )
                    maCellData.mnXfId = mnBiff2XfId;
            }
            else
            {
                /*  Let the Xf class do the API conversion. Keeping the member
                    maCellData.mnXfId untouched will prevent to trigger the
                    usual XF formatting conversion later on. */
                PropertySet aPropSet( getCell( maCellData.maCellAddr ) );
                Xf::writeBiff2CellFormatToPropertySet( *this, aPropSet, nFlags1, nFlags2, nFlags3 );
            }
        }
        else
        {
            // BIFF3-BIFF8: 16-bit XF identifier
            maCellData.mnXfId = rStrm.readuInt16();
        }
    }
    return bValidAddr;
}

bool BiffSheetDataContext::readCellHeader( BiffInputStream& rStrm, bool bBiff2 )
{
    BinAddress aAddr;
    rStrm >> aAddr;
    return readCellXfId( rStrm, aAddr, bBiff2 );
}

bool BiffSheetDataContext::readFormulaRef( BiffInputStream& rStrm )
{
    BinRange aRange;
    aRange.read( rStrm, false );    // columns always 8-bit
    return mrAddressConv.convertToCellRange( maFmlaData.maFormulaRef, aRange, mnSheet, true, true );
}

void BiffSheetDataContext::importBlank( BiffInputStream& rStrm )
{
    if( readCellHeader( rStrm, rStrm.getRecId() == BIFF2_ID_BLANK ) )
        mrSheetData.setBlankCell( maCellData );
}

void BiffSheetDataContext::importBoolErr( BiffInputStream& rStrm )
{
    if( readCellHeader( rStrm, rStrm.getRecId() == BIFF2_ID_BOOLERR ) )
    {
        sal_uInt8 nValue, nType;
        rStrm >> nValue >> nType;
        switch( nType )
        {
            case BIFF_BOOLERR_BOOL:
                maCellData.mnCellType = XML_b;
                mrSheetData.setBooleanCell( maCellData, nValue != 0 );
            break;
            case BIFF_BOOLERR_ERROR:
                maCellData.mnCellType = XML_e;
                mrSheetData.setErrorCell( maCellData, nValue );
            break;
            default:
                OSL_ENSURE( false, "BiffSheetDataContext::importBoolErr - unknown cell type" );
                maCellData.mnCellType = XML_TOKEN_INVALID;
                mrSheetData.setBlankCell( maCellData );
        }
    }
}

void BiffSheetDataContext::importFormula( BiffInputStream& rStrm )
{
    if( readCellHeader( rStrm, getBiff() == BIFF2 ) )
    {
        maCellData.mnCellType = XML_n;
        rStrm.skip( mnFormulaSkipSize );
        ApiTokenSequence aTokens = mrFormulaParser.importFormula( maCellData.maCellAddr, FORMULATYPE_CELL, rStrm );
        mrSheetData.setFormulaCell( maCellData, aTokens );
    }
}

void BiffSheetDataContext::importInteger( BiffInputStream& rStrm )
{
    if( readCellHeader( rStrm, true ) )
    {
        maCellData.mnCellType = XML_n;
        mrSheetData.setValueCell( maCellData, rStrm.readuInt16() );
    }
}

void BiffSheetDataContext::importLabel( BiffInputStream& rStrm )
{
    /*  the deep secrets of BIFF type and record identifier...
        record id   BIFF    ->  XF type     String type
        0x0004      2-7     ->  3 byte      8-bit length, byte string
        0x0004      8       ->  3 byte      16-bit length, unicode string
        0x0204      2-7     ->  2 byte      16-bit length, byte string
        0x0204      8       ->  2 byte      16-bit length, unicode string
     */
    bool bBiff2Xf = rStrm.getRecId() == BIFF2_ID_LABEL;
    if( readCellHeader( rStrm, bBiff2Xf ) )
    {
        maCellData.mnCellType = XML_inlineStr;
        if( getBiff() == BIFF8 )
        {
            // string may contain rich-text formatting
            RichStringRef xString( new RichString( *this ) );
            xString->importUniString( rStrm );
            xString->finalizeImport();
            mrSheetData.setStringCell( maCellData, xString );
        }
        else
        {
            // #i63105# use text encoding from FONT record
            rtl_TextEncoding eTextEnc = getTextEncoding();
            if( const Font* pFont = getStyles().getFontFromCellXf( maCellData.mnXfId ).get() )
                eTextEnc = pFont->getFontEncoding();
            // RSTRING record contains rich-text formatting
            if( rStrm.getRecId() == BIFF_ID_RSTRING )
            {
                BiffStringFlags nFlags = BIFF_STR_EXTRAFONTS;
                // BIFF2 record identifier: 8-bit string length (see above)
                setFlag( nFlags, BIFF_STR_8BITLENGTH, bBiff2Xf );
                RichStringRef xString( new RichString( *this ) );
                xString->importByteString( rStrm, eTextEnc, nFlags );
                xString->finalizeImport();
                mrSheetData.setStringCell( maCellData, xString );
            }
            else
            {
                // BIFF2 record identifier: 8-bit string length (see above)
                OUString aText = rStrm.readByteStringUC( !bBiff2Xf, eTextEnc );
                mrSheetData.setStringCell( maCellData, aText );
            }
        }
    }
}

void BiffSheetDataContext::importLabelSst( BiffInputStream& rStrm )
{
    if( readCellHeader( rStrm, false ) )
    {
        maCellData.mnCellType = XML_s;
        mrSheetData.setStringCell( maCellData, rStrm.readInt32() );
    }
}

void BiffSheetDataContext::importMultBlank( BiffInputStream& rStrm )
{
    BinAddress aAddr;
    bool bValidAddr = true;
    for( rStrm >> aAddr; bValidAddr && (rStrm.getRemaining() > 2); ++aAddr.mnCol )
        if( (bValidAddr = readCellXfId( rStrm, aAddr, false )) == true )
            mrSheetData.setBlankCell( maCellData );
}

void BiffSheetDataContext::importMultRk( BiffInputStream& rStrm )
{
    BinAddress aAddr;
    bool bValidAddr = true;
    for( rStrm >> aAddr; bValidAddr && (rStrm.getRemaining() > 2); ++aAddr.mnCol )
    {
        if( (bValidAddr = readCellXfId( rStrm, aAddr, false )) == true )
        {
            maCellData.mnCellType = XML_n;
            sal_Int32 nRkValue = rStrm.readInt32();
            mrSheetData.setValueCell( maCellData, BiffHelper::calcDoubleFromRk( nRkValue ) );
        }
    }
}

void BiffSheetDataContext::importNumber( BiffInputStream& rStrm )
{
    if( readCellHeader( rStrm, rStrm.getRecId() == BIFF2_ID_NUMBER ) )
    {
        maCellData.mnCellType = XML_n;
        mrSheetData.setValueCell( maCellData, rStrm.readDouble() );
    }
}

void BiffSheetDataContext::importRk( BiffInputStream& rStrm )
{
    if( readCellHeader( rStrm, false ) )
    {
        maCellData.mnCellType = XML_n;
        mrSheetData.setValueCell( maCellData, BiffHelper::calcDoubleFromRk( rStrm.readInt32() ) );
    }
}

void BiffSheetDataContext::importArray( BiffInputStream& rStrm )
{
    if( readFormulaRef( rStrm ) && maFmlaData.isValidArrayRef( maCellData.maCellAddr ) )
    {
        rStrm.skip( mnArraySkipSize );
        ApiTokenSequence aTokens = mrFormulaParser.importFormula( maCellData.maCellAddr, FORMULATYPE_ARRAY, rStrm );
        mrSheetData.createArrayFormula( maFmlaData.maFormulaRef, aTokens );
    }
}

void BiffSheetDataContext::importDataTable( BiffInputStream& rStrm )
{
    if( readFormulaRef( rStrm ) )
    {
        DataTableModel aModel;
        BinAddress aRef1, aRef2;
        switch( rStrm.getRecId() )
        {
            case BIFF2_ID_DATATABLE:
                rStrm.skip( 1 );
                aModel.mbRowTable = rStrm.readuInt8() != 0;
                aModel.mb2dTable = false;
                rStrm >> aRef1;
            break;
            case BIFF2_ID_DATATABLE2:
                rStrm.skip( 2 );
                aModel.mb2dTable = true;
                rStrm >> aRef1 >> aRef2;
            break;
            case BIFF3_ID_DATATABLE:
            {
                sal_uInt16 nFlags;
                rStrm >> nFlags >> aRef1 >> aRef2;
                aModel.mbRowTable = getFlag( nFlags, BIFF_DATATABLE_ROW );
                aModel.mb2dTable = getFlag( nFlags, BIFF_DATATABLE_2D );
                aModel.mbRef1Deleted = getFlag( nFlags, BIFF_DATATABLE_REF1DEL );
                aModel.mbRef2Deleted = getFlag( nFlags, BIFF_DATATABLE_REF2DEL );
            }
            break;
            default:
                OSL_ENSURE( false, "BiffSheetDataContext::importDataTable - unknown record id" );
        }
        aModel.maRef1 = FormulaProcessorBase::generateAddress2dString( aRef1, false );
        aModel.maRef2 = FormulaProcessorBase::generateAddress2dString( aRef2, false );
        mrSheetData.createTableOperation( maFmlaData.maFormulaRef, aModel );
    }
}

void BiffSheetDataContext::importSharedFmla( BiffInputStream& rStrm )
{
    if( readFormulaRef( rStrm ) && maFmlaData.isValidSharedRef( maCellData.maCellAddr ) )
    {
        rStrm.skip( 2 );    // flags
        ApiTokenSequence aTokens = mrFormulaParser.importFormula( maCellData.maCellAddr, FORMULATYPE_SHAREDFORMULA, rStrm );
        mrSheetData.createSharedFormula( maCellData.maCellAddr, aTokens );
    }
}

// ============================================================================

} // namespace xls
} // namespace oox
