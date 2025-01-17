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

#include "stdafx.h"
#include "UAccCOM2.h"
#include "MAccessible.h"

#include <algorithm>
#include "AccAction.h"

#include <com/sun/star/accessibility/XAccessibleText.hpp>
#include <com/sun/star/accessibility/XAccessibleEditableText.hpp>
#include <com/sun/star/accessibility/XAccessibleImage.hpp>
#include <com/sun/star/accessibility/XAccessibleTable.hpp>
#include <com/sun/star/accessibility/XAccessibleExtendedComponent.hpp>
#include <com/sun/star/accessibility/XAccessibleAction.hpp>
#include <com/sun/star/accessibility/XAccessibleKeyBinding.hpp>
#include <com/sun/star/accessibility/XAccessibleHyperText.hpp>
#include <com/sun/star/accessibility/XAccessibleHyperlink.hpp>
#include <com/sun/star/accessibility/XAccessibleRelationSet.hpp>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/accessibility/XAccessibleGroupPosition.hpp>
#include <com/sun/star/accessibility/XAccessibleValue.hpp>
#include <com/sun/star/accessibility/XAccessibleExtendedAttributes.hpp>
#include <com/sun/star/style/LineSpacing.hpp>
#include <com/sun/star/style/TabStop.hpp>
#include <com/sun/star/container/XIndexReplace.hpp>

#include "act.hxx"

using namespace com::sun::star::accessibility::AccessibleStateType;

// IA2 states mapping, and name
// maintenance the consistency, change one array, change the three all
long IA2_STATES[] =
{
	IA2_STATE_ACTIVE,					// =					0x1;
	IA2_STATE_ARMED,					// =					0x2;
	IA2_STATE_DEFUNCT,					// =					0x4;
	IA2_STATE_EDITABLE,					// =					0x8;
	IA2_STATE_HORIZONTAL,				// =					0x10;
	IA2_STATE_ICONIFIED,				// =					0x20;
	IA2_STATE_INVALID_ENTRY,			// =					0x80;
	IA2_STATE_MANAGES_DESCENDANTS,		// =					0x100;
	IA2_STATE_MODAL,					// =					0x200;
	IA2_STATE_MULTI_LINE,				// =					0x400;
	IA2_STATE_OPAQUE,					// =					0x800;
	IA2_STATE_REQUIRED,					// =					0x2000;
	IA2_STATE_SELECTABLE_TEXT,			// =					0x3000;
	IA2_STATE_SINGLE_LINE,				// =					0x4000;
	IA2_STATE_STALE,					// =					0x8000;
	IA2_STATE_SUPPORTS_AUTOCOMPLETION,	// =					0x10000;
	IA2_STATE_TRANSIENT,				//=						0x20000;
	IA2_STATE_VERTICAL					// =					0x40000;
};
/*

<=== map ===>  

*/
short UNO_STATES[] =
{
	ACTIVE,			// = (sal_Int16)1;
	ARMED,			// = (sal_Int16)2;
	DEFUNC,			// = (sal_Int16)5;
	EDITABLE,		// = (sal_Int16)6;
	HORIZONTAL,		// = (sal_Int16)12;
	ICONIFIED,		// = (sal_Int16)13;
	-1,				//IA2_STATE_INVALID_ENTRY
	MANAGES_DESCENDANTS, // = (sal_Int16)15;
	MODAL,			// = (sal_Int16)16;
	MULTI_LINE,		// = (sal_Int16)17;
	OPAQUE,			// = (sal_Int16)19;
	-1,				//IA2_STATE_REQUIRED
	-1,				//IA2_STATE_SELECTABLE_TEXT
	SINGLE_LINE,	// = (sal_Int16)26;
	STALE,			// = (sal_Int16)27;
	-1,				//IA2_STATE_SUPPORTS_AUTOCOMPLETION
	TRANSIENT,		//IA2_STATE_TRANSIENT
	VERTICAL		// = (sal_Int16)29;
};

//  <=== map ===>

BSTR IA2_STATES_NAME[] =
{
	_T("Active"),
	_T("Armed"),
	_T("Defunct"),
	_T("Editable"),
	_T("Horizontal"),
	_T("Iconified"),
	_T("Invalid Entry"),
	_T("Manages Descendants"),
	_T("Modal"),
	_T("Multi Line"),
	_T("Opaque"),
	_T("Required"),
	_T("Selectable Text"),
	_T("Single Line"),
	_T("Stale"),
	_T("Supports Autocompletion"),
	_T("Transient"),
	_T("Vertical")
};

// IA2 states mapping, and name
// maintenance the consistency. change one, change them all

BSTR UNO_ALL_STATES[] =
{
	_T("INVALID"),			// 	INVALID	( 0 )
	_T("ACTIVE"),			// 	ACTIVE	( 1 )
	_T("ARMED"),			// 	ARMED	( 2 )
	_T("BUSY"),				// 	BUSY	( 3 )
	_T("CHECKED"),			// 	CHECKED	( 4 )
	_T("DEFUNC"),			// 	DEFUNC	( 5 )
	_T("EDITABLE"),			// 	EDITABLE	( 6 )
	_T("ENABLED"),			// 	ENABLED	( 7 )
	_T("EXPANDABLE"),		// 	EXPANDABLE	( 8 )
	_T("EXPANDED"),			// 	EXPANDED	( 9 )
	_T("FOCUSABLE"),		// 	FOCUSABLE	( 10 )
	_T("FOCUSED"),			// 	FOCUSED	( 11 )
	_T("HORIZONTAL"),		// 	HORIZONTAL	( 12 )
	_T("ICONIFIED"),		// 	ICONIFIED	( 13 )
	_T("INDETERMINATE"),	// 	INDETERMINATE	( 14 )
	_T("MANAGES_DESCENDANTS"),// 	MANAGES_DESCENDANTS	( 15 )
	_T("MODAL"),			// 	MODAL	( 16 )
	_T("MULTI_LINE"),		// 	MULTI_LINE	( 17 )
	_T("MULTI_SELECTABLE"),	// 	MULTI_SELECTABLE	( 18 )
	_T("OPAQUE"),			// 	OPAQUE	( 19 )
	_T("PRESSED"),			// 	PRESSED	( 20 )
	_T("RESIZABLE"),		// 	RESIZABLE	( 21 )
	_T("SELECTABLE"),		// 	SELECTABLE	( 22 )
	_T("SELECTED"),			// 	SELECTED	( 23 )
	_T("SENSITIVE"),		// 	SENSITIVE	( 24 )
	_T("SHOWING"),			// 	SHOWING	( 25 )
	_T("SINGLE_LINE"),		// 	SINGLE_LINE	( 26 )
	_T("STALE"),			// 	STALE	( 27 )
	_T("TRANSIENT"),		// 	TRANSIENT	( 28 )
	_T("VERTICAL"),			// 	VERTICAL	( 29 )
	_T("VISIBLE"),			// 	VISIBLE	( 30 )
	_T("MOVEABLE"),			//  MOVEABLE ( 31 )
	_T("OFFSCREEN"),		//  OFFSCREEN ( 32 )
	_T("COLLAPSE"),			//  COLLAPSE ( 33 )
	_T("DEFAULT")			//  DEFAULT ( 34 )
};


using namespace com::sun::star::accessibility::AccessibleRole;



#define QUERYXINTERFACE(ainterface)	\
{							\
	if(pXAcc == NULL)		\
	return FALSE;		\
	pRContext = pXAcc->getAccessibleContext();	\
	if( !pRContext.is() )	\
{						\
	return FALSE;		\
}						\
	Reference<X##ainterface> pRXI(pRContext,UNO_QUERY);\
	if( !pRXI.is() )		\
{						\
	return FALSE;		\
}						\
	*ppXI = (XInterface*)pRXI.get();		\
	return TRUE;			\
}

#define ISDESTROY()	\
	if(m_isDestroy)	\
	return S_FALSE;


AccObjectManagerAgent* CMAccessible::g_pAgent = NULL;

CMAccessible::CMAccessible():
m_iRole(0x00),
m_dState(0x00),
m_dChildID(0x00),
m_dFocusChildID(UACC_NO_FOCUS),
m_hwnd(NULL),
m_pIParent(NULL),
m_pszName(NULL),
m_pszValue(NULL),
m_pszDescription(NULL),
m_isDestroy(FALSE),
m_pszActionDescription(NULL),
m_pXAction(NULL),
m_bRequiresSave(FALSE),
pUNOInterface(NULL)
{
	m_sLocation.m_dLeft=0;
	m_sLocation.m_dTop = 0;
	m_sLocation.m_dWidth=0;
	m_sLocation.m_dHeight=0;
	CEnumVariant::Create(&m_pEnumVar);
	m_containedObjects.clear();
}

CMAccessible::~CMAccessible()
{
	if(m_pszName!=NULL)
	{
		SAFE_SYSFREESTRING(m_pszName);
		m_pszName=NULL;
	}
	if(m_pszValue!=NULL)
	{
		SAFE_SYSFREESTRING(m_pszValue);
		m_pszValue=NULL;
	}
	if(m_pszDescription!=NULL)
	{
		SAFE_SYSFREESTRING(m_pszDescription);
		m_pszDescription=NULL;
	}

	if(m_pszActionDescription!=NULL)
	{
		SAFE_SYSFREESTRING(m_pszActionDescription);
		m_pszActionDescription=NULL;
	}

	if(m_pIParent)
	{
		m_pIParent->Release();
		m_pIParent=NULL;
	}
	pRef = NULL;
	m_pEnumVar->Release();
	m_containedObjects.clear();
	pRContext = NULL;
}

/**
* Returns the Parent IAccessible interface pointer to AT. 
* It should add reference, and the client should release the component.
* It should return E_FAIL when the parent point is null.
* @param	ppdispParent [in,out] used to return the parent interface point.
*			when the point is null, should return null.
* @return   S_OK if successful and E_FAIL if the m_pIParent is NULL.
*/
STDMETHODIMP CMAccessible::get_accParent(IDispatch **ppdispParent)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(ppdispParent == NULL)
		{
			return E_INVALIDARG;
		}

		if(m_pIParent)
		{
			*ppdispParent = m_pIParent;
			(*ppdispParent)->AddRef();
			return S_OK;
		}
		else if(m_hwnd)
		{
			HRESULT hr = AccessibleObjectFromWindow(m_hwnd, OBJID_WINDOW, IID_IAccessible, (void**)ppdispParent);
			if( ! SUCCEEDED( hr ) || ! ppdispParent )
			{
				return S_FALSE;
			}
			return S_OK;
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns child count of current COM object. 
* @param	pcountChildren [in,out] used to return the children count.
* @return   S_OK if successful.
*/
STDMETHODIMP CMAccessible::get_accChildCount(long *pcountChildren)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pcountChildren == NULL)
		{
			return E_INVALIDARG;
		}

		if(!pUNOInterface)
			return S_FALSE;

		Reference< XAccessibleContext > pRContext = pUNOInterface->getAccessibleContext();
		if( pRContext.is() )
		{
			*pcountChildren = pRContext->getAccessibleChildCount();
		}

		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns child interface pointer for AT according to input child ID. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	ppdispChild, [in,out] use to return the child interface point.
