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

class meshing_quality_dlg_proc : public ParamMap2UserDlgProc {
    Frost* m_obj;

    std::vector<int> m_optionButtons;
    void init() {
        m_optionButtons.clear();
        m_optionButtons = { IDC_RENDER_MESHING_RESOLUTION_OPTIONS,
                            IDC_VIEWPORT_MESHING_RESOLUTION_OPTIONS,
                            IDC_RENDER_VOXEL_LENGTH_OPTIONS,
                            IDC_VIEWPORT_VOXEL_LENGTH_OPTIONS,
                            IDC_RENDER_VERT_REFINEMENT_ITERATIONS_OPTIONS,
                            IDC_VIEWPORT_VERT_REFINEMENT_ITERATIONS_OPTIONS };
    }

  public:
    meshing_quality_dlg_proc()
        : m_obj( 0 ) {
        init();
    }

    meshing_quality_dlg_proc( Frost* obj )
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
            IParamBlock2* pb = map->GetParamBlock();
            if( !pb ) {
                throw std::runtime_error( "ParamBlock2 is NULL" );
            }
            int id = LOWORD( wParam );
            // int notifyCode = HIWORD( wParam );
            switch( msg ) {
            case WM_INITDIALOG: {
                BOOST_FOREACH( int idc, m_optionButtons ) {
                    GetFrostGuiResources()->apply_custbutton_fast_forward_icon( GetDlgItem( hwnd, idc ) );
                    CustButton_SetRightClickNotify( GetDlgItem( hwnd, idc ), true );
                }
                GetFrostGuiResources()->apply_custbutton_down_arrow_icon(
                    GetDlgItem( hwnd, IDC_MESHING_RES_TO_VOXEL_LENGTH ) );
                GetFrostGuiResources()->apply_custbutton_up_arrow_icon(
                    GetDlgItem( hwnd, IDC_VOXEL_LENGTH_TO_MESHING_RES ) );
                typedef std::pair<int, int> control_tooltip_t;
                control_tooltip_t buttonTooltips[] = {
                    std::make_pair( IDC_MESHING_RES_TO_VOXEL_LENGTH, IDS_MESHING_RES_TO_VOXEL_LENGTH_TOOLTIP ),
                    std::make_pair( IDC_VOXEL_LENGTH_TO_MESHING_RES, IDS_VOXEL_LENGTH_TO_MESHING_RES_TOOLTIP ) };
                BOOST_FOREACH( const control_tooltip_t& buttonTooltip, buttonTooltips ) {
                    CustButton_SetTooltip( GetDlgItem( hwnd, buttonTooltip.first ), GetString( buttonTooltip.second ) );
                }
                UpdateEnables();
            }
                return TRUE;
            case WM_COMMAND:
                switch( id ) {
                case IDC_RENDER_MESHING_RESOLUTION_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_RenderMeshingResolutionOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case IDC_RENDER_VERT_REFINEMENT_ITERATIONS_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_RenderVertRefinementIterationsOptions_pressed ")
                        _T("frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case IDC_VIEWPORT_MESHING_RESOLUTION_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_ViewportMeshingResolutionOptions_pressed ")
                        _T("frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case IDC_VIEWPORT_VERT_REFINEMENT_ITERATIONS_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_ViewportVertRefinementIterationsOptions_pressed ")
                        _T("frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case IDC_MESHING_RES_TO_VOXEL_LENGTH: {
                    const float renderResolution = pb->GetFloat( pb_renderMeshingResolution, t );
                    const float viewportResolution = pb->GetFloat( pb_viewportMeshingResolution, t );
                    if( renderResolution > 0 && viewportResolution > 0 ) {
                        const float r = m_obj->GetMaxParticleRadius( t );
                        if( r > 0 ) {
                            if( !theHold.Holding() ) {
                                theHold.Begin();
                            }
                            pb->SetValue( pb_renderVoxelLength, t, r / renderResolution );
                            pb->SetValue( pb_viewportVoxelLength, t, r / viewportResolution );
                            if( m_obj->get_meshing_resolution_mode() != MESHING_RESOLUTION_MODE::VOXEL_LENGTH ) {
                                m_obj->set_meshing_resolution_mode( MESHING_RESOLUTION_MODE::VOXEL_LENGTH );
                            }
                            theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                        } else {
                            MessageBox( GetCOREInterface()->GetMAXHWnd(), _T("No particles to get radius from."),
                                        _T("Frost"), MB_OK | MB_ICONEXCLAMATION );
                        }
                    } else {
                        MessageBox( GetCOREInterface()->GetMAXHWnd(), _T("Resolution must be greater than zero."),
                                    _T("Frost"), MB_OK | MB_ICONEXCLAMATION );
                    }
                }
                    return TRUE;
                case IDC_VOXEL_LENGTH_TO_MESHING_RES: {
                    const float renderVoxelLength = pb->GetFloat( pb_renderVoxelLength, t );
                    const float viewportVoxelLength = pb->GetFloat( pb_viewportVoxelLength, t );
                    if( renderVoxelLength > 0 && viewportVoxelLength > 0 ) {
                        const float r = m_obj->GetMaxParticleRadius( t );
                        if( r > 0 ) {
                            if( !theHold.Holding() ) {
                                theHold.Begin();
                            }
                            pb->SetValue( pb_renderMeshingResolution, t, r / renderVoxelLength );
                            pb->SetValue( pb_viewportMeshingResolution, t, r / viewportVoxelLength );
                            if( m_obj->get_meshing_resolution_mode() !=
                                MESHING_RESOLUTION_MODE::SUBDIVIDE_MAX_RADIUS ) {
                                m_obj->set_meshing_resolution_mode( MESHING_RESOLUTION_MODE::SUBDIVIDE_MAX_RADIUS );
                            }
                            theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                        } else {
                            MessageBox( GetCOREInterface()->GetMAXHWnd(), _T("No particles to get radius from."),
                                        _T("Frost"), MB_OK | MB_ICONEXCLAMATION );
                        }
                    } else {
                        MessageBox( GetCOREInterface()->GetMAXHWnd(), _T("Voxel length must be greater than zero."),
                                    _T("Frost"), MB_OK | MB_ICONEXCLAMATION );
                    }
                }
                    return TRUE;
                case IDC_RENDER_VOXEL_LENGTH_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_RenderVoxelLengthOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case IDC_VIEWPORT_VOXEL_LENGTH_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_ViewportVoxelLengthOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                }
                break;
            }
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("meshing_quality_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
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
                _T("meshing_quality_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
    }

    void UpdateEnables( /*HWND hwnd*/ ) {
        if( !m_obj ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_meshing_quality_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }

        const bool useViewportSettings = pb->GetInt( pb_renderUsingViewportSettings ) != 0;
        const bool useMeshingResolution =
            pb->GetInt( pb_meshingResolutionMode ) == MESHING_RESOLUTION_MODE::SUBDIVIDE_MAX_RADIUS;
        const bool useVoxelLength = !useMeshingResolution;
        const bool inCreateMode = GetCOREInterface()->GetCommandPanelTaskMode() == TASK_MODE_CREATE;

        BOOST_FOREACH( int idc, m_optionButtons ) {
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, idc ), !inCreateMode );
        }

