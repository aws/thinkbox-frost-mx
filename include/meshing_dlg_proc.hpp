// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "resource.h"

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

#include <frantic/max3d/max_utility.hpp>
#include <frantic/win32/utility.hpp>

#include "frost_gui_resources.hpp"
#include "frost_param_block_desc.hpp"

#include "frost_fixed_combo_box.hpp"
#include "utility.hpp"

class meshing_dlg_proc : public ParamMap2UserDlgProc {
    Frost* m_obj;
    HWND m_hwnd;

    COLORREF m_radiusIconColor;
    COLORREF m_idIconColor;

    std::vector<int> m_meshingMethodIndexToCode;
    std::vector<int> m_viewportLoadIndexToCode;
    std::vector<int> m_motionBlurIndexToCode;
    std::vector<int> m_radiusAnimationModeIndexToCode;

    std::vector<int> m_optionButtons;
    void init() {
        m_optionButtons.clear();
        m_optionButtons = {
            IDC_VIEWPORT_PERCENT_OPTIONS,   IDC_RADIUS_OPTIONS,       IDC_RADIUS_RANDOM_VARIATION_OPTIONS,
            IDC_RADIUS_RANDOM_SEED_OPTIONS, IDC_RADIUS_SCALE_OPTIONS, IDC_RADIUS_ANIMATION_TOOLS };
    }

    void reset() {
        m_radiusIconColor = RGB( 0, 0, 0 );
        m_idIconColor = RGB( 0, 0, 0 );
        m_meshingMethodIndexToCode.clear();
        m_viewportLoadIndexToCode.clear();
        m_motionBlurIndexToCode.clear();
        m_radiusAnimationModeIndexToCode.clear();
    }