* @return   S_OK if successful and S_FALSE if failure.
*/
STDMETHODIMP CMAccessible::get_accChild(VARIANT varChild, IDispatch **ppdispChild)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(ppdispChild == NULL)
		{
			return E_INVALIDARG;
		}
		if(varChild.vt==VT_I4)
		{
			//get child interface pointer due to child ID
			if(varChild.lVal==CHILDID_SELF)
			{
				AddRef();
				*ppdispChild = this;
				return S_OK;
			}
			*ppdispChild = GetChildInterface(varChild.lVal);
			if((*ppdispChild) == NULL)
				return E_FAIL;
			(*ppdispChild)->AddRef();
			return S_OK;
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the accessible name of the current COM object self or its one child to AT. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	pszName, [in,out] use to return the name of the proper object.
* @return   S_OK if successful and S_FALSE if failure.
*/
STDMETHODIMP CMAccessible::get_accName(VARIANT varChild, BSTR *pszName)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pszName == NULL)
		{
			return E_INVALIDARG;
		}
		if(varChild.vt==VT_I4)
		{
			if(varChild.lVal==CHILDID_SELF)
			{
				SAFE_SYSFREESTRING(*pszName);
				*pszName = SysAllocString(m_pszName);
				return S_OK;
			}

			long lVal = varChild.lVal;
			varChild.lVal = CHILDID_SELF;
			IMAccessible *pChild = this->GetChildInterface(lVal);
			if(!pChild)
				return E_FAIL;
			return pChild->get_accName(varChild,pszName);
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the accessible value of the current COM object self or its one child to AT. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	pszValue, [in,out] use to return the value of the proper object.
* @return   S_OK if successful and S_FALSE if failure.
*/
STDMETHODIMP CMAccessible::get_accValue(VARIANT varChild, BSTR *pszValue)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if( pszValue == NULL )
		{
			return E_INVALIDARG;
		}
		if( varChild.vt==VT_I4 )
		{
			if(varChild.lVal==CHILDID_SELF)
			{
				if(m_dState & STATE_SYSTEM_PROTECTED)
					return E_ACCESSDENIED;

				if ( m_pszValue !=NULL && wcslen(m_pszValue) == 0 )
					return S_OK;

				SAFE_SYSFREESTRING(*pszValue);
				*pszValue = SysAllocString(m_pszValue);
				return S_OK;
			}

			long lVal = varChild.lVal;
			varChild.lVal = CHILDID_SELF;
			IMAccessible *pChild = this->GetChildInterface(lVal);
			if(!pChild)
				return E_FAIL;
			return pChild->get_accValue(varChild,pszValue);
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the accessible description of the current COM object self or its one child to AT. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	pszDescription, [in,out] use to return the description of the proper object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::get_accDescription(VARIANT varChild, BSTR *pszDescription)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pszDescription == NULL)
		{
			return E_INVALIDARG;
		}
		if(varChild.vt==VT_I4)
		{
			if(varChild.lVal==CHILDID_SELF)
			{
				SAFE_SYSFREESTRING(*pszDescription);
				*pszDescription = SysAllocString(m_pszDescription);
				return S_OK;
			}

			long lVal = varChild.lVal;
			varChild.lVal = CHILDID_SELF;
			IMAccessible *pChild = this->GetChildInterface(lVal);
			if(!pChild)
				return E_FAIL;
			return pChild->get_accDescription(varChild,pszDescription);
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the accessible role of the current COM object self or its one child to AT. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	pvarRole, [in,out] use to return the role of the proper object.
* @return   S_OK if successful and S_FALSE if failure.
*/
STDMETHODIMP CMAccessible::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pvarRole == NULL)
		{
			return E_INVALIDARG;
		}
		if(varChild.vt == VT_I4)
		{

			if(varChild.lVal == CHILDID_SELF)
			{
				if( m_iRole < IA2_ROLE_CAPTION )
				{
					VariantInit(pvarRole);
					pvarRole->vt = VT_I4;
					pvarRole->lVal = m_iRole;
				}
				else
				{
					VariantInit(pvarRole);
					pvarRole->vt = VT_I4;
					pvarRole->lVal = ROLE_SYSTEM_CLIENT;
				}
				return S_OK;
			}


			long lVal = varChild.lVal;
			varChild.lVal = CHILDID_SELF;
			IMAccessible *pChild = this->GetChildInterface(lVal);
			if(!pChild)
				return E_FAIL;
			return pChild->get_accRole(varChild,pvarRole);
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the accessible state of the current COM object self or its one child to AT. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	pvarState, [in,out] use to return the state of the proper object.
* @return   S_OK if successful and S_FALSE if failure.
*/
STDMETHODIMP CMAccessible::get_accState(VARIANT varChild, VARIANT *pvarState)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pvarState == NULL)
		{
			return E_INVALIDARG;
		}
		if(varChild.vt==VT_I4)
		{
			if(varChild.lVal == CHILDID_SELF)
			{
				if(pUNOInterface)
				{
					Reference< XAccessibleContext > pContext = pUNOInterface->getAccessibleContext();
					if(pContext.is())
					{
						// add the STATE_SYSTEM_LINKED state
						Reference< XAccessibleHypertext > pRHypertext(pContext,UNO_QUERY);
						if(pRHypertext.is())
						{
							if( pRHypertext->getHyperLinkCount() > 0 )
								m_dState |= STATE_SYSTEM_LINKED;
							else
								m_dState &= ~STATE_SYSTEM_LINKED;
						}
						else
							m_dState &= ~STATE_SYSTEM_LINKED;
					}
				}

				VariantInit(pvarState);
				pvarState->vt = VT_I4;
				pvarState->lVal = m_dState;
				return S_OK;
			}

			long lVal = varChild.lVal;
			varChild.lVal = CHILDID_SELF;
			IMAccessible *pChild = this->GetChildInterface(lVal);
			if(!pChild)
				return E_FAIL;
			return pChild->get_accState(varChild,pvarState);
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the accessible helpString of the current COM object self or its one child to AT. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	pszHelp, [in,out] use to return the helpString of the proper object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::get_accHelp(VARIANT, BSTR *)
{
	return E_NOTIMPL;
}

/**
* Returns the accessible HelpTopic of the current COM object self or its one child to AT. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	pszHelpFile, [in,out] use to return the HelpTopic of the proper object.
* @param	pidTopic, use to return the HelpTopic ID of the proper object.
* @return   S_OK if successful and E_FAIL if failure.
* Not implemented yet
*/
STDMETHODIMP CMAccessible::get_accHelpTopic(BSTR *, VARIANT, long *)
{
	return E_NOTIMPL;
}

static void GetMnemonicChar( const ::rtl::OUString& aStr, WCHAR* wStr)
{
	int  nLen    = aStr.pData->length;
	int  i       = 0;
	WCHAR* text = aStr.pData->buffer;

	while ( i < nLen )
	{
		if ( text[i] == L'~' )
			if ( text[i+1] != L'~' )
			{
				wStr[0] = text[i+1];
				break;
			}
			i++;
	}
}

/**
* Returns the accessible keyboard shortcut of the current COM object self or its one child to AT. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	pszKeyboardShortcut, [in,out] use to return the kbshortcut of the proper object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszKeyboardShortcut)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK

		ISDESTROY()
		// #CHECK#
		if(pszKeyboardShortcut == NULL)
		{
			return E_INVALIDARG;
		}

		if(varChild.vt==VT_I4)
		{
			if(varChild.lVal == CHILDID_SELF)
			{
				if( pUNOInterface )
				{
					Reference<XAccessibleContext> pRContext = pUNOInterface->getAccessibleContext();
					if( !pRContext.is() )
						return S_FALSE;

					Reference<XAccessibleAction> pRXI(pRContext,UNO_QUERY);

					OLECHAR wString[64]={0};

					if( pRXI.is() && pRXI->getAccessibleActionCount() >= 1)
					{
						Reference< XAccessibleKeyBinding > binding = pRXI->getAccessibleActionKeyBinding(0);
						if( binding.is() )
						{
							long nCount = binding->getAccessibleKeyBindingCount();
							if(nCount >= 1)
							{
								CAccAction::GetkeyBindingStrByXkeyBinding( binding->getAccessibleKeyBinding(0),wString );
							}
						}
					}
					if(wString[0] == 0)
					{
						Reference<XAccessibleRelationSet> pRrelationSet = pRContext->getAccessibleRelationSet();
						if(!pRrelationSet.is())
						{
							return S_FALSE;
						}

						long nRelCount = pRrelationSet->getRelationCount();

						// Modified by Steve Yin, for SODC_1552
						if( /*nRelCount <= 0 &&*/ m_iRole == ROLE_SYSTEM_TEXT )
						{
							VARIANT varParentRole;
							VariantInit( &varParentRole );

							m_pIParent->get_accRole(varChild, &varParentRole);

							if( m_pIParent && varParentRole.lVal == ROLE_SYSTEM_COMBOBOX ) // edit in comoboBox
							{
								m_pIParent->get_accKeyboardShortcut(varChild, pszKeyboardShortcut);
								return S_OK;
							}
						}

						AccessibleRelation *paccRelation = NULL;
						AccessibleRelation accRelation;
						for(int i=0; i<nRelCount ; i++)
						{
							if( pRrelationSet->getRelation(i).RelationType == 6 )
							{
								accRelation = pRrelationSet->getRelation(i);
								paccRelation = &accRelation;
							}
						}

						if(paccRelation == NULL)
							return S_FALSE;

						Sequence< Reference< XInterface > > xTargets = paccRelation->TargetSet;
						Reference<XInterface> pRAcc = xTargets[0];

						XAccessible* pXAcc = (XAccessible*)pRAcc.get();

						Reference<XAccessibleContext> pRLebelContext = pXAcc->getAccessibleContext();
						if(!pRLebelContext.is())
							return S_FALSE;

						pRrelationSet = pRLebelContext->getAccessibleRelationSet();
						nRelCount = pRrelationSet->getRelationCount();

						paccRelation = NULL;
						for(int j=0; j<nRelCount ; j++)
						{
							if( pRrelationSet->getRelation(j).RelationType == 5 )
							{
								accRelation = pRrelationSet->getRelation(j);
								paccRelation = &accRelation;
							}
						}

						if(paccRelation)
						{
							xTargets = paccRelation->TargetSet;
							pRAcc = xTargets[0];
							if(pUNOInterface != (XAccessible*)pRAcc.get())
								return S_FALSE;
						}

						Reference<XAccessibleExtendedComponent> pRXIE(pRLebelContext,UNO_QUERY);
						if(!pRXIE.is())
							return S_FALSE;

						::rtl::OUString ouStr = pRXIE->getTitledBorderText();
						WCHAR key[2] = {NULL};
						GetMnemonicChar(ouStr, key);
						if(key[0] != 0)
						{
							wcscat(wString, L"Alt+");
							wcscat(wString, key);
						}
						else
							return S_FALSE;
					}

					SAFE_SYSFREESTRING(*pszKeyboardShortcut);
					*pszKeyboardShortcut = SysAllocString(wString);

					return S_OK;
				}
				else
				{
					return S_FALSE;
				}
			}

			long lVal = varChild.lVal;
			varChild.lVal = CHILDID_SELF;
			IMAccessible *pChild = this->GetChildInterface(lVal);
			if(!pChild)
				return E_FAIL;

			return pChild->get_accKeyboardShortcut(varChild,pszKeyboardShortcut);
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the current focused child to AT. 
* @param	pvarChild, [in,out] vt member of pvarChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::get_accFocus(VARIANT *pvarChild)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pvarChild == NULL)
		{
			return E_INVALIDARG;
		}
		if( m_dFocusChildID==UACC_NO_FOCUS )
		{
			pvarChild->vt = VT_EMPTY;//no focus on the object and its children
			return S_OK;
		}
		//if the descendant of current object has focus indicated by m_dFocusChildID, return the IDispatch of this focused object
		else
		{
			IMAccessible* pIMAcc = NULL;
			g_pAgent->GetIAccessibleFromResID(m_dFocusChildID,&pIMAcc);
			pIMAcc->AddRef();
			pvarChild->vt = VT_DISPATCH;
			pvarChild->pdispVal = pIMAcc;

		}
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the selection of the current COM object to AT. 
* @param	pvarChildren,[in,out]
* if selection num is 0,return VT_EMPTY for vt,
* if selection num is 1,return VT_I4 for vt,and child index for lVal
* if selection num >1,return VT_UNKNOWN for vt, and IEnumVariant* for punkVal
* @return   S_OK if successful and S_FALSE if failure.
*/
STDMETHODIMP CMAccessible::get_accSelection(VARIANT *pvarChildren)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pvarChildren == NULL)
		{
			return E_INVALIDARG;
		}
		switch(m_pEnumVar->GetCountOfElements())
		{
		case 0:
			pvarChildren->vt = VT_EMPTY;
			break;
		case 1:
			VARIANT varTmp[1];
			ULONG count;
			VariantInit(&varTmp[0]);
			m_pEnumVar->Next(1,varTmp,&count);
			if(count!=1)
				return S_FALSE;
			pvarChildren->vt = VT_I4;
			pvarChildren->lVal = varTmp[0].lVal;
			VariantClear(&varTmp[0]);
			m_pEnumVar->Reset();
			break;
		default:
			pvarChildren->vt = VT_UNKNOWN;
			m_pEnumVar->AddRef();
			pvarChildren->punkVal = m_pEnumVar;
			break;
		}
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the location of the current COM object self or its one child to AT. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	pxLeft, [in,out] use to return the x-coordination of the proper object.
* @param	pyTop,  [in,out] use to return the y-coordination of the proper object.
* @param	pcxWidth, [in,out] use to return the x-coordination width of the proper object.
* @param	pcyHeight, [in,out] use to return the y-coordination height of the proper object.
* @return   S_OK if successful and S_FALSE if failure.
*/
STDMETHODIMP CMAccessible::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pxLeft == NULL || pyTop == NULL || pcxWidth == NULL || pcyHeight == NULL)
		{
			return E_INVALIDARG;
		}

		if(varChild.vt==VT_I4)
		{
			if(varChild.lVal==CHILDID_SELF)
			{

				if(pUNOInterface)
				{
					Reference< XAccessibleContext > pRContext = pUNOInterface->getAccessibleContext();
					if( !pRContext.is() )
						return S_FALSE;
					Reference< XAccessibleComponent > pRComponent(pRContext,UNO_QUERY);
					if( !pRComponent.is() )
						return S_FALSE;

					::com::sun::star::awt::Point pCPoint = pRComponent->getLocationOnScreen();
					::com::sun::star::awt::Size pCSize = pRComponent->getSize();
					*pxLeft = pCPoint.X;
					*pyTop =  pCPoint.Y;
					*pcxWidth = pCSize.Width;
					*pcyHeight = pCSize.Height;
					return S_OK;
				}
				else
				{
					*pxLeft = m_sLocation.m_dLeft;
					*pyTop = m_sLocation.m_dTop;
					*pcxWidth = m_sLocation.m_dWidth;
					*pcyHeight = m_sLocation.m_dHeight;
					return S_OK;
				}
			}

		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Returns the current focused child to AT. 
* @param	navDir, the direction flag of the navigation.
* @param	varStart, the start child id of this navigation action.
* @param	pvarEndUpAt, [in,out] the end up child of this navigation action.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pvarEndUpAt == NULL)
		{
			return E_INVALIDARG;
		}
		HRESULT ret = E_FAIL;
		switch (navDir)
		{
		case NAVDIR_FIRSTCHILD:
			ret = GetFirstChild(varStart,pvarEndUpAt);
			break;
		case NAVDIR_LASTCHILD:
			ret = GetLastChild(varStart,pvarEndUpAt);
			break;
		case NAVDIR_NEXT:
			ret = GetNextSibling(varStart,pvarEndUpAt);
			break;
		case NAVDIR_PREVIOUS:
			ret = GetPreSibling(varStart,pvarEndUpAt);
			break;
		case NAVDIR_DOWN://do not implement temporarily
			break;
		case NAVDIR_UP://do not implement temporarily
			break;
		case NAVDIR_LEFT://do not implement temporarily
			break;
		case NAVDIR_RIGHT://do not implement temporarily
			break;
		default:
			break;
		};
		return ret;

		LEAVE_PROTECTED_BLOCK
}

