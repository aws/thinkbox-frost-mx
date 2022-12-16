// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frost_help_dlg_proc.hpp"

#include "resource.h"

#include <boost/foreach.hpp>

#include <Shellapi.h>

#include <frantic/max3d/ui/about_dialog.hpp>

#include "Frost.hpp"

#include "FrostMXVersion.h"

#include "FrostStaticInterface.hpp"
#include "attributions.hpp"
#include "frost_gui_resources.hpp"
#include "frost_param_block_desc.hpp"
#include "utility.hpp"

extern HINSTANCE ghInstance;

extern TCHAR* GetString( int id );

frost_help_dlg_proc::frost_help_dlg_proc()
    : m_obj( 0 ) {}
frost_help_dlg_proc::frost_help_dlg_proc( Frost* obj )
    : m_obj( obj ) {}

INT_PTR frost_help_dlg_proc::DlgProc( TimeValue t, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam,
                                      LPARAM /*lParam*/ ) {
    try {
        if( !m_obj ) {
            throw std::runtime_error( "Error: class object is NULL" );
        }
        if( !map ) {
            throw std::runtime_error( "Error: parameter map is NULL" );
        }
        int id = LOWORD( wParam );
        int notifyCode = HIWORD( wParam );
        switch( msg ) {
        case WM_INITDIALOG: {
            SetWindowText( GetDlgItem( map->GetHWnd(), LBL_VERSION ),
                           ( _T("Version: ") + frantic::strings::to_tstring( FRANTIC_VERSION ) ).c_str() );

            typedef std::pair<int, int> control_tooltip_t;
            control_tooltip_t buttonTooltips[] = { std::make_pair( IDC_ONLINE_HELP, IDS_ONLINE_HELP_TOOLTIP ) };
            BOOST_FOREACH( const control_tooltip_t& buttonTooltip, buttonTooltips ) {
                CustButton_SetTooltip( GetDlgItem( hwnd, buttonTooltip.first ), GetString( buttonTooltip.second ) );
            }

            int fastForwardButtons[] = { IDC_LOG_WINDOW_OPTIONS };
            BOOST_FOREACH( int idc, fastForwardButtons ) {
                GetFrostGuiResources()->apply_custbutton_fast_forward_icon( GetDlgItem( hwnd, idc ) );
            }
            int rightClickButtons[] = { IDC_LOG_WINDOW, IDC_LOG_WINDOW_OPTIONS };
            BOOST_FOREACH( int idc, rightClickButtons ) {
                CustButton_SetRightClickNotify( GetDlgItem( hwnd, idc ), true );
            }

            UpdateEnables();
        }
            return TRUE;
        case WM_COMMAND:
            switch( id ) {
            case IDC_ONLINE_HELP: {
                std::wstringstream versionSS;
                versionSS << FRANTIC_MAJOR_VERSION << L"." << FRANTIC_MINOR_VERSION;
                const frantic::tstring version = frantic::strings::to_tstring( versionSS.str() );
                const frantic::tstring url =
                    _T("http://docs.thinkboxsoftware.com/products/frost/") + version + _T("/1_Documentation/");
                HINSTANCE result = ShellExecute( NULL, _T("open"), url.c_str(), NULL, NULL, SW_SHOWNORMAL );
                if( int( result ) <= 32 ) {
                    throw std::runtime_error( "Unable to open online help." );
                }
            } break;
            case IDC_ABOUT:
                frantic::max3d::ui::show_about_dialog( ghInstance, _T("About Frost"), _T("Frost MX"),
                                                       _T( FRANTIC_VERSION ), get_attributions() );
                break;
            case IDC_LOG_WINDOW:
                if( notifyCode == BN_RIGHTCLICK ) {
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_LogWindowOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                } else {
                    GetFrostInterface()->SetLogWindowVisible( true );
                    GetFrostInterface()->FocusLogWindow();
                }
                break;
            case IDC_LOG_WINDOW_OPTIONS:
                notify_if_missing_frost_mxs();
                frantic::max3d::mxs::expression(
                    _T("if (FrostUi!=undefined) do (FrostUi.on_LogWindowOptions_pressed frostNode)") )
                    .bind( _T("frostNode"), m_obj )
                    .at_time( t )
                    .evaluate<void>();
                break;
            }
            break;
        }
    } catch( const std::exception& e ) {
        frantic::tstring errmsg =
            _T("frost_help_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        FF_LOG( error ) << errmsg << std::endl;
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return FALSE;
}

void frost_help_dlg_proc::Update( TimeValue /*t*/ ) {
    try {
        UpdateEnables();
    } catch( const std::exception& e ) {
        frantic::tstring errmsg =
            _T("frost_help_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        FF_LOG( error ) << errmsg << std::endl;
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

void frost_help_dlg_proc::UpdateEnables() {
    if( !m_obj ) {
        return;
    }
    if( m_obj != m_obj->editObj ) {
        return;
    }
    IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
    if( !pb ) {
        return;
    }
    IParamMap2* pm = pb->GetMap( frost_help_param_map );
    if( !pm ) {
        return;
    }
    const HWND hwnd = pm->GetHWnd();
    if( !hwnd ) {
        return;
    }
}

void frost_help_dlg_proc::SetThing( ReferenceTarget* obj ) { m_obj = (Frost*)obj; }

void frost_help_dlg_proc::SetFrost( Frost* frost ) { m_obj = frost; }

void frost_help_dlg_proc::InvalidateUI( HWND hwnd, int /*element*/ ) {
    if( !hwnd ) {
        return;
    }
}

void frost_help_dlg_proc::DeleteThis() {
    // static !
    // delete this;
}