  public:
    meshing_dlg_proc()
        : m_obj( 0 )
        , m_hwnd( 0 ) {
        init();
        reset();
    }
    meshing_dlg_proc( Frost* obj )
        : m_obj( obj )
        , m_hwnd( 0 ) {
        init();
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
                frost_fixed_combo_box meshingMethodControl( hwnd, IDC_MESHING_METHOD, Frost::get_meshing_method_codes,
                                                            Frost::get_meshing_method_name,
                                                            m_meshingMethodIndexToCode );
                meshingMethodControl.reset_strings();
                meshingMethodControl.set_cur_sel_code( m_obj->get_meshing_method() );

                frost_fixed_combo_box radiusAnimationControl(
                    hwnd, IDC_RADIUS_ANIMATION_MODE, Frost::get_radius_animation_mode_codes,
                    Frost::get_radius_animation_mode_name, m_radiusAnimationModeIndexToCode );
                radiusAnimationControl.reset_strings();
                radiusAnimationControl.set_cur_sel_code( m_obj->get_radius_animation_mode() );

                frost_fixed_combo_box viewportLoadModeControl(
                    hwnd, IDC_VIEWPORT_LOAD_MODE, Frost::get_viewport_load_mode_codes,
                    Frost::get_viewport_load_mode_name, m_viewportLoadIndexToCode );
                viewportLoadModeControl.reset_strings();
                viewportLoadModeControl.set_cur_sel_code( m_obj->get_viewport_load_mode() );

                frost_fixed_combo_box motionBlurControl( hwnd, IDC_MOTION_BLUR_MODE, Frost::get_motion_blur_codes,
                                                         Frost::get_motion_blur_name, m_motionBlurIndexToCode );
                motionBlurControl.reset_strings();
                motionBlurControl.set_cur_sel_code( m_obj->get_motion_blur_mode() );

                UpdateEnables( hwnd );

                BOOST_FOREACH( int idc, m_optionButtons ) {
                    GetFrostGuiResources()->apply_custbutton_fast_forward_icon( GetDlgItem( hwnd, idc ) );
                    CustButton_SetRightClickNotify( GetDlgItem( hwnd, idc ), true );
                }

                UpdateStatusMessages( hwnd, t );
            } break;
            case WM_COMMAND:
                switch( id ) {
                case IDC_MESHING_METHOD:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box meshingMethodControl(
                            hwnd, IDC_MESHING_METHOD, Frost::get_meshing_method_codes, Frost::get_meshing_method_name,
                            m_meshingMethodIndexToCode );
                        int sel = meshingMethodControl.get_cur_sel_code();
                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_meshing_method( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                    }
                    break;
                case IDC_VIEWPORT_LOAD_MODE:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box viewportLoadModeControl(
                            hwnd, IDC_VIEWPORT_LOAD_MODE, Frost::get_viewport_load_mode_codes,
                            Frost::get_viewport_load_mode_name, m_viewportLoadIndexToCode );
                        int sel = viewportLoadModeControl.get_cur_sel_code();
                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_viewport_load_mode( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                    }
                    break;
                case IDC_VIEWPORT_PERCENT_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_ViewportLoadPercentOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    break;
                case IDC_RADIUS_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_RadiusOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    break;
                case IDC_RADIUS_RANDOM_VARIATION_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_RadiusRandomVariationOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    break;
                case IDC_RADIUS_RANDOM_SEED_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_RadiusRandomSeedOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    break;
                case IDC_RADIUS_ANIMATION_TOOLS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_RadiusScaleAnimationTools_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    break;
                case IDC_RADIUS_SCALE_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_RadiusScaleOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    break;
                case IDC_RADIUS_ANIMATION_MODE:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box control(
                            hwnd, IDC_RADIUS_ANIMATION_MODE, Frost::get_radius_animation_mode_codes,
                            Frost::get_radius_animation_mode_name, m_radiusAnimationModeIndexToCode );
                        int sel = control.get_cur_sel_code();
                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_radius_animation_mode( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                    }
                    break;
                case IDC_MOTION_BLUR_MODE:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box motionBlurControl( hwnd, IDC_MOTION_BLUR_MODE,
                                                                 Frost::get_motion_blur_codes,
                                                                 Frost::get_motion_blur_name, m_motionBlurIndexToCode );
                        int sel = motionBlurControl.get_cur_sel_code();
                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_motion_blur_mode( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                    }
                    break;
                }
                break;
            }
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("meshing_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
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
                _T("meshing_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
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
        IParamMap2* pm = pb->GetMap( frost_meshing_param_map );
        if( !pm ) {
            return;
        }
        if( !hwnd ) {
            return;
        }

        const bool randomizeRadius = pb->GetInt( pb_randomizeRadius ) != 0;
        const bool enableRadiusScale = pb->GetInt( pb_enableRadiusScale ) != 0;
        const bool inCreateMode = GetCOREInterface()->GetCommandPanelTaskMode() == TASK_MODE_CREATE;

        BOOST_FOREACH( int idc, m_optionButtons ) {
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, idc ), !inCreateMode );
        }

        // radius
        {
            const bool enable = true;
            EnableWindow( GetDlgItem( hwnd, IDC_RADIUS_STATIC ), enable );
            pm->Enable( pb_radius, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_RADIUS_OPTIONS ), enable && !inCreateMode );
        }

        // use radius channel
        {
            const bool enable = true;
            pm->Enable( pb_useRadiusChannel, enable );
        }

        // randomize
        {
            const bool enable = true;
            pm->Enable( pb_randomizeRadius, enable );
        }

        // randomize params
        {
            const bool enable = randomizeRadius;

            EnableWindow( GetDlgItem( hwnd, IDC_RADIUS_RANDOM_VARIATION_STATIC ), enable );
            pm->Enable( pb_radiusRandomVariation, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_RADIUS_RANDOM_VARIATION_OPTIONS ),
                                              enable && !inCreateMode );

            EnableWindow( GetDlgItem( hwnd, IDC_RADIUS_RANDOM_SEED_STATIC ), enable );
            pm->Enable( pb_radiusSeed, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_RADIUS_RANDOM_SEED_OPTIONS ),
                                              enable && !inCreateMode );
        }

        // radius scale
        {
            const bool enable = enableRadiusScale;

            pm->Enable( pb_radiusScale, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_RADIUS_SCALE_OPTIONS ), enable && !inCreateMode );

            EnableWindow( GetDlgItem( hwnd, IDC_RADIUS_ANIMATION_MODE_STATIC ), enable );
            EnableWindow( GetDlgItem( hwnd, IDC_RADIUS_ANIMATION_MODE ), enable );
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
        IParamMap2* map = pb->GetMap( frost_meshing_param_map );
        if( !map ) {
            return;
        }
        const HWND hwnd = map->GetHWnd();
        if( !hwnd ) {
            return;
        }

        if( m_obj->editObj == m_obj ) {
            {
                frost_status_code_t status = m_obj->get_parameter_status_code( pb_useRadiusChannel, t );
                HWND hwndIcon = GetDlgItem( hwnd, IDC_RADIUS_CHANNEL_STATUS_ICON );
                HWND hwndText = GetDlgItem( hwnd, IDC_RADIUS_CHANNEL_STATUS_TEXT );
                int weight = m_obj->get_status_code_weight( status );
                if( weight > 0 && map->GetParamBlock() &&
                    ( map->GetParamBlock()->GetInt( pb_useRadiusChannel ) != 0 ) ) {
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

            {
                frost_status_code_t status = m_obj->get_parameter_status_code( pb_radiusRandomVariation, t );
                HWND hwndIcon = GetDlgItem( hwnd, IDC_ID_CHANNEL_STATUS_ICON );
                HWND hwndText = GetDlgItem( hwnd, IDC_ID_CHANNEL_STATUS_TEXT );
                int weight = m_obj->get_status_code_weight( status );
                if( weight > 0 && map->GetParamBlock() &&
                    ( map->GetParamBlock()->GetInt( pb_randomizeRadius ) != 0 ) ) {
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

            {
                frost_status_code_t status = m_obj->get_parameter_status_code( pb_radiusAnimationMode, t );
                HWND hwndIcon = GetDlgItem( hwnd, IDC_RADIUS_ANIMATION_MODE_STATUS_ICON );
                HWND hwndText = GetDlgItem( hwnd, IDC_RADIUS_ANIMATION_MODE_STATUS_TEXT );
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
        case pb_meshingMethod: {
            if( m_obj && m_obj->editObj == m_obj ) {
                const int meshingMethod = m_obj->get_meshing_method();
                frost_fixed_combo_box meshingMethodControl( hwnd, IDC_MESHING_METHOD, Frost::get_meshing_method_codes,
                                                            Frost::get_meshing_method_name,
                                                            m_meshingMethodIndexToCode );
                meshingMethodControl.set_cur_sel_code( meshingMethod );
                m_obj->update_ui_to_meshing_method( -1 );
            }
        } break;
        case pb_viewportLoadMode: {
            if( m_obj && m_obj->editObj == m_obj ) {
                const int viewportLoadMode = m_obj->get_viewport_load_mode();
                frost_fixed_combo_box viewportLoadModeControl(
                    hwnd, IDC_VIEWPORT_LOAD_MODE, Frost::get_viewport_load_mode_codes,
                    Frost::get_viewport_load_mode_name, m_viewportLoadIndexToCode );
                viewportLoadModeControl.set_cur_sel_code( viewportLoadMode );
            }
        } break;
        case pb_radiusAnimationMode: {
            if( m_obj && m_obj->editObj == m_obj ) {
                const int radiusAnimationMode = m_obj->get_radius_animation_mode();
                frost_fixed_combo_box control( hwnd, IDC_RADIUS_ANIMATION_MODE, Frost::get_radius_animation_mode_codes,
                                               Frost::get_radius_animation_mode_name,
                                               m_radiusAnimationModeIndexToCode );
                control.set_cur_sel_code( radiusAnimationMode );
            }
        } break;
        case pb_motionBlurMode: {
            if( m_obj && m_obj->editObj == m_obj ) {
                const int motionBlurMode = m_obj->get_motion_blur_mode();
                frost_fixed_combo_box motionBlurControl( hwnd, IDC_MOTION_BLUR_MODE, Frost::get_motion_blur_codes,
                                                         Frost::get_motion_blur_name, m_motionBlurIndexToCode );
                motionBlurControl.set_cur_sel_code( motionBlurMode );
            }
        } break;
        }
    }

    void DeleteThis() {
        // static !
        // delete this;
    }
};