STDMETHODIMP CMAccessible::accHitTest(long xLeft, long yTop, VARIANT *pvarChild)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pvarChild == NULL)
		{
			return E_INVALIDARG;
		}
		long x, y, w, h;
		VARIANT varSelf;
		VariantInit(&varSelf);
		varSelf.vt = VT_I4;
		varSelf.lVal = CHILDID_SELF;
		accLocation(&x,&y,&w,&h,varSelf);
		if( (x < xLeft && (x + w) >xLeft) && (y < yTop && (y + h) >yTop) )
		{
			int i, nCount;
			pvarChild->vt = VT_EMPTY;
			Reference< XAccessibleContext > pRContext = GetContextByXAcc(pUNOInterface);
			nCount = pRContext->getAccessibleChildCount();
			if(nCount > 256)
				return E_FAIL;
			IMAccessible* child = NULL;
			for( i = 0; i<nCount; i++)
			{

				child = GetChildInterface(i + 1);
				if(child && child->accHitTest(xLeft,yTop,pvarChild) == S_OK)
					break;
			}

			if(pvarChild->vt == VT_DISPATCH)
				return S_OK;

			if( i < nCount)
			{
				pvarChild->vt = VT_DISPATCH;
				pvarChild->pdispVal = child;
				child->AddRef();
			}
			else
			{
				pvarChild->vt = VT_I4;
				pvarChild->lVal = CHILDID_SELF;
			}
			return S_OK;
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* Get The other Interface from CMAccessible. 
* @param	guidService, must be IID_IAccessible here.
* @param	riid, the IID interface .
* @return   S_OK if successful and S_FALSE if failure.
*/
STDMETHODIMP CMAccessible::QueryService(REFGUID guidService, REFIID riid, void** ppvObject)
{
	if( InlineIsEqualGUID(guidService, IID_IAccessible) )
		return QueryInterface(riid, ppvObject);
	return S_FALSE;
}

/**
* Set the accessible name of the current COM object self or its one child from UNO. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	szName, the name used to set the name of the proper object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::put_accName(VARIANT varChild, BSTR szName)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if(varChild.vt==VT_I4)
		{
			if(varChild.lVal==CHILDID_SELF)
			{
				SAFE_SYSFREESTRING(m_pszName);
				m_pszName=SysAllocString(szName);
				return S_OK;
			}

			long lVal = varChild.lVal;
			varChild.lVal = CHILDID_SELF;
			IMAccessible *pChild = this->GetChildInterface(lVal);
			if(!pChild)
				return E_FAIL;
			return pChild->put_accName(varChild,szName);
		}
		return E_FAIL;

		LEAVE_PROTECTED_BLOCK
}

/**
* Set the accessible value of the current COM object self or its one child from UNO. 
* @param	varChild, vt member of varChild must be VT_I4,and lVal member stores the child ID,
* the child ID specify child index from 0 to children count, 0 stands for object self.
* @param	szValue, the value used to set the value of the proper object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::put_accValue(VARIANT varChild, BSTR szValue)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if(varChild.vt==VT_I4)
		{
			if(varChild.lVal==CHILDID_SELF)
			{
				SysAllocString(m_pszValue);
				m_pszValue=SysAllocString(szValue);
				return S_OK;
			}

			long lVal = varChild.lVal;
			varChild.lVal = CHILDID_SELF;
			IMAccessible *pChild = this->GetChildInterface(lVal);
			if(!pChild)
				return E_FAIL;
			return pChild->put_accValue(varChild,szValue);
		}
		return E_FAIL;

		LEAVE_PROTECTED_BLOCK
}

/**
* Set the accessible name of the current COM object self from UNO. 
* @param	pszName, the name value used to set the name of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccName(const OLECHAR __RPC_FAR *pszName)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pszName == NULL)
		{
			return E_INVALIDARG;
		}

		SAFE_SYSFREESTRING(m_pszName);//??
		m_pszName = SysAllocString(pszName);
		if(m_pszName==NULL)
			return E_FAIL;
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
* Set the accessible role of the current COM object self from UNO. 
* @param	pRole, the role value used to set the role of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccRole(unsigned short pRole)
{
	m_iRole = pRole;
	return S_OK;
}

/**
* Add one state into the current state set for the current COM object from UNO. 
* @param	pXSate, the state used to set the name of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::DecreaseState(DWORD pXSate)
{
	m_dState &= (~pXSate);
	return S_OK;
}

/**
* Delete one state into the current state set for the current COM object from UNO. 
* @param	pXSate, the state used to set the name of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::IncreaseState(DWORD pXSate)
{
	m_dState |= pXSate;
	return S_OK;
}

/**
* Set state into the current state set for the current COM object from UNO. 
* @param	pXSate, the state used to set the name of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::SetState(DWORD pXSate)
{
	m_dState = pXSate;
	return S_OK;
}



/**
* Set the accessible description of the current COM object self from UNO. 
* @param	pszDescription, the name used to set the description of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccDescription(const OLECHAR __RPC_FAR *pszDescription)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pszDescription == NULL)
		{
			return E_INVALIDARG;
		}

		SAFE_SYSFREESTRING(m_pszDescription);
		m_pszDescription = SysAllocString(pszDescription);

		if(m_pszDescription==NULL)
			return E_FAIL;
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
* Set the accessible value of the current COM object self from UNO. 
* @param	pszAccValue, the name used to set the value of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccValue(const OLECHAR __RPC_FAR *pszAccValue)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pszAccValue == NULL)
		{
			return E_INVALIDARG;
		}
		SAFE_SYSFREESTRING(m_pszValue);
		m_pszValue = SysAllocString(pszAccValue);
		if(m_pszValue==NULL)
			return E_FAIL;
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
* Set the HWND value of the current COM object self from UNO. It should set the parent IAccessible
* Object through the method AccessibleObjectFromWindow(...).
* @param	hwnd, the HWND used to set the value of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccWindowHandle(HWND hwnd)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		m_hwnd = hwnd;
	return S_OK;

	LEAVE_PROTECTED_BLOCK
}

/**
* Set accessible focus by specifying child ID
* @param	dChildID, the child id identifies the focus child.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccFocus(long dChildID)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()

		if(dChildID==CHILDID_SELF)
		{
			if(m_pIParent)
			{
				m_pIParent->Put_XAccFocus(m_dChildID);
			}
		}
		else
		{
			m_dFocusChildID = dChildID;
			//traverse all ancestors to set the focused child ID so that when the get_accFocus is called on 
			//any of the ancestors, this id can be used to get the IAccessible of focused object. 
			if(m_pIParent)
			{
				m_pIParent->Put_XAccFocus(dChildID);
			}
		}
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
*Set accessible object location for the current COM object
* @param	sLocation, the location of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccLocation(const Location sLocation)
{

	this->m_sLocation = sLocation;
	return S_OK;
}

/**
* Set accessible parent object for the current COM object if 
* the current object is a child of some COM object
* @param	pIParent, the parent of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccParent(IMAccessible __RPC_FAR *pIParent)
{
	this->m_pIParent = pIParent;

	if(pIParent)
		m_pIParent->AddRef();

	return S_OK;
}

/**
* Set unique child id to COM
* @param	dChildID, the id of the current object.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccChildID(long dChildID)
{

	this->m_dChildID = dChildID;
	return S_OK;
}

/**
* Set AccObjectManagerAgent object pointer to COM
* @param	pAgent, the AccObjectManagerAgent point.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::Put_XAccAgent(long pAgent)
{
	g_pAgent = (AccObjectManagerAgent*)pAgent;
	return S_OK;
}

/**
* When a UNO control disposing, it disposes its listeners,
* then notify AccObject in bridge management, then notify 
* COM that the XAccessible is invalid,so set pUNOInterface as NULL
* @param	isDestroy, true is it need to be destroyed.
* @return   S_OK if successful and E_FAIL if failure.
*/
STDMETHODIMP CMAccessible::NotifyDestroy(BOOL isDestroy)
{

	m_isDestroy = isDestroy;
	pUNOInterface = NULL;
	return S_OK;
}

/**
*private methods that help implement public functions
*/

/**
* Return child interface pointer by child ID,note: need to call AddRef()
* @param	lChildID, specify child index,which AT(such as Inspect32) gives.
* @return  IMAccessible*, pointer to the corresponding child object.
*/
IMAccessible* CMAccessible::GetChildInterface(long dChildID)//for test
{

	long dChildIndex = 0;
	if(dChildID<0)
	{
		if(g_pAgent)
		{
			IMAccessible* pIMAcc = NULL;
			g_pAgent->GetIAccessibleFromResID(dChildID,&pIMAcc);
			return pIMAcc;
		}
		return NULL;
	}
	else
	{
		Reference< XAccessibleContext > pRContext = pUNOInterface->getAccessibleContext();
		if( !pRContext.is() )
			return NULL;

		if(dChildID<1 || dChildID>pRContext->getAccessibleChildCount())
			return NULL;

		IAccessible* pChild = NULL;
		Reference< XAccessible > pXChild = pRContext->getAccessibleChild(dChildID-1);
		BOOL isGet = get_IAccessibleFromXAccessible((long)pXChild.get(),&pChild);

		if(!isGet)
		{
			g_pAgent->InsertAccObj(pXChild.get(),pUNOInterface,(long)m_hwnd);
			isGet = get_IAccessibleFromXAccessible((long)pXChild.get(),&pChild);
		}

		if(isGet)
		{
			IMAccessible* pIMAcc =  (IMAccessible*)pChild;
			return pIMAcc;
		}
	}

	return NULL;
}

/**
* For List, tree and table,these roles belong to manage_decendant in UNO,
* need to process specifically when navigate
* @return  BOOL, if it is decendantmanager, return true.
*/
BOOL CMAccessible::IsDecendantManage()
{

	return (m_iRole==ROLE_SYSTEM_LIST)||(m_iRole==ROLE_SYSTEM_OUTLINE)||(m_iRole==ROLE_SYSTEM_TABLE);
}

/**
* for decendantmanager circumstance,provide child interface when navigate
* @param	varCur, the current child.
* @param	flags, the navigation direction.
* @return  IMAccessible*, the child of the end up node.
*/
IMAccessible* CMAccessible::GetNavigateChildForDM(VARIANT varCur, short flags)
{

	XAccessibleContext* pXContext = GetContextByXAcc(pUNOInterface);
	if(pXContext==NULL)
	{
		return NULL;
	}

	int count = pXContext->getAccessibleChildCount();
	if(count<1)
	{
		return NULL;
	}

	IMAccessible* pCurChild = NULL;
	XAccessible* pChildXAcc = NULL;
	Reference<XAccessible> pRChildXAcc;
	XAccessibleContext* pChildContext = NULL;
	int index = 0,delta=0;
	switch(flags)
	{
	case DM_FIRSTCHILD:
		pRChildXAcc = pXContext->getAccessibleChild(0);
		break;
	case DM_LASTCHILD:
		pRChildXAcc = pXContext->getAccessibleChild(count-1);
		break;
	case DM_NEXTCHILD:
	case DM_PREVCHILD:
		pCurChild = GetChildInterface(varCur.lVal);
		if(pCurChild==NULL)
		{
			return NULL;
		}
		pCurChild->GetUNOInterface((long*)&pChildXAcc);
		if(pChildXAcc==NULL)
		{
			return NULL;
		}
		pChildContext = GetContextByXAcc(pChildXAcc);
		if(pChildContext == NULL)
		{
			return NULL;
		}
		delta = (flags==DM_NEXTCHILD)?1:-1;
		//currently, getAccessibleIndexInParent is error in UNO for
		//some kind of List,such as ValueSet, the index will be less 1 than
		//what should be, need to fix UNO code
		index = pChildContext->getAccessibleIndexInParent()+delta;
		if((index>=0)&&(index<=count-1))
		{
			pRChildXAcc = pXContext->getAccessibleChild(index);
		}
		break;
	default:
		break;
	}

	if(!pRChildXAcc.is())
	{
		return NULL;
	}
	pChildXAcc = pRChildXAcc.get();
	g_pAgent->InsertAccObj(pChildXAcc,pUNOInterface);
	return g_pAgent->GetIMAccByXAcc(pChildXAcc);
}

/**
*the following 4 private methods are for accNavigate implementation
*/

/**
* Return first child for parent container, process differently according 
* to whether it is descendant manage
* @param	varStart, the start child id of this navigation action.
* @param	pvarEndUpAt, [in,out] the end up child of this navigation action.
* @return   S_OK if successful and E_FAIL if failure.
*/
HRESULT CMAccessible::GetFirstChild(VARIANT varStart,VARIANT* pvarEndUpAt)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pvarEndUpAt == NULL)
		{
			return E_INVALIDARG;
		}
		if(varStart.vt != VT_I4)
		{
			pvarEndUpAt->vt = VT_EMPTY;
			return E_INVALIDARG;
		}

		pvarEndUpAt->pdispVal = GetNavigateChildForDM(varStart, DM_FIRSTCHILD);
		if(pvarEndUpAt->pdispVal)
		{
			pvarEndUpAt->pdispVal->AddRef();
			pvarEndUpAt->vt = VT_DISPATCH;
			return S_OK;
		}

		pvarEndUpAt->vt = VT_EMPTY;
		return E_FAIL;

		LEAVE_PROTECTED_BLOCK
}

