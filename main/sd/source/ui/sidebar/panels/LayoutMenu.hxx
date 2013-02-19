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



#ifndef SD_SIDEBAR_LAYOUT_MENU_HXX
#define SD_SIDEBAR_LAYOUT_MENU_HXX

#include "../IDisposable.hxx"
#include "../ILayoutableWindow.hxx"
#include "../ISidebarReceiver.hxx"

#include "glob.hxx"
#include "pres.hxx"

#include <vcl/ctrl.hxx>
#include <svtools/valueset.hxx>
#include <svtools/transfer.hxx>
#include <sfx2/shell.hxx>

#include <com/sun/star/frame/XStatusListener.hpp>
#include <com/sun/star/ui/XSidebar.hpp>


class SfxModule;

namespace css = ::com::sun::star;
namespace cssu = ::com::sun::star::uno;

namespace sd {
class DrawDocShell;
class PaneManagerEvent;
class ViewShellBase;
}


namespace sd { namespace tools {
class EventMultiplexerEvent;
} }


namespace sd { namespace sidebar {

class ControlFactory;
class SidebarViewShell;
class SidebarShellManager;


class LayoutMenu
    : public ValueSet,
      public SfxShell,
      public DragSourceHelper, 
      public DropTargetHelper,
      public ILayoutableWindow,
      public IDisposable,
      public ISidebarReceiver
{
public:
    TYPEINFO();
    SFX_DECL_INTERFACE(SD_IF_SDLAYOUTMENU)

    /** Create a new layout menu.  Depending on the given flag it
        displays its own scroll bar or lets a surrounding window
        handle that.
        @param i_pParent
            the parent node in the control tree
        @param i_rPanelViewShell
            the view shell of the task pane.
    */
    LayoutMenu (
        ::Window* pParent,
        ViewShellBase& rViewShellBase,
        SidebarShellManager& rSubShellManager);
    virtual ~LayoutMenu (void);

    virtual void Dispose (void);
    
    /** Return a numerical value representing the currently selected
        layout.
    */
    AutoLayout GetSelectedAutoLayout (void);

    Size GetPreferredSize (void);
    sal_Int32 GetPreferredWidth (sal_Int32 nHeight);
    sal_Int32 GetMinimumWidth (void);

    // From ILayoutableWindow
    virtual css::ui::LayoutSize GetHeightForWidth (const sal_Int32 nWidth);

    // From ::Window
	virtual void Paint (const Rectangle& rRect);
    virtual void Resize (void);

    /** Show a context menu when the right mouse button is pressed.
    */
    virtual void MouseButtonDown (const MouseEvent& rEvent);

    void Execute (SfxRequest& rRequest);
    void GetState (SfxItemSet& rItemSet);

    /** The LayoutMenu does not support some main views.  In this case the
        LayoutMenu is disabled.  This state is updated in this method.
        @param eMode
            On some occasions the edit mode is being switched when this
            method is called can not (yet) be reliably detected.  Luckily,
            in these cases the new value is provided by some broadcaster.
            On other occasions the edit mode is not modified and is also not
            provided.  Therefore the Unknown value.
    */
    enum MasterMode { MM_NORMAL, MM_MASTER, MM_UNKNOWN };
    void UpdateEnabledState (const MasterMode eMode);

    SidebarShellManager* GetShellManager (void);

    /** Call this method when the set of displayed layouts is not up-to-date
        anymore.  It will re-assemple this set according to the current
        settings.
    */
    void InvalidateContent (void);

	// DragSourceHelper
	virtual void StartDrag (sal_Int8 nAction, const Point& rPosPixel);

	// DropTargetHelper
	virtual sal_Int8 AcceptDrop (const AcceptDropEvent& rEvent);
	virtual sal_Int8 ExecuteDrop (const ExecuteDropEvent& rEvent);

    /** The context menu is requested over this Command() method.
    */
	virtual void Command (const CommandEvent& rEvent);

    /** Call Fill() when switching to or from high contrast mode so that the
        correct set of icons is displayed.
    */
    virtual void DataChanged (const DataChangedEvent& rEvent);

    // ISidebarReceiver
    virtual void SetSidebar (const cssu::Reference<css::ui::XSidebar>& rxSidebar);

	using Window::GetWindow;
	using ValueSet::StartDrag;

private:
    ViewShellBase& mrBase;
    SidebarShellManager& mrShellManager;

    /** Do we use our own scroll bar or is viewport handling done by
        our parent?
    */
    bool mbUseOwnScrollBar;

    /** If we are asked for the preferred window size, then use this
        many columns for the calculation.
    */
    const int mnPreferredColumnCount;
    cssu::Reference<css::frame::XStatusListener> mxListener;
    bool mbSelectionUpdatePending;
    bool mbIsMainViewChangePending;
    cssu::Reference<css::ui::XSidebar> mxSidebar;
    bool mbIsDisposed;

    /** Calculate the number of displayed rows.  This depends on the given
        item size, the given number of columns, and the size of the
        control.  Note that this is not the number of rows managed by the
        valueset.  This number may be larger.  In that case a vertical
        scroll bar is displayed.
    */
    int CalculateRowCount (const Size& rItemSize, int nColumnCount);

    /** Fill the value set with the layouts that are applicable to the
        current main view shell.
    */
    void Fill (void);

    /** Remove all items from the value set.
    */
    void Clear (void);

    /** Assign the given layout to all selected slides of a slide sorter.
        If no slide sorter is active then this call is ignored.  The slide
        sorter in the center pane is preferred if the choice exists.
    */
    void AssignLayoutToSelectedSlides (AutoLayout aLayout);

    /** Insert a new page with the given layout.  The page is inserted via
        the main view shell, i.e. its SID_INSERTPAGE slot is called.  It it
        does not support this slot then inserting a new page does not take
        place.  The new page is inserted after the currently active one (the
        one returned by ViewShell::GetActualPage().)
    */
    void InsertPageWithLayout (AutoLayout aLayout);

    /** Create a request structure that can be used with the SID_INSERTPAGE
        and SID_MODIFYPAGE slots.  The parameters are set so that the given
        layout is assigned to the current page of the main view shell.
        @param nSlotId
            Supported slots are SID_INSERTPAGE and SID_MODIFYPAGE.
        @param aLayout
            Layout of the page to insert or to assign.
    */
    SfxRequest CreateRequest (
        sal_uInt16 nSlotId,
        AutoLayout aLayout);

    /** Select the layout that is used by the current page.
    */
    void UpdateSelection (void);

    // internal ctor
    void    implConstruct( DrawDocShell& rDocumentShell );

    /** When clicked then set the current page of the view in the center pane.
    */
    DECL_LINK(ClickHandler, ValueSet*);
    DECL_LINK(RightClickHandler, MouseEvent*);
    DECL_LINK(StateChangeHandler, ::rtl::OUString*);
    DECL_LINK(EventMultiplexerListener, ::sd::tools::EventMultiplexerEvent*);
    DECL_LINK(WindowEventHandler, VclWindowEvent*);
};

} } // end of namespace ::sd::toolpanel

#endif
