// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "resource.h"

#include <boost/foreach.hpp>
#include <frantic/max3d/max_utility.hpp>
#include <frantic/win32/utility.hpp>

#include "frost_gui_resources.hpp"

#include "frost_fixed_combo_box.hpp"

class material_dlg_proc : public ParamMap2UserDlgProc {
    Frost* m_obj;
    HWND m_hwnd;

    COLORREF m_materialModeIconColor;

    std::vector<int> m_materialModeIndexToCode;

    void reset() {
        m_materialModeIconColor = RGB( 0, 0, 0 );
        m_materialModeIndexToCode.clear();
    }

  public:
    material_dlg_proc()
        : m_obj( 0 )
        , m_hwnd( 0 ) {
        reset();
    }
    material_dlg_proc( Frost* obj )
        : m_obj( obj )
        , m_hwnd( 0 ) {
        reset();
    }

    INT_PTR DlgProc( TimeValue t, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam, LPARAM /*lParam*/ ) {
        m_hwnd = hwnd;
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
                frost_fixed_combo_box materialModeControl( hwnd, IDC_MATERIAL_MODE, Frost::get_material_mode_codes,
                                                           Frost::get_material_mode_name, m_materialModeIndexToCode );
                materialModeControl.reset_strings();
                materialModeControl.set_cur_sel_code( m_obj->get_material_mode() );

                typedef std::pair<int, int> control_tooltip_t;
                control_tooltip_t buttonTooltips[] = {
                    std::make_pair( IDC_CREATE_MATERIAL, IDS_CREATE_MATERIAL_TOOLTIP ) };
                BOOST_FOREACH( const control_tooltip_t& buttonTooltip, buttonTooltips ) {
                    CustButton_SetTooltip( GetDlgItem( hwnd, buttonTooltip.first ), GetString( buttonTooltip.second ) );
                }

                UpdateEnables( hwnd );

                UpdateStatusMessages( hwnd, t );
            } break;
            case WM_COMMAND:
                switch( id ) {
                case IDC_MATERIAL_MODE:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box materialModeControl(
                            hwnd, IDC_MATERIAL_MODE, Frost::get_material_mode_codes, Frost::get_material_mode_name,
                            m_materialModeIndexToCode );
                        int sel = materialModeControl.get_cur_sel_code();
                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_material_mode( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                    }
                    break;
                case IDC_CREATE_MATERIAL: {
                    INode* frostNode = m_obj->get_inode();
                    if( frostNode ) {
                        notify_if_missing_frost_mxs();
                        frantic::max3d::mxs::expression(
                            _T("if (FrostUi!=undefined) do (FrostUi.on_CreateMaterial_pressed frostNode)") )
                            .bind( _T("frostNode"), frostNode )
                            .at_time( t )
                            .evaluate<void>();
                        m_obj->invalidate_frost();
                        m_obj->maybe_invalidate( t );
                    } else {
                        const frantic::tstring title = _T("Frost");
                        const frantic::tstring msg = _T("Unable to find Frost node.");
                        MessageBox( GetCOREInterface()->GetMAXHWnd(), msg.c_str(), title.c_str(),
                                    MB_OK + MB_ICONEXCLAMATION );
                    }
                } break;
                }
                break;
            }
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("material_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
        return FALSE;
    }

    void Update( TimeValue t ) {
        try {
            UpdateEnables( m_hwnd );
            UpdateStatusMessages( m_hwnd, t );
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("material_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
    }

    void UpdateEnables( HWND hwnd ) {
        if( !m_obj ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_material_param_map );
        if( !pm ) {
            return;
        }
        if( !hwnd ) {
            return;
        }

        const bool hasMaterial = pb->GetInt( pb_materialMode ) == MATERIAL_MODE::MATERIAL_FROM_GEOMETRY &&
                                 pb->GetInt( pb_meshingMethod ) == MESHING_METHOD::GEOMETRY;

        {
            const bool enable = hasMaterial;
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_CREATE_MATERIAL ), enable );
        }
    }

    void UpdateStatusMessages( HWND /*hwnd*/, TimeValue t ) {
        if( !m_obj ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* map = pb->GetMap( frost_material_param_map );
        if( !map ) {
            return;
        }
        const HWND hwnd = map->GetHWnd();
        if( !hwnd ) {
            return;
        }

        if( m_obj->editObj == m_obj ) {
            {
                frost_status_code_t status = m_obj->get_parameter_status_code( pb_materialMode, t );
                HWND hwndIcon = GetDlgItem( hwnd, IDC_MATERIAL_STATUS_ICON );
                HWND hwndText = GetDlgItem( hwnd, IDC_MATERIAL_STATUS_TEXT );
                int weight = m_obj->get_status_code_weight( status );
                if( weight > 0 && map->GetParamBlock() ) {
                    // show
                    ShowWindow( hwndIcon, SW_SHOW );
                    frantic::max3d::EnableCustStatus( hwndText, TRUE );
                } else {
                    // hide
                    ShowWindow( hwndIcon, SW_HIDE );
                    frantic::max3d::EnableCustStatus( hwndText, FALSE );
                }
                const frantic::tstring msg = m_obj->get_status_code_string( status );
                frantic::max3d::SetCustStatusText( hwndText, msg );
                GetFrostGuiResources()->apply_custimage_lamp_icon( hwndIcon, m_obj->get_status_code_lamp( status ) );
            }
        }
    }

    void SetThing( ReferenceTarget* obj ) {
        reset();
        m_obj = (Frost*)obj;
    }

    void SetFrost( Frost* frost ) {
        reset();
        m_obj = frost;
        m_hwnd = 0;
    }

    void InvalidateUI( HWND hwnd, int element ) {
        if( !hwnd ) {
            return;
        }
        switch( element ) {
        case pb_materialMode: {
            if( m_obj && m_obj->editObj == m_obj ) {
                const int materialMode = m_obj->get_material_mode();
                frost_fixed_combo_box materialModeControl( hwnd, IDC_MATERIAL_MODE, Frost::get_material_mode_codes,
                                                           Frost::get_material_mode_name, m_materialModeIndexToCode );
                materialModeControl.set_cur_sel_code( materialMode );
            }
        } break;
        }
    }

    void DeleteThis() {
        // static !
        // delete this;
    }
};