/**
* Return last child for parent container, process differently according 
* to whether it is descendant manage
* @param	varStart, the start child id of this navigation action.
* @param	pvarEndUpAt, [in,out] the end up child of this navigation action.
* @return   S_OK if successful and E_FAIL if failure.
*/
HRESULT CMAccessible::GetLastChild(VARIANT varStart,VARIANT* pvarEndUpAt)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pvarEndUpAt == NULL)
		{
			return E_INVALIDARG;
		}
		if(varStart.vt != VT_I4)
		{
			pvarEndUpAt->vt = VT_EMPTY;
			return E_INVALIDARG;
		}

		pvarEndUpAt->pdispVal = GetNavigateChildForDM(varStart, DM_LASTCHILD);
		if(pvarEndUpAt->pdispVal)
		{
			pvarEndUpAt->pdispVal->AddRef();
			pvarEndUpAt->vt = VT_DISPATCH;
			return S_OK;
		}
		pvarEndUpAt->vt = VT_EMPTY;
		return E_FAIL;

		LEAVE_PROTECTED_BLOCK
}

/**
* The method GetNextSibling is general, whatever it is descendant manage or not
* Get the next sibling object.
* @param	varStart, the start child id of this navigation action.
* @param	pvarEndUpAt, [in,out] the end up child of this navigation action.
* @return   S_OK if successful and E_FAIL if failure.
*/
HRESULT CMAccessible::GetNextSibling(VARIANT varStart,VARIANT* pvarEndUpAt)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if(varStart.vt != VT_I4)
		{
			pvarEndUpAt->vt = VT_EMPTY;
			return E_INVALIDARG;
		}

		Reference< XAccessibleContext > pRContext = GetContextByXAcc(pUNOInterface);
		if(pRContext.is())
		{
			varStart.iVal = sal_Int16(pRContext->getAccessibleIndexInParent() + 2);
			if(m_pIParent)
				if( m_pIParent->get_accChild(varStart,&pvarEndUpAt->pdispVal) == S_OK)
				{
					pvarEndUpAt->vt = VT_DISPATCH;
					return S_OK;
				}
		}
		pvarEndUpAt->vt = VT_EMPTY;
		return E_FAIL;

		LEAVE_PROTECTED_BLOCK
}

/**
*the method GetPreSibling is general, whatever it is descendant manage or not
* @param	varStart, the start child id of this navigation action.
* @param	pvarEndUpAt, [in,out] the end up child of this navigation action.
* @return   S_OK if successful and E_FAIL if failure.
*/
HRESULT CMAccessible::GetPreSibling(VARIANT varStart,VARIANT* pvarEndUpAt)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pvarEndUpAt == NULL)
		{
			return E_INVALIDARG;
		}
		if(varStart.vt != VT_I4)
		{
			pvarEndUpAt->vt = VT_EMPTY;
			return E_INVALIDARG;
		}

		Reference< XAccessibleContext > pRContext = GetContextByXAcc(pUNOInterface);
		if(pRContext.is())
		{
			varStart.iVal = sal_Int16(pRContext->getAccessibleIndexInParent());
			if(m_pIParent && varStart.iVal > 0)
				if( m_pIParent->get_accChild(varStart,&pvarEndUpAt->pdispVal) == S_OK)
				{
					pvarEndUpAt->vt = VT_DISPATCH;
					return S_OK;
				}
		}
		pvarEndUpAt->vt = VT_EMPTY;
		return E_FAIL;

		LEAVE_PROTECTED_BLOCK
}

/**
* For IAccessible2 implementation methods
*/
STDMETHODIMP CMAccessible::get_nRelations( long __RPC_FAR *nRelations)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()

		// #CHECK#
		if(nRelations == NULL)
		{
			return E_INVALIDARG;
		}

		*nRelations = 0;

		if( !pRContext.is() )
			return E_FAIL;
		Reference<XAccessibleRelationSet> pRrelationSet = pRContext.get()->getAccessibleRelationSet();
		if(!pRrelationSet.is())
		{
			*nRelations = 0;
			return S_OK;
		}

		*nRelations = pRrelationSet->getRelationCount();
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

STDMETHODIMP CMAccessible::get_relation( long relationIndex, IAccessibleRelation __RPC_FAR *__RPC_FAR *relation)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(relation == NULL)
		{
			return E_INVALIDARG;
		}

		if( !pRContext.is() )
			return E_FAIL;


		long nMax = 0;
		long nReal = 0;
		get_nRelations(&nMax);

		*relation = (IAccessibleRelation*)::CoTaskMemAlloc(sizeof(IAccessibleRelation));

		// #CHECK Memory Allocation#
		if(*relation == NULL)
		{
			return E_FAIL;
		}

		if( relationIndex < nMax )
		{


			Reference<XAccessibleRelationSet> pRrelationSet = pRContext.get()->getAccessibleRelationSet();
			if(!pRrelationSet.is())
			{

				return E_FAIL;
			}

			IAccessibleRelation* pRelation = NULL;
			ActivateActContext();
			HRESULT hr = CoCreateInstance( CLSID_AccRelation, NULL, CLSCTX_SERVER ,
				IID_IAccessibleRelation,
				(void **)&pRelation);
			DeactivateActContext();
			if(SUCCEEDED(hr))
			{
				IUNOXWrapper* wrapper = NULL;
				hr = pRelation->QueryInterface(IID_IUNOXWrapper, (void**)&wrapper);
				if(SUCCEEDED(hr))
				{
					AccessibleRelation accRelation = pRrelationSet->getRelation(relationIndex);
					wrapper->put_XSubInterface((long)&accRelation);
					wrapper->Release();
					*relation = pRelation;
					return S_OK;
				}

			}
		}

		return E_FAIL;

		LEAVE_PROTECTED_BLOCK
}

