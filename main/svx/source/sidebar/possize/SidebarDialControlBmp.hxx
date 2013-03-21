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

#ifndef SVX_SIDEBAR_POSSIZE_DIAL_CONTROL_BMP_HXX
#define SVX_SIDEBAR_POSSIZE_DIAL_CONTROL_BMP_HXX

#include <svx/dialcontrol.hxx>
/*
    
#include <sfx2/sidebar/propertypanel.hrc>
#include <sfx2/sidebar/Theme.hxx>
#include <sfx2/sidebar/ControlFactory.hxx>
#include <TransformationPropertyPanel.hxx>
#include <TransformationPropertyPanel.hrc>
#include <svx/dialogs.hrc>
#include <svx/dialmgr.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/objsh.hxx>
#include <svx/dlgutil.hxx>
#include <unotools/viewoptions.hxx>
#include <vcl/virdev.hxx>
#include <vcl/svapp.hxx>
#include <vcl/field.hxx>
#include <vcl/fixed.hxx>
#include <vcl/toolbox.hxx>
#include <svx/svdview.hxx>
#include <svl/aeitem.hxx>

*/

namespace svx { namespace sidebar {


class SidebarDialControlBmp
    : public svx::DialControlBmp
{
public:
    explicit SidebarDialControlBmp( Window& rParent);
    virtual ~SidebarDialControlBmp (void);

    virtual void DrawElements( const String& rText, sal_Int32 nAngle );
    virtual void DrawBackground();

private:
};

} } // end of namespace svx::sidebar

#endif
