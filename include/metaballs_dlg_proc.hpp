// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "Frost.hpp"
#include "resource.h"

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

#include <frantic/max3d/max_utility.hpp>

#include "frost_gui_resources.hpp"
#include "frost_param_block_desc.hpp"
#include "utility.hpp"

class metaballs_dlg_proc : public ParamMap2UserDlgProc {
    Frost* m_obj;

    std::vector<int> m_optionButtons;
    void init() {
        m_optionButtons.clear();
        m_optionButtons = { IDC_METABALL_RADIUS_SCALE_OPTIONS, IDC_METABALL_ISOSURFACE_LEVEL_OPTIONS };
    }

    void UpdateEnables() {
        if( !m_obj ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_metaballs_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }

        const bool inCreateMode = GetCOREInterface()->GetCommandPanelTaskMode() == TASK_MODE_CREATE;
        BOOST_FOREACH( int idc, m_optionButtons ) {
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, idc ), !inCreateMode );
        }
    }

  public:
    metaballs_dlg_proc()
        : m_obj( 0 ) {
        init();
    }

    metaballs_dlg_proc( Frost* obj )
        : m_obj( obj ) {
        init();
    }

    INT_PTR DlgProc( TimeValue t, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam, LPARAM /*lParam*/ ) {
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
                BOOST_FOREACH( int idc, m_optionButtons ) {
                    GetFrostGuiResources()->apply_custbutton_fast_forward_icon( GetDlgItem( hwnd, idc ) );
                    CustButton_SetRightClickNotify( GetDlgItem( hwnd, idc ), true );
                }
                UpdateEnables();
            } break;
            case WM_COMMAND:
                switch( id ) {
                case IDC_METABALL_RADIUS_SCALE_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_MetaballRadiusScaleOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    break;
                case IDC_METABALL_ISOSURFACE_LEVEL_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_MetaballIsosurfaceLevelOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    break;
                }
                break;
            }
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("metaballs_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
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
                _T("metaballs_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
    }

    void SetThing( ReferenceTarget* obj ) { m_obj = (Frost*)obj; }

    void SetFrost( Frost* frost ) { m_obj = frost; }

    void DeleteThis() {
        // static !
        /*delete this;*/
    }
};