STDMETHODIMP CMAccessible::get_relations( long, IAccessibleRelation __RPC_FAR *__RPC_FAR *relation, long __RPC_FAR *nRelations)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()

		// #CHECK#
		if(relation == NULL || nRelations == NULL)
		{
			return E_INVALIDARG;
		}
		// #CHECK XInterface#

		if( !pRContext.is() )
			return E_FAIL;

		Reference<XAccessibleRelationSet> pRrelationSet = pRContext.get()->getAccessibleRelationSet();
		if(!pRrelationSet.is())
		{
			*nRelations = 0;
			return S_OK;
		}

		long nCount = pRrelationSet->getRelationCount();

		*relation = (IAccessibleRelation*)::CoTaskMemAlloc(nCount*sizeof(IAccessibleRelation));

		// #CHECK Memory Allocation#
		if(*relation == NULL)
		{
			return E_FAIL;
		}

		for(int i=0; i<nCount ; i++)
		{
			IAccessibleRelation* pRelation = NULL;
			ActivateActContext();
			HRESULT hr = CoCreateInstance( CLSID_AccRelation, NULL, CLSCTX_SERVER ,
				IID_IAccessibleRelation,
				(void **)&pRelation);
			DeactivateActContext();
			if(SUCCEEDED(hr))
			{
				IUNOXWrapper* wrapper = NULL;
				hr = pRelation->QueryInterface(IID_IUNOXWrapper, (void**)&wrapper);
				if(SUCCEEDED(hr))
				{
					AccessibleRelation accRelation = pRrelationSet->getRelation(i);
					wrapper->put_XSubInterface((long)&accRelation);
					wrapper->Release();
				}
				(relation)[i] = pRelation;
			}
		}

		*nRelations = nCount;
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

STDMETHODIMP CMAccessible::role(long __RPC_FAR *role)
{
	ENTER_PROTECTED_BLOCK

		(*role) = m_iRole;

	return S_OK;

	LEAVE_PROTECTED_BLOCK
}


STDMETHODIMP CMAccessible:: get_nActions(long __RPC_FAR *nActions)
{

	try
	{
		ISDESTROY()
			// #CHECK#
			if(nActions == NULL)
			{
				return E_INVALIDARG;
			}
			*nActions = 0L;
			IAccessibleAction* pAcc = NULL;
			HRESULT hr = QueryInterface(IID_IAccessibleAction, (void**)&pAcc);
			if( hr == S_OK )
			{
				pAcc->nActions(nActions);
				pAcc->Release();
			}

			return S_OK;
	}
	catch(...)
	{
		*nActions = 0L;
		return S_OK;
	}
}


STDMETHODIMP CMAccessible:: scrollToPoint(enum IA2CoordinateType, long, long)
{

	ENTER_PROTECTED_BLOCK
	ISDESTROY()
	return E_NOTIMPL;
	LEAVE_PROTECTED_BLOCK

}
STDMETHODIMP CMAccessible:: scrollTo(enum IA2ScrollType)
{

	ENTER_PROTECTED_BLOCK
	ISDESTROY()

	return E_NOTIMPL;

	LEAVE_PROTECTED_BLOCK
}

static XAccessible* getTheParentOfMember(XAccessible* pXAcc)
{
	// #CHECK#
	if(pXAcc == NULL)
	{
		return NULL;
	}
	Reference<XAccessibleContext> pRContext = pXAcc->getAccessibleContext();
	Reference<XAccessibleRelationSet> pRrelationSet = pRContext->getAccessibleRelationSet();
	long nRelations = pRrelationSet->getRelationCount();
	for(int i=0 ; i<nRelations ; i++)
	{
		AccessibleRelation accRelation = pRrelationSet->getRelation(i);
		if(accRelation.RelationType == 7)
		{
			Sequence< Reference< XInterface > > xTargets = accRelation.TargetSet;
			return (XAccessible*)xTargets[0].get();
		}
	}
	return NULL;
}

STDMETHODIMP CMAccessible:: get_groupPosition(long __RPC_FAR *groupLevel,long __RPC_FAR *similarItemsInGroup,long __RPC_FAR *positionInGroup)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(groupLevel == NULL || similarItemsInGroup == NULL || positionInGroup == NULL)
		{
			return E_INVALIDARG;
		}

		Reference<XAccessibleContext> pRContext = pUNOInterface->getAccessibleContext();
		if(!pRContext.is())
			return E_FAIL;
		long Role = pRContext->getAccessibleRole();

		*groupLevel = 0;
		*similarItemsInGroup = 0;
		*positionInGroup = 0;

		if (Role != AccessibleRole::DOCUMENT)
		{
			Reference< XAccessibleGroupPosition > xGroupPosition( pRContext, UNO_QUERY );
			if ( xGroupPosition.is() )
			{
				Sequence< sal_Int32 > rSeq = xGroupPosition->getGroupPosition( makeAny( pRContext ) );
				sal_Int32* pSeq = rSeq.getArray();
				if ( pSeq )
				{
					*groupLevel = pSeq[0];
					*similarItemsInGroup = pSeq[1];
					*positionInGroup = pSeq[2];
					return S_OK;
				}
				return S_OK;
			}
		}

		Reference< XAccessible> pParentAcc = pRContext->getAccessibleParent();
		if( !pParentAcc.is() )
		{
			return S_OK;
		}

		Reference<XAccessibleContext> pRParentContext = pParentAcc->getAccessibleContext();

		int level = 0;
		int index = 0;
		int number = 0;

		if( Role ==  RADIO_BUTTON )
		{
			Reference<XAccessibleRelationSet> pRrelationSet = pRContext->getAccessibleRelationSet();
			long nRel = pRrelationSet->getRelationCount();
			for(int i=0 ; i<nRel ; i++)
			{
				AccessibleRelation accRelation = pRrelationSet->getRelation(i);
				if(accRelation.RelationType == 7)
				{
					Sequence< Reference< XInterface > > xTargets = accRelation.TargetSet;
					int nCount = xTargets.getLength();

					Reference<XInterface> pRAcc = xTargets[0];
					for(int j=0; j<pRParentContext->getAccessibleChildCount(); j++)
					{
						if( getTheParentOfMember(pRParentContext->getAccessibleChild(j).get())
							== (XAccessible*)pRAcc.get() &&
							pRParentContext->getAccessibleChild(j)->getAccessibleContext()->getAccessibleRole() == RADIO_BUTTON)
							number++;
						if(pRParentContext->getAccessibleChild(j).get() == pUNOInterface)
							index = number;
					}
				}
			}
			*groupLevel = 1;
			*similarItemsInGroup = number;
			*positionInGroup = index;
			return S_OK;
		}

		else if ( COMBO_BOX == Role )
		{
			*groupLevel = 1;
			*similarItemsInGroup = 0;
			*positionInGroup = -1;

			long nCount = pRContext->getAccessibleChildCount();
			if( 2 != nCount)
			{
				return S_OK;
			}
			Reference<XAccessible> xList=pRContext->getAccessibleChild(1);
			if (!xList.is())
			{
				return S_OK;
			}
			Reference<XAccessibleContext> xListContext(xList,UNO_QUERY);
			if (!xListContext.is())
			{
				return S_OK;
			}
			Reference<XAccessibleSelection> xListSel(xList,UNO_QUERY);
			if (!xListSel.is())
			{
				return S_OK;
			}
			*similarItemsInGroup = xListContext->getAccessibleChildCount();
			if (*similarItemsInGroup > 0 )
			{
				try
				{
					Reference<XAccessible> xChild = xListSel->getSelectedAccessibleChild(0);
					if (xChild.is())
					{
						Reference<XAccessibleContext> xChildContext(xChild,UNO_QUERY);
						if (xChildContext.is())
						{
							*positionInGroup=xChildContext->getAccessibleIndexInParent() + 1 ;
							return S_OK;
						}
					}
				}
				catch(...)
				{}
			}
			return S_OK;
		}
		else if ( PAGE_TAB == Role )
		{
			*groupLevel = 1;
			*similarItemsInGroup = pRParentContext->getAccessibleChildCount();

			if (*similarItemsInGroup > 0 )
			{
				*positionInGroup=pRContext->getAccessibleIndexInParent() + 1 ;
			}
			else
			{
				*positionInGroup = -1;
			}
			return S_OK;
		}


		BOOL isFound = FALSE;
		while( pParentAcc.is() && !isFound)
		{
			level++;
			pRParentContext = pParentAcc->getAccessibleContext();
			Role = pRParentContext->getAccessibleRole();
			if( (Role == TREE) || (Role == LIST) )
				isFound = TRUE;
			pParentAcc = pRParentContext->getAccessibleParent();
		}

		if( isFound )
		{
			Reference< XAccessible> pTempAcc = pRContext->getAccessibleParent();
			pRParentContext = pTempAcc->getAccessibleContext();
			*groupLevel = level;
			*similarItemsInGroup = pRParentContext->getAccessibleChildCount();
			*positionInGroup = pRContext->getAccessibleIndexInParent() + 1;
		}
		else
		{
			*groupLevel = 0;
			*similarItemsInGroup = 0;
			*positionInGroup = 0;
		}
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

STDMETHODIMP CMAccessible:: get_extendedStates( long, BSTR __RPC_FAR *__RPC_FAR *, long __RPC_FAR *)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()

		return E_NOTIMPL;

	LEAVE_PROTECTED_BLOCK
}


STDMETHODIMP CMAccessible:: get_uniqueID(long __RPC_FAR *uniqueID)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(uniqueID == NULL)
		{
			return E_INVALIDARG;
		}
		*uniqueID = m_dChildID;
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

STDMETHODIMP CMAccessible:: get_windowHandle(HWND __RPC_FAR *windowHandle)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(windowHandle == NULL)
		{
			return E_INVALIDARG;
		}

		HWND nHwnd = m_hwnd;
		IAccessible* pParent = m_pIParent;
		CMAccessible* pChild = this;
		while((nHwnd==0) && pParent)
		{
			pChild = (CMAccessible*)pParent;
			if(pChild)
			{
				pParent = (IAccessible*)pChild->m_pIParent;
				nHwnd = (HWND)pChild->m_hwnd;
			}
			else
				pParent = NULL;
		}

		*windowHandle = nHwnd;
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
* Get XAccessibleContext directly from UNO by the stored XAccessible pointer
* @param	pXAcc, UNO XAccessible object point.
* @return   XAccessibleContext*, the context of the pXAcc.
*/
XAccessibleContext* CMAccessible::GetContextByXAcc( XAccessible* pXAcc )
{
	Reference< XAccessibleContext > pRContext;
	if( pXAcc == NULL)
		return NULL;

	pRContext = pXAcc->getAccessibleContext();
	if( !pRContext.is() )
		return NULL;
	return pRContext.get();
}