        // meshing resolution labels
        {
            const bool enable = useMeshingResolution;

            EnableWindow( GetDlgItem( hwnd, IDC_RENDER_MESHING_RES_STATIC ), enable );
            EnableWindow( GetDlgItem( hwnd, IDC_VIEWPORT_MESHING_RES_STATIC ), enable );
        }
        // voxel length labels
        {
            const bool enable = useVoxelLength;

            EnableWindow( GetDlgItem( hwnd, IDC_RENDER_VOXEL_LENGTH_STATIC ), enable );
            EnableWindow( GetDlgItem( hwnd, IDC_VIEWPORT_VOXEL_LENGTH_STATIC ), enable );
        }
        // render enable all controls
        {
            const bool enable = !useViewportSettings;

            EnableWindow( GetDlgItem( hwnd, IDC_RENDER_MESHING_RES_STATIC ), enable && useMeshingResolution );
            pm->Enable( pb_renderMeshingResolution, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_RENDER_MESHING_RESOLUTION_OPTIONS ),
                                              enable && !inCreateMode );

            EnableWindow( GetDlgItem( hwnd, IDC_RENDER_VOXEL_LENGTH_STATIC ), enable && useVoxelLength );
            pm->Enable( pb_renderVoxelLength, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_RENDER_VOXEL_LENGTH_OPTIONS ),
                                              enable && !inCreateMode );

            EnableWindow( GetDlgItem( hwnd, IDC_ENABLE_RENDER_VERT_REFINE ), enable );

            EnableWindow( GetDlgItem( hwnd, IDC_RENDER_VERT_REFINE_STATIC ), enable );
            pm->Enable( pb_renderVertRefinementIterations, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_RENDER_VERT_REFINEMENT_ITERATIONS_OPTIONS ),
                                              enable && !inCreateMode );
        }
        // resolution -> voxel length button
        {
            const bool enable = useMeshingResolution;

            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_MESHING_RES_TO_VOXEL_LENGTH ), enable );
        }
        // voxel length -> resolution button
        {
            const bool enable = useVoxelLength;

            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_VOXEL_LENGTH_TO_MESHING_RES ), enable );
        }
    }

    void SetThing( ReferenceTarget* obj ) { m_obj = (Frost*)obj; }

    void SetFrost( Frost* frost ) { m_obj = frost; }

    void InvalidateUI( HWND /*hwnd*/, int /*element*/ ) {}

    void DeleteThis() {
        // static !
        /*delete this;*/
    }
};
