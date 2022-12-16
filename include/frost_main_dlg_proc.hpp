// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "resource.h"

#include <boost/foreach.hpp>

#include "FrostMXVersion.h"

#include "FrostStaticInterface.hpp"
#include "frost_gui_resources.hpp"
#include "frost_param_block_desc.hpp"
#include "utility.hpp"

class frost_main_dlg_proc : public ParamMap2UserDlgProc {
    Frost* m_obj;

  public:
    frost_main_dlg_proc()
        : m_obj( 0 ) {}
    frost_main_dlg_proc( Frost* obj )
        : m_obj( obj ) {}

    INT_PTR DlgProc( TimeValue /*t*/, IParamMap2* map, HWND /*hwnd*/, UINT msg, WPARAM wParam, LPARAM /*lParam*/ ) {
        try {
            if( !m_obj ) {
                throw std::runtime_error( "Error: class object is NULL" );
            }
            if( !map ) {
                throw std::runtime_error( "Error: parameter map is NULL" );
            }
            int id = LOWORD( wParam );
            // int notifyCode = HIWORD( wParam );
            switch( msg ) {
            case WM_INITDIALOG: {
                UpdateEnables();
            }
                return TRUE;
            case WM_COMMAND:
                switch( id ) {
                case BTN_FORCE_VIEWPORT_UPDATE:
                    m_obj->force_viewport_update();
                    break;
                }
                break;
            }
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("frost_main_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
        return FALSE;
    }

    void Update( TimeValue /*t*/ ) {
        try {
            UpdateEnables();
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("frost_main_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
    }

    void UpdateEnables() {
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
        IParamMap2* pm = pb->GetMap( frost_main_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }

        const bool enableWriteVelocityMapChannel = pb->GetInt( pb_writeVelocityMapChannel ) != 0;

        {
            const bool enable = enableWriteVelocityMapChannel;
            EnableWindow( GetDlgItem( hwnd, IDC_VELOCITY_MAP_CHANNEL_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_VELOCITY_MAP_CHANNEL_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_velocityMapChannel, enable );
        }
    }

    void SetThing( ReferenceTarget* obj ) { m_obj = (Frost*)obj; }

    void SetFrost( Frost* frost ) { m_obj = frost; }

    void InvalidateUI( HWND hwnd, int /*element*/ ) {
        if( !hwnd ) {
            return;
        }
    }

    void DeleteThis() {
        // static !
        // delete this;
    }
};