/**
* Return the member variable m_pXAccessibleSelection, instead of 
* get XAccessibleSelection according to XAccessibleContext because if so,it will 
* depend on the UNO implementation code,so when COM is created, put XAccessibleSelection
* by bridge management system
* @return   XAccessibleSelection*, the selection of the current object.
*/
Reference< XAccessibleSelection > CMAccessible::GetSelection()
{
	if( pUNOInterface == NULL )
		return NULL;
	Reference< XAccessibleContext > pRContext = pUNOInterface->getAccessibleContext();
	if(pRContext.is())
	{
		Reference< XAccessibleSelection > pRSelection(pRContext,UNO_QUERY);
		return pRSelection;
	}
	return NULL;
}

/**
* Select one XAccessible item, for accSelect implementation
* @param	pItem, the item should be selected.
* @return  S_OK if successful.
*/
HRESULT CMAccessible::SelectChild(XAccessible* pItem)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		XAccessibleContext* pParentContext = GetContextByXAcc( pUNOInterface );
	XAccessibleContext* pContext = GetContextByXAcc( pItem );
	if( pParentContext == NULL || pContext == NULL )
		return E_FAIL;

	Reference< XAccessibleSelection > pRSelection = GetSelection();
	if( !pRSelection.is() )
		return E_FAIL;
	long Index = pContext->getAccessibleIndexInParent();
	pRSelection->selectAccessibleChild( Index );
	return S_OK;

	LEAVE_PROTECTED_BLOCK
}

/**
* Deselect one XAccessible item, for accSelect implimentation
* @param	pItem, the item should be deselected.
* @return  S_OK if successful.
*/
HRESULT CMAccessible::DeSelectChild(XAccessible* pItem)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		XAccessibleContext* pParentContext = GetContextByXAcc( pUNOInterface );
	;
	XAccessibleContext* pContext = GetContextByXAcc( pItem );
	if( pParentContext == NULL || pContext == NULL )
		return E_INVALIDARG;

	Reference< XAccessibleSelection > pRSelection = GetSelection();
	if( !pRSelection.is() )
		return E_FAIL;
	long Index = pContext->getAccessibleIndexInParent();
	pRSelection->deselectAccessibleChild( Index );

	return S_OK;

	LEAVE_PROTECTED_BLOCK
}

/**
* Select multiple XAccessible items,for implementation of accSelect
* @param	pItem, the items should be selected.
* @param	size, the size of the items.
* @return  S_OK if successful.
*/
HRESULT	CMAccessible::SelectMultipleChidren( XAccessible** pItem,int size )
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pItem == NULL)
		{
			return E_INVALIDARG;
		}
		for(int index = 0;index < size;index++)
		{
			SelectChild( pItem[index] );
		}
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
* Deselect multiple XAccessible items,for implementation of accSelect
* @param	pItem, the items should be selected.
* @param	size, the size of the items.
* @return  S_OK if successful.
*/
HRESULT CMAccessible::DeSelectMultipleChildren( XAccessible** pItem,int size )
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pItem == NULL)
		{
			return E_INVALIDARG;
		}
		for(int index = 0;index < size;index++)
		{
			DeSelectChild( pItem[index] );
		}
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

/**
* When COM is created, UNO set XAccessible pointer to it
* in order to COM can operate UNO information
* @param	pXAcc, the XAccessible object of current object.
* @return  S_OK if successful.
*/
STDMETHODIMP CMAccessible::SetXAccessible(long pXAcc)
{
	pUNOInterface = (XAccessible*)pXAcc;
	pRef = pUNOInterface;
	m_pEnumVar->PutSelection(/*XAccessibleSelection*/(long)pUNOInterface);

	pRContext = pUNOInterface->getAccessibleContext();
	pRContextInterface = (XAccessibleContext*)pRContext.is();

	return S_OK;
}

/**
* accSelect method has many optional flags, needs to process comprehensively
* Mozilla and Microsoft do not implement SELFLAG_EXTENDSELECTION flag. 
* The implementation of this flag is a little trouble-shooting,so we also
* do not implement it now
* @param	flagsSelect, the selection flag of the select action.
* @param	varChild, the child object pointer of current action.
* @return  S_OK if successful.
*/
STDMETHODIMP CMAccessible::accSelect(long flagsSelect, VARIANT varChild)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if( (flagsSelect&SELFLAG_ADDSELECTION) &&
			(SELFLAG_REMOVESELECTION&flagsSelect) )
			return E_INVALIDARG;

	if ( (flagsSelect&SELFLAG_TAKESELECTION) &&
		(
		(flagsSelect&SELFLAG_ADDSELECTION) ||
		(flagsSelect&SELFLAG_REMOVESELECTION) ||
		(flagsSelect&SELFLAG_EXTENDSELECTION )
		)
		)
		return E_INVALIDARG;

	if ( varChild.vt !=	VT_I4 )
		return E_INVALIDARG;

	IMAccessible* pSelectAcc;
	if( varChild.lVal == CHILDID_SELF )
	{
		pSelectAcc = this;
		pSelectAcc->AddRef();
	}
	else
	{
		pSelectAcc = GetChildInterface(varChild.lVal);
	}

	if( pSelectAcc == NULL )
		return E_INVALIDARG;

	if( flagsSelect&SELFLAG_TAKEFOCUS )
	{
		long pTempUNO = 0;
		pSelectAcc->GetUNOInterface( &pTempUNO);

		if( pTempUNO == NULL )
			return NULL;

		Reference< XAccessibleContext > pRContext = ( (XAccessible*)pTempUNO)->getAccessibleContext();
		Reference< XAccessibleComponent > pRComponent(pRContext,UNO_QUERY);
		Reference< XAccessible > pRParentXAcc = pRContext->getAccessibleParent();
		Reference< XAccessibleContext > pRParentContext = pRParentXAcc->getAccessibleContext();
		Reference< XAccessibleComponent > pRParentComponent(pRParentContext,UNO_QUERY);
		Reference< XAccessibleSelection > pRParentSelection(pRParentContext,UNO_QUERY);


		pRComponent->grabFocus();

		if( flagsSelect & SELFLAG_TAKESELECTION )
		{
			pRParentSelection->clearAccessibleSelection();
			pRParentSelection->selectAccessibleChild( pRContext->getAccessibleIndexInParent() );
		}

		if( flagsSelect & SELFLAG_ADDSELECTION  )
		{
			pRParentSelection->selectAccessibleChild( pRContext->getAccessibleIndexInParent() );
		}

		if( flagsSelect & SELFLAG_REMOVESELECTION )
		{
			pRParentSelection->deselectAccessibleChild( pRContext->getAccessibleIndexInParent() );
		}

		if( flagsSelect & SELFLAG_EXTENDSELECTION  )
		{
			long indexInParrent = pRContext->getAccessibleIndexInParent();

			if( pRParentSelection->isAccessibleChildSelected( indexInParrent + 1 ) ||
				pRParentSelection->isAccessibleChildSelected( indexInParrent - 1 ) )
			{
				pRParentSelection->selectAccessibleChild( indexInParrent );
			}
		}

	}

	pSelectAcc->Release();
	return S_OK;

	LEAVE_PROTECTED_BLOCK
}

/**
* Return XAccessible interface pointer when needed
* @param pXAcc, [in, out] the Uno interface of the current object.
* @return S_OK if successful.
*/
STDMETHODIMP CMAccessible::GetUNOInterface(long* pXAcc)
{
	// #CHECK#
	if(pXAcc == NULL)
		return E_INVALIDARG;

	*pXAcc = (long)pUNOInterface;
	return S_OK;
}

/**
* Helper method for Implementation of get_accDefaultAction
* @param pAction, the default action point of the current object.
* @return S_OK if successful.
*/
STDMETHODIMP CMAccessible::SetDefaultAction(long pAction)
{
	m_pXAction = (XAccessibleAction*)pAction;
	return S_OK;
}

/**
* This method is called when AT open some UI elements initially
* the UI element takes the default action defined here
* @param varChild, the child id of the defaultaction.
* @param pszDefaultAction,[in/out] the description of the current action.
* @return S_OK if successful.
*/
HRESULT STDMETHODCALLTYPE CMAccessible::get_accDefaultAction(VARIANT varChild, BSTR *pszDefaultAction)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(pszDefaultAction == NULL)
		{
			return E_INVALIDARG;
		}
		if(varChild.vt==VT_I4)
		{
			if(varChild.lVal==CHILDID_SELF)
			{
				if( m_pXAction == NULL )
					return DISP_E_MEMBERNOTFOUND;
				SAFE_SYSFREESTRING(*pszDefaultAction);
				*pszDefaultAction = SysAllocString(m_pszActionDescription);
				return S_OK;
			}

			long lVal = varChild.lVal;
			varChild.lVal = CHILDID_SELF;
			IMAccessible *pChild = this->GetChildInterface(lVal);
			if(!pChild)
				return E_FAIL;
			return pChild->get_accDefaultAction(varChild,pszDefaultAction);
		}
		return S_FALSE;

		LEAVE_PROTECTED_BLOCK
}

/**
* AT call this method to operate application
* @param varChild, the child id of the action object.
* @return S_OK if successful.
*/
HRESULT STDMETHODCALLTYPE CMAccessible::accDoDefaultAction(VARIANT varChild)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if( varChild.vt != VT_I4 )
			return E_INVALIDARG;
	if( m_pXAction == NULL )
		return E_FAIL;
	if( m_pXAction->getAccessibleActionCount() == 0 )
		return E_FAIL;

	if(varChild.lVal==CHILDID_SELF)
	{
		if(m_pXAction->getAccessibleActionCount() > 0)
			m_pXAction->doAccessibleAction(0);
		return S_OK;
	}

	long lVal = varChild.lVal;
	varChild.lVal = CHILDID_SELF;
	IMAccessible *pChild = this->GetChildInterface(lVal);
	if(!pChild)
		return E_FAIL;
	return pChild->accDoDefaultAction( varChild );

	LEAVE_PROTECTED_BLOCK
}

/**
* UNO set description information for action to COM.
* @param szAction, the action description of the current object.
* @return S_OK if successful.
*/
STDMETHODIMP CMAccessible::Put_ActionDescription( const OLECHAR* szAction)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(szAction == NULL)
		{
			return E_INVALIDARG;
		}
		SAFE_SYSFREESTRING(m_pszActionDescription );
		m_pszActionDescription = SysAllocString( szAction );
		return S_OK;

		LEAVE_PROTECTED_BLOCK
}

BOOL CMAccessible::GetXInterfaceFromXAccessible(XAccessible* pXAcc, XInterface** ppXI, int index)
{
	Reference< XAccessibleContext > pRContext;

	switch(index)
	{
	case XI_COMPONENT:
		QUERYXINTERFACE(AccessibleComponent)
			break;
	case XI_TEXT:
		QUERYXINTERFACE(AccessibleText)
			break;
	case XI_EDITABLETEXT:
		QUERYXINTERFACE(AccessibleEditableText)
			break;
	case XI_TABLE:
		QUERYXINTERFACE(AccessibleTable)
			break;
	case XI_SELECTION:
		QUERYXINTERFACE(AccessibleSelection)
			break;
	case XI_EXTENDEDCOMP:
		QUERYXINTERFACE(AccessibleExtendedComponent)
			break;
	case XI_KEYBINDING:
		QUERYXINTERFACE(AccessibleKeyBinding)
			break;
	case XI_ACTION:
		QUERYXINTERFACE(AccessibleAction)
			break;
	case XI_VALUE:
		QUERYXINTERFACE(AccessibleValue)
			break;
	case XI_HYPERTEXT:
		QUERYXINTERFACE(AccessibleHypertext)
			break;
	case XI_HYPERLINK:
		QUERYXINTERFACE(AccessibleHyperlink)
			break;
	case XI_IMAGE:
		QUERYXINTERFACE(AccessibleImage)
			break;
	default:
		break;
	}

	return FALSE;
}

HRESULT WINAPI CMAccessible::SmartQI(void* pv, REFIID iid, void** ppvObject)
{
	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if( ImplIsEqualGUID(iid,IID_IAccIdentity) ||
			ImplIsEqualGUID(iid,IID_IStdMarshalInfo) ||
			ImplIsEqualGUID(iid,IID_IMarshal) ||
			ImplIsEqualGUID(iid,IID_IExternalConnection)||
			ImplIsEqualGUID(iid,IID_IOleWindow))
			return E_FAIL;


	_UNO_AGGMAP_ENTRY* pMap = _GetAggEntries();
	while(pMap && pMap->piid)
	{
		if(ImplIsEqualGUID(iid, *pMap->piid))
		{
			XInterface* pXI = NULL;
			BOOL bFound = GetXInterfaceFromXAccessible(pUNOInterface,&pXI,pMap->XIFIndex);
			if(!bFound)
			{
				return E_FAIL;
			}

			XGUIDToComObjHash::iterator pIndTemp = m_containedObjects.find( iid );
			if ( pIndTemp != m_containedObjects.end() )
			{
				return pIndTemp->second.p->QueryInterface( iid, ppvObject );
			}
			else
			{
				ActivateActContext();
				HRESULT hr = pMap->pfnCreateInstance(pv, iid, ppvObject);
				DeactivateActContext();
				if(hr == S_OK)
				{
					m_containedObjects.insert(XGUIDToComObjHash::value_type(*pMap->piid,(IUnknown*)*ppvObject));
					IUNOXWrapper* wrapper = NULL;
					((IUnknown*)*ppvObject)->QueryInterface(IID_IUNOXWrapper, (void**)&wrapper);
					if(wrapper)
					{
						wrapper->put_XInterface((long)pUNOInterface);
						wrapper->Release();
					}
					return S_OK;
				}
			}
			return E_FAIL;
		}
		pMap++;
	}
	return E_FAIL;

	LEAVE_PROTECTED_BLOCK
}

BOOL CMAccessible::get_IAccessibleFromXAccessible(long pXAcc, IAccessible **ppIA)
{

	ENTER_PROTECTED_BLOCK

		// #CHECK#
		if(ppIA == NULL)
		{
			return E_INVALIDARG;
		}
		BOOL isGet = FALSE;
		if(g_pAgent)
			isGet = g_pAgent->GetIAccessibleFromXAccessible((XAccessible*)pXAcc,ppIA);

		if(isGet)
			return TRUE;
		else
			return FALSE;

		LEAVE_PROTECTED_BLOCK
}

void CMAccessible::get_OLECHARFromAny(Any& pAny, OLECHAR* pChar)
{
	// #CHECK#
	if(pChar == NULL)
		return;

	switch(pAny.getValueTypeClass())
	{
	case TypeClass_CHAR:
		{
			sal_Int8 val;
			pAny >>= val;
			swprintf( pChar, L"%d", val);
			break;
		}
	case TypeClass_BOOLEAN:
		{
			sal_Bool val;
			pAny >>= val;
			swprintf( pChar, L"%d", val);
			break;
		}
	case TypeClass_BYTE:
		{
			sal_Int8 val;
			pAny >>= val;
			swprintf( pChar, L"%d", val);
			break;
		}
	case TypeClass_SHORT:
		{
			SHORT val;
			pAny >>= val;
			swprintf( pChar, L"%d", val);
			break;
		}
	case TypeClass_UNSIGNED_SHORT:
		{
			USHORT val;
			pAny >>= val;
			swprintf( pChar, L"%d", val);
			break;
		}
	case TypeClass_LONG:
		{
			LONG val;
			pAny >>= val;
			swprintf( pChar, L"%ld", val);
			break;
		}
	case TypeClass_UNSIGNED_LONG:
		{
			ULONG val;
			pAny >>= val;
			swprintf( pChar, L"%ld", val);
			break;
		}
	case TypeClass_FLOAT:
		{
			FLOAT val;
			pAny >>= val;
			swprintf( pChar, L"%.3f", val);
			break;
		}
	case TypeClass_DOUBLE:
		{
			DOUBLE val;
			pAny >>= val;
			swprintf( pChar, L"%.6lf", val);
			break;
		}
	case TypeClass_STRING:
		{
			::rtl::OUString val;
			pAny >>= val;
			wcscpy(pChar, val.getStr());
			break;
		}
	case TypeClass_SEQUENCE:
		{
			if(pAny.getValueType() == getCppuType( (Sequence< ::rtl::OUString > *)0 ) )
			{
				Sequence < ::rtl::OUString > val;
				pAny >>= val;

				::rtl::OUString pString;

				int count = val.getLength();

				for( int iIndex = 0;iIndex < count;iIndex++ )
				{
					pString += val[iIndex];
				}
				wcscpy(pChar, pString.getStr());
			}
			else if (pAny.getValueType() == getCppuType( (Sequence< ::com::sun::star::style::TabStop >* )0 ) )
			{
				Sequence < ::com::sun::star::style::TabStop > val;
				pAny >>= val;
				int count = val.getLength();

				for( int iIndex = 0;iIndex < count;iIndex++ )
				{
					OLECHAR pAttrs[512] = {NULL};

					OLECHAR pAttrsPosition[512] = {NULL};
					OLECHAR pAttrsDescimalChar[512] = {NULL};
					OLECHAR pAttrsFillChar[512] = {NULL};

					::com::sun::star::style::TabStop sigleVal = val[iIndex];

					swprintf( pAttrsPosition, L"Position=%ld,TabAlign=%ld",
						sigleVal.Position, sigleVal.Alignment);

					if(sigleVal.DecimalChar==';' || sigleVal.DecimalChar == ':' || sigleVal.DecimalChar == ',' ||
						sigleVal.DecimalChar == '=' || sigleVal.DecimalChar == '\\')
						swprintf( pAttrsDescimalChar, L"DecimalChar=\\%c",sigleVal.DecimalChar);
					else
						swprintf( pAttrsDescimalChar, L"DecimalChar=%c",sigleVal.DecimalChar);

					if(sigleVal.FillChar==';' || sigleVal.FillChar == ':' || sigleVal.FillChar == ',' ||
						sigleVal.FillChar == '=' || sigleVal.FillChar == '\\')
						swprintf( pAttrsFillChar, L"FillChar=\\%c",sigleVal.FillChar);
					else
						swprintf( pAttrsFillChar, L"FillChar=%c",sigleVal.FillChar);

					swprintf( pAttrs, L"%s,%s,%s,",pAttrsPosition,pAttrsDescimalChar,pAttrsFillChar);

					wcscat(pChar,pAttrs);
				}
			}
			break;
		}
	case TypeClass_ENUM:
		{
			if (pAny.getValueType() == getCppuType( (::com::sun::star::awt::FontSlant* )0 ) )
			{
				com::sun::star::awt::FontSlant val;
				pAny >>= val;
				swprintf( pChar, L"%d", val);
			}
		}
	case TypeClass_STRUCT:
		{
			if (pAny.getValueType() == getCppuType( (::com::sun::star::style::LineSpacing* )0 ) )
			{
				com::sun::star::style::LineSpacing val;
				pAny >>= val;
				swprintf( pChar, L"Mode=%ld,Height=%ld,", val.Mode, val.Height);
			}
			else if (pAny.getValueType() == getCppuType( (com::sun::star::accessibility::TextSegment *)0 ) )
			{
				com::sun::star::accessibility::TextSegment val;
				pAny >>= val;
				::rtl::OUString realVal(val.SegmentText);
				wcscpy(pChar, realVal.getStr());
			}
			break;
		}
	case TypeClass_VOID:
	case TypeClass_HYPER:
	case TypeClass_UNSIGNED_HYPER:
	case TypeClass_TYPE:
	case TypeClass_ANY:
	case TypeClass_TYPEDEF:
	case TypeClass_UNION:
	case TypeClass_EXCEPTION:
	case TypeClass_ARRAY:
	case TypeClass_INTERFACE:
	case TypeClass_SERVICE:
	case TypeClass_MODULE:
	case TypeClass_INTERFACE_METHOD:
	case TypeClass_INTERFACE_ATTRIBUTE:
	case TypeClass_UNKNOWN:
	case TypeClass_PROPERTY:
	case TypeClass_CONSTANT:
	case TypeClass_CONSTANTS:
	case TypeClass_SINGLETON:
	case TypeClass_MAKE_FIXED_SIZE:
		break;
	default:
		break;
	}
}

void CMAccessible::get_OLECHAR4Numbering(const Any& pAny, short numberingLevel,const OUString& numberingPrefix,OLECHAR* pChar)
{
	if(pChar == NULL)
		return;
	Reference< ::com::sun::star::container::XIndexReplace > pXIndex;
	if((pAny>>=pXIndex) && (numberingLevel !=-1))//numbering level is -1,means invalid value
	{
		Any aAny = pXIndex->getByIndex(numberingLevel);
		Sequence< ::com::sun::star::beans::PropertyValue > aProps;
		aAny >>= aProps;
		const ::com::sun::star::beans::PropertyValue* pPropArray = aProps.getConstArray();
		sal_Int32 nCount = aProps.getLength();
		swprintf(pChar,L"Numbering:NumberingLevel=%d,",numberingLevel);
		for( sal_Int32 i=0; i<nCount; i++ )
		{
			::com::sun::star::beans::PropertyValue rProp = pPropArray[i];
			if(	(rProp.Name.compareTo(OUString::createFromAscii("BulletChar"))==0)||
				(rProp.Name.compareTo(OUString::createFromAscii("GraphicURL"))==0)||
				(rProp.Name.compareTo(OUString::createFromAscii("NumberingType"))==0))
			{
				OLECHAR propStr[512] = {NULL};
				swprintf(propStr,L"%s=",rProp.Name.getStr());
				OLECHAR pTemp[256] = {NULL};
				CMAccessible::get_OLECHARFromAny(rProp.Value,pTemp);
				if(rProp.Name.compareTo(OUString::createFromAscii("GraphicURL"))==0)
				{
					OLECHAR* pOccur = wcschr(pTemp,':');
					if(pOccur)
						*pOccur = '.';
				}
				wcscat(propStr,pTemp);
				wcscat(pChar,propStr);
				wcscat(pChar,L",");

				if(rProp.Name.compareTo(OUString::createFromAscii("NumberingType"))==0)
				{
					if(numberingPrefix.getLength()!=0)
					{
						swprintf(pTemp,L"NumberingPrefix=%s,",numberingPrefix.getStr());
						wcscat(pChar,pTemp);
					}
				}
			}
		}
	}

	//Because now have three types numbering level:
	//1.real numbering list,numbering level>=0 and numbering Rule !=NULL;
	//2.common paragraph, numbering level >=0, and numbering Rule == NULL;
	//3.TOC paragraph, numbering level >0, and numbering Rule ==NULL;
	// IAText:numberinglevel base on 0, but TOC's level base on 1,
	// so NumberingLevel value will be decreased 1 in bridge code.
	else if(numberingLevel >0)
	{
		swprintf(pChar,L"Numbering:NumberingLevel=%d,NumberingType=4,NumberingPrefix=,",numberingLevel-1);
	}
	else
	{
		swprintf(pChar,L"Numbering:");
	}
}

void CMAccessible::ConvertAnyToVariant(const ::com::sun::star::uno::Any &rAnyVal, VARIANT *pvData)
{
	if(rAnyVal.hasValue())
	{
		// Clear VARIANT variable.
		VariantClear(pvData);

		// Set value according to value type.
		switch(rAnyVal.getValueTypeClass())
		{
		case TypeClass_CHAR:
			pvData->vt = VT_UI1;
			memcpy(&pvData->bVal, rAnyVal.getValue(), sizeof(sal_Char));
			break;

		case TypeClass_BOOLEAN:
			pvData->vt = VT_BOOL;
			memcpy(&pvData->boolVal, rAnyVal.getValue(), sizeof(sal_Bool));
			break;

		case TypeClass_BYTE:
			pvData->vt = VT_UI1;
			memcpy(&pvData->bVal, rAnyVal.getValue(), sizeof(sal_Int8));
			break;

		case TypeClass_SHORT:
			pvData->vt = VT_I2;
			memcpy(&pvData->iVal, rAnyVal.getValue(), sizeof(sal_Int16));
			break;

		case TypeClass_UNSIGNED_SHORT:
			pvData->vt = VT_I2;
			memcpy(&pvData->iVal, rAnyVal.getValue(), sizeof(sal_uInt16));
			break;

		case TypeClass_LONG:
			pvData->vt = VT_I4;
			memcpy(&pvData->lVal, rAnyVal.getValue(), sizeof(sal_Int32));
			break;

		case TypeClass_UNSIGNED_LONG:
			pvData->vt = VT_I4;
			memcpy(&pvData->lVal, rAnyVal.getValue(), sizeof(sal_uInt32));
			break;

		case TypeClass_FLOAT:
			pvData->vt = VT_R4;
			memcpy(&pvData->fltVal, rAnyVal.getValue(), sizeof(float));
			break;

		case TypeClass_DOUBLE:
			pvData->vt = VT_R8;
			memcpy(&pvData->dblVal, rAnyVal.getValue(), sizeof(double));
			break;

		case TypeClass_STRING:
			{
				pvData->vt = VT_BSTR;
				::rtl::OUString val;
				rAnyVal >>= val;
				pvData->bstrVal = SysAllocString((OLECHAR *)val.getStr());
				break;
			}

		case TypeClass_VOID:
		case TypeClass_HYPER:
		case TypeClass_UNSIGNED_HYPER:
		case TypeClass_TYPE:
		case TypeClass_ANY:
		case TypeClass_ENUM:
		case TypeClass_TYPEDEF:
		case TypeClass_STRUCT:
		case TypeClass_UNION:
		case TypeClass_EXCEPTION:
		case TypeClass_SEQUENCE:
		case TypeClass_ARRAY:
		case TypeClass_INTERFACE:
			{
				Reference< XAccessible > pXAcc;
				if(rAnyVal >>= pXAcc)
				{
					if(pXAcc.is())
					{
						IAccessible* pIAcc = NULL;
						get_IAccessibleFromXAccessible((long)pXAcc.get(), &pIAcc);
						if(pIAcc == NULL)
						{
							Reference< XAccessibleContext > pXAccContext = pXAcc->getAccessibleContext();
							g_pAgent->InsertAccObj(pXAcc.get(),pXAccContext->getAccessibleParent().get());
							get_IAccessibleFromXAccessible((long)pXAcc.get(), &pIAcc);
						}
						if(pIAcc)
						{
							pIAcc->AddRef();

							pvData->vt = VT_UNKNOWN;
							pvData->pdispVal = (IAccessible2*)pIAcc;
							break;
						}
					}
				}
			}
		case TypeClass_SERVICE:
		case TypeClass_MODULE:
		case TypeClass_INTERFACE_METHOD:
		case TypeClass_INTERFACE_ATTRIBUTE:
		case TypeClass_UNKNOWN:
		case TypeClass_PROPERTY:
		case TypeClass_CONSTANT:
		case TypeClass_CONSTANTS:
		case TypeClass_SINGLETON:
		case TypeClass_MAKE_FIXED_SIZE:
			// Output the type string, if there is other uno value type.
			pvData->vt = VT_BSTR;
			pvData->bstrVal = SysAllocString(rAnyVal.getValueTypeName().getStr());
			break;

		default:
			break;
		}
	}
	else
	{
		VariantClear(pvData);
	}
}

STDMETHODIMP CMAccessible::Get_XAccChildID(long* childID)
{
	// #CHECK#
	if(childID == NULL)
	{
		return E_FAIL;
	}
	*childID = m_dChildID;
	return S_OK;
}
STDMETHODIMP CMAccessible:: get_states(AccessibleStates __RPC_FAR *states )
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK XInterface#
		if( !pRContext.is() )
			return E_FAIL;

	Reference<XAccessibleStateSet> pRStateSet = pRContext.get()->getAccessibleStateSet();
	if(!pRStateSet.is())
	{
		return S_OK;
	}
	Sequence<short> pStates = pRStateSet->getStates();


	long count = pStates.getLength() ;
	*states = 0x0;
	for( int i = 0; i < count; i++  )
	{
		for( int j = 0; j < sizeof(UNO_STATES) / sizeof(UNO_STATES[0]); j++ )
		{
			if( pStates[i] == UNO_STATES[j] )
			{
				*states |= IA2_STATES[j];
				break;
			}
		}
	}
	return S_OK;


	LEAVE_PROTECTED_BLOCK
}

// return the UNO roles
STDMETHODIMP CMAccessible:: get_extendedRole( BSTR __RPC_FAR *  )
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()

		return E_NOTIMPL;

	LEAVE_PROTECTED_BLOCK
}

STDMETHODIMP CMAccessible:: get_localizedExtendedRole( BSTR __RPC_FAR *  )
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		return E_NOTIMPL;

	LEAVE_PROTECTED_BLOCK
}
STDMETHODIMP CMAccessible:: get_nExtendedStates( long __RPC_FAR * )
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()

		return E_NOTIMPL;

	LEAVE_PROTECTED_BLOCK
}


STDMETHODIMP CMAccessible:: get_localizedExtendedStates( long, BSTR __RPC_FAR *__RPC_FAR *, long __RPC_FAR *)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		return E_NOTIMPL;

	LEAVE_PROTECTED_BLOCK
}


STDMETHODIMP CMAccessible:: get_indexInParent( long __RPC_FAR *accParentIndex)
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		// #CHECK#
		if(accParentIndex == NULL)
			return E_INVALIDARG;

	// #CHECK XInterface#
	if( !pRContext.is() )
		return E_FAIL;

	*accParentIndex = pRContext.get()->getAccessibleIndexInParent();
	return S_OK;


	LEAVE_PROTECTED_BLOCK
}
STDMETHODIMP CMAccessible:: get_locale( IA2Locale __RPC_FAR *locale  )
{

	CHECK_ENABLE_INF
		ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if(locale == NULL)
			return E_INVALIDARG;
	// #CHECK XInterface#

	if( !pRContext.is() )
		return E_FAIL;

	::com::sun::star::lang::Locale unoLoc = pRContext.get()->getLocale();
	locale->language = SysAllocString((OLECHAR*)unoLoc.Language.getStr());
	locale->country = SysAllocString((OLECHAR*)unoLoc.Country.getStr());
	locale->variant = SysAllocString((OLECHAR*)unoLoc.Variant.getStr());

	return S_OK;

	LEAVE_PROTECTED_BLOCK
}

DWORD GetMSAAStateFromUNO(short xState)
{
	DWORD IState = STATE_SYSTEM_UNAVAILABLE;
	switch( xState )
	{
	case /*AccessibleStateType::*/AccessibleStateType::BUSY:
		IState = STATE_SYSTEM_BUSY;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::CHECKED:
		IState = STATE_SYSTEM_CHECKED;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::DEFUNC:
		IState = STATE_SYSTEM_UNAVAILABLE;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::EXPANDED:
		IState = STATE_SYSTEM_EXPANDED;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::FOCUSABLE:
		IState = STATE_SYSTEM_FOCUSABLE;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::FOCUSED:
		IState = STATE_SYSTEM_FOCUSED;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::INDETERMINATE:
		IState = STATE_SYSTEM_MIXED;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::MULTI_SELECTABLE:
		IState = STATE_SYSTEM_MULTISELECTABLE;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::PRESSED:
		IState = STATE_SYSTEM_PRESSED;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::RESIZABLE:
		IState = STATE_SYSTEM_SIZEABLE;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::SELECTABLE:
		IState = STATE_SYSTEM_SELECTABLE;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::SELECTED:
		IState = STATE_SYSTEM_SELECTED;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::ARMED:
		IState = STATE_SYSTEM_FOCUSED;
		break;
	case /*AccessibleStateType::*/AccessibleStateType::EXPANDABLE:
		IState = STATE_SYSTEM_COLLAPSED;
		break;
	default:
		break;
	}
	return IState;
}

STDMETHODIMP CMAccessible:: get_appName( BSTR __RPC_FAR *name)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if(name == NULL)
			return E_INVALIDARG;

	*name = SysAllocString(OLESTR("Hannover"));
	return S_OK;
	LEAVE_PROTECTED_BLOCK
}
STDMETHODIMP CMAccessible:: get_appVersion(BSTR __RPC_FAR *version)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if(version == NULL)
			return E_INVALIDARG;
	*version=SysAllocString(OLESTR("3.0"));
	return S_OK;
	LEAVE_PROTECTED_BLOCK
}
STDMETHODIMP CMAccessible:: get_toolkitName(BSTR __RPC_FAR *name)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if(name == NULL)
			return E_INVALIDARG;
	*name = SysAllocString(OLESTR(" "));
	return S_OK;
	LEAVE_PROTECTED_BLOCK
}
STDMETHODIMP CMAccessible:: get_toolkitVersion(BSTR __RPC_FAR *version)
{

	ENTER_PROTECTED_BLOCK
		ISDESTROY()
		if(version == NULL)
			return E_INVALIDARG;
	*version = SysAllocString(OLESTR(" "));
	return S_OK;
	LEAVE_PROTECTED_BLOCK
}


STDMETHODIMP CMAccessible::get_attributes(/*[out]*/ BSTR *pAttr)
{
	ENTER_PROTECTED_BLOCK
	ISDESTROY()
	CHECK_ENABLE_INF
		Reference<XAccessibleContext> pRContext = pUNOInterface->getAccessibleContext();
	if( !pRContext.is() )
	{
		return E_FAIL;
	}
	Reference<XAccessibleExtendedAttributes> pRXI(pRContext,UNO_QUERY);
	if( !pRXI.is() )
		return E_FAIL;
	else
	{
		com::sun::star::uno::Reference<com::sun::star::accessibility::XAccessibleExtendedAttributes> pRXAttr;
		pRXAttr = pRXI.get();
		::com::sun::star::uno::Any	anyVal = pRXAttr->getExtendedAttributes();

		::rtl::OUString val;
		anyVal >>= val;

		if(*pAttr)
			SAFE_SYSFREESTRING(*pAttr);
		*pAttr = SysAllocString((OLECHAR *)val.getStr());

		return S_OK;
	}
	LEAVE_PROTECTED_BLOCK
}

