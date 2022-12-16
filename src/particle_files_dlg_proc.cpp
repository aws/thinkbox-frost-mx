// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "resource.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/array.hpp>
#include <boost/foreach.hpp>

#include <frantic/max3d/max_utility.hpp>
#include <frantic/max3d/maxscript/maxscript.hpp>

#include "frost_fixed_combo_box.hpp"
#include "frost_gui_resources.hpp"
#include "frost_param_block_desc.hpp"
#include "handle_listbox_contextmenu.hpp"
#include "particle_files_dlg_proc.hpp"
#include "utility.hpp"

extern TCHAR* GetString( int id );

frantic::tstring get_particle_filename( HWND hwnd ) {
    boost::array<TCHAR, 2 + MAX_PATH> filename;
    filename.assign( 0 );
    OPENFILENAME ofn;
    memset( &ofn, 0, sizeof( OPENFILENAME ) );
    ofn.lStructSize = sizeof( OPENFILENAME );
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = _T("All Particle Files\0*.prt;*.bin*.csv\0Frantic Particle Files (*.prt)\0*.prt\0RealFlow Files ")
                      _T("(*.bin)\0*.bin\0CSV Files (*.csv)\0*.csv\0All(*.*)\0*.*\0\0");
    ofn.lpstrFile = &filename[0];
    ofn.nMaxFile = static_cast<DWORD>( filename.size() );
    ofn.lpstrTitle = _T("Select the Particle File Sequence");
    ofn.Flags = OFN_FILEMUSTEXIST;
    BOOL gotFilename = GetOpenFileName( &ofn );
    if( gotFilename ) {
        return frantic::tstring( filename.begin(), filename.end() );
    } else {
        return _T("");
    }
}

Value* particle_files_dlg_proc::get_sel_indices( HWND hwnd ) {
    std::vector<int> selIndices;

    HWND hwndListBox = GetDlgItem( hwnd, LB_PARTICLE_FILES );
    if( hwndListBox ) {
        int selCount = frantic::win32::ffListBox_GetSelCount( hwndListBox );
        if( selCount > 0 ) {
            std::vector<int> tempIndices;
            frantic::win32::ffListBox_GetSelItems( hwndListBox, tempIndices );
            std::sort( tempIndices.begin(), tempIndices.end() );
            for( std::size_t i = 0; i < tempIndices.size(); ++i ) {
                selIndices.push_back( 1 + tempIndices[i] );
            }
        }
    }
    return frantic::max3d::fpwrapper::MaxTypeTraits<std::vector<int>>::to_max_type( selIndices );
}

particle_files_dlg_proc::particle_files_dlg_proc()
    : m_obj( 0 )
    , m_hwnd( 0 ) {
    init();
}

particle_files_dlg_proc::particle_files_dlg_proc( Frost* obj )
    : m_obj( obj )
    , m_hwnd( 0 ) {
    init();
}

void particle_files_dlg_proc::update_particle_files_view( HWND hwnd ) {
    if( m_obj && m_obj->editObj == m_obj ) {
        HWND hwndCtrl = GetDlgItem( hwnd, LB_PARTICLE_FILES );
        if( hwndCtrl ) {
            std::vector<frantic::tstring> fileNames;
            m_obj->get_particle_file_display_names( fileNames );
            frantic::win32::ffListBox_SetStrings( hwndCtrl, fileNames );
        }
    }
}

INT_PTR particle_files_dlg_proc::DlgProc( TimeValue t, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam,
                                          LPARAM lParam ) {
    try {
        if( !map ) {
            throw std::runtime_error( "Error: parameter map is NULL" );
        }

        m_hwnd = hwnd;

        switch( msg ) {
        case WM_INITDIALOG: {
            typedef std::pair<int, int> control_tooltip_t;
            control_tooltip_t buttonTooltips[] = {
                std::make_pair( BTN_PARTICLE_FILES_ADD, IDS_PARTICLE_FILES_ADD_TOOLTIP ),
                std::make_pair( BTN_PARTICLE_FILES_REMOVE, IDS_PARTICLE_FILES_REMOVE_TOOLTIP ),
                std::make_pair( BTN_PARTICLE_FILES_EDIT, IDS_PARTICLE_FILES_EDIT_TOOLTIP ),
                std::make_pair( IDC_RELOAD_SOURCE_SEQUENCES, IDS_RELOAD_SOURCE_SEQUENCES_TOOLTIP ),
                std::make_pair( IDC_SET_USING_EXISTING_FRAMES, IDS_SET_USING_EXISTING_FRAMES_TOOLTIP ) };
            BOOST_FOREACH( const control_tooltip_t& buttonTooltip, buttonTooltips ) {
                CustButton_SetTooltip( GetDlgItem( hwnd, buttonTooltip.first ), GetString( buttonTooltip.second ) );
            }

            Update( t );
            update_particle_files_view( hwnd );

            frost_fixed_combo_box fileUnitControl( hwnd, IDC_PARTICLE_FILES_UNITS,
                                                   Frost::get_file_particle_scale_factor_codes,
                                                   Frost::get_file_length_unit_name, m_fileUnitIndexToCode );
            fileUnitControl.reset_strings();
            fileUnitControl.set_cur_sel_code( m_obj->get_file_length_unit() );

            frost_fixed_combo_box beforeRangeControl( hwnd, IDC_PARTICLE_FILES_BEFORE_RANGE_BEHAVIOR,
                                                      Frost::get_before_range_behavior_codes,
                                                      Frost::get_before_range_behavior_name, m_beforeRangeIndexToCode );
            beforeRangeControl.reset_strings();
            beforeRangeControl.set_cur_sel_code( m_obj->get_before_range_behavior() );

            frost_fixed_combo_box afterRangeControl( hwnd, IDC_PARTICLE_FILES_AFTER_RANGE_BEHAVIOR,
                                                     Frost::get_after_range_behavior_codes,
                                                     Frost::get_after_range_behavior_name, m_afterRangeIndexToCode );
            afterRangeControl.reset_strings();
            afterRangeControl.set_cur_sel_code( m_obj->get_after_range_behavior() );

            BOOST_FOREACH( int idc, m_optionButtons ) {
                GetFrostGuiResources()->apply_custbutton_fast_forward_icon( GetDlgItem( hwnd, idc ) );
                CustButton_SetRightClickNotify( GetDlgItem( hwnd, idc ), true );
            }

            m_inplaceTooltip.attach( GetDlgItem( hwnd, LB_PARTICLE_FILES ) );

            return TRUE;
        } break;
        case WM_DESTROY:
            m_inplaceTooltip.detach( GetDlgItem( hwnd, LB_PARTICLE_FILES ) );
            break;
        case WM_CONTEXTMENU:
            if( handle_listbox_contextmenu( GetDlgItem( hwnd, LB_PARTICLE_FILES ), lParam ) ) {
                UpdateEnables();
                notify_if_missing_frost_mxs();
                frantic::max3d::mxs::expression(
                    _T("if (FrostUi!=undefined) do (FrostUi.on_FileList_rightClicked frostNode)") )
                    .bind( _T("frostNode"), m_obj )
                    .at_time( t )
                    .evaluate<void>();
            }
            break;
        case WM_COMMAND:
            int ctrlID = LOWORD( wParam );
            int notifyCode = HIWORD( wParam );

            switch( ctrlID ) {
            case BTN_PARTICLE_FILES_ADD: {
                notify_if_missing_frost_mxs();
                frantic::max3d::mxs::expression(
                    _T("if (FrostUi!=undefined) do (FrostUi.on_addFile_pressed frostNode)") )
                    .bind( _T("frostNode"), m_obj )
                    .at_time( t )
                    .evaluate<void>();
            } break;
            case BTN_PARTICLE_FILES_REMOVE:
                // needs to be disabled when no file is selected
                {
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_removeFile_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                }
                break;
            case BTN_PARTICLE_FILES_EDIT: {
                notify_if_missing_frost_mxs();
                frantic::max3d::mxs::expression(
                    _T("if (FrostUi!=undefined) do (FrostUi.on_editFile_pressed frostNode)") )
                    .bind( _T("frostNode"), m_obj )
                    .at_time( t )
                    .evaluate<void>();
            } break;
            case LB_PARTICLE_FILES:
                switch( notifyCode ) {
                case LBN_SELCHANGE:
                    UpdateEnables();
                    break;
                case LBN_SELCANCEL:
                    UpdateEnables();
                    break;
                case LBN_DBLCLK: {
                    HWND hwndListBox = GetDlgItem( hwnd, LB_PARTICLE_FILES );
                    if( hwndListBox ) {
                        int selCount = ListBox_GetSelCount( hwndListBox );
                        if( selCount == 1 ) {
                            int selection = 0;
                            selCount = ListBox_GetSelItems( hwndListBox, 1, &selection );
                            if( selCount == 1 ) {
                                notify_if_missing_frost_mxs();
                                frantic::max3d::mxs::expression(
                                    _T("if (FrostUi!=undefined) do (FrostUi.on_fileList_doubleClicked frostNode itm)") )
                                    .bind( _T("frostNode"), m_obj )
                                    .bind( _T("itm"), Integer::heap_intern( 1 + selection ) )
                                    .at_time( t )
                                    .evaluate<void>();
                            }
                        }
                    }
                } break;
                }
                break;
            case IDC_SET_USING_EXISTING_FRAMES:
                m_obj->SetToValidFrameRange( t );
                break;
            case IDC_RELOAD_SOURCE_SEQUENCES:
                m_obj->reload_particle_files( t );
                m_obj->force_invalidate();
                break;
            case IDC_PARTICLE_FILES_BEFORE_RANGE_BEHAVIOR:
                if( notifyCode == CBN_SELCHANGE ) {
                    frost_fixed_combo_box beforeRangeControl(
                        hwnd, IDC_PARTICLE_FILES_BEFORE_RANGE_BEHAVIOR, Frost::get_before_range_behavior_codes,
                        Frost::get_before_range_behavior_name, m_beforeRangeIndexToCode );
                    int sel = beforeRangeControl.get_cur_sel_code();
                    if( !theHold.Holding() ) {
                        theHold.Begin();
                    }
                    m_obj->set_before_range_behavior( (int)sel );
                    theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                }
                break;
            case IDC_PARTICLE_FILES_AFTER_RANGE_BEHAVIOR:
                if( notifyCode == CBN_SELCHANGE ) {
                    frost_fixed_combo_box afterRangeControl(
                        hwnd, IDC_PARTICLE_FILES_AFTER_RANGE_BEHAVIOR, Frost::get_after_range_behavior_codes,
                        Frost::get_after_range_behavior_name, m_afterRangeIndexToCode );
                    int code = afterRangeControl.get_cur_sel_code();
                    if( !theHold.Holding() ) {
                        theHold.Begin();
                    }
                    m_obj->set_after_range_behavior( (int)code );
                    theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                }
                break;
            case IDC_PARTICLE_FILES_UNITS:
                if( notifyCode == CBN_SELCHANGE ) {
                    HWND hwndFileUnits = GetDlgItem( hwnd, IDC_PARTICLE_FILES_UNITS );
                    if( hwndFileUnits ) {
                        frost_fixed_combo_box fileUnitControl(
                            hwnd, IDC_PARTICLE_FILES_UNITS, Frost::get_file_particle_scale_factor_codes,
                            Frost::get_file_length_unit_name, m_fileUnitIndexToCode );
                        const int sel = fileUnitControl.get_cur_sel_code();

                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_file_length_unit( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                    }
                }
                break;
            case IDC_PARTICLE_FILES_CUSTOM_SCALE_OPTIONS:
                notify_if_missing_frost_mxs();
                frantic::max3d::mxs::expression(
                    _T("if (FrostUi!=undefined) do (FrostUi.on_FileCustomScaleOptions_pressed frostNode)") )
                    .bind( _T("frostNode"), m_obj )
                    .at_time( t )
                    .evaluate<void>();
                break;
            }

            break;
        }
    } catch( const std::exception& e ) {
        frantic::tstring errmsg =
            _T("particle_files_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        FF_LOG( error ) << errmsg << std::endl;
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return FALSE;
}

void particle_files_dlg_proc::Update( TimeValue /*t*/ ) {
    try {
        UpdateEnables();
    } catch( const std::exception& e ) {
        frantic::tstring errmsg =
            _T("particle_files_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        FF_LOG( error ) << errmsg << std::endl;
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

void particle_files_dlg_proc::UpdateEnables() {
    if( !m_obj ) {
        return;
    }
    IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
    if( !pb ) {
        return;
    }
    IParamMap2* pm = pb->GetMap( frost_particle_files_param_map );
    if( !pm ) {
        return;
    }
    const HWND hwnd = pm->GetHWnd();
    if( !hwnd ) {
        return;
    }

    const int selCount = ListBox_GetSelCount( GetDlgItem( hwnd, LB_PARTICLE_FILES ) );

    const bool loadSingleFrame = pb->GetInt( pb_loadSingleFrame ) != 0;         // m_obj->get_load_single_frame();
    const bool enablePlaybackGraph = pb->GetInt( pb_enablePlaybackGraph ) != 0; // m_obj->get_enable_playback_graph();
    const bool limitToRange = pb->GetInt( pb_limitToRange ) != 0;               // m_obj->get_limit_to_range();
    const bool hasSel = selCount > 0;
    const bool inCreateMode = GetCOREInterface()->GetCommandPanelTaskMode() == TASK_MODE_CREATE;

    // add/remove/edit buttons
    frantic::max3d::EnableCustButton( GetDlgItem( hwnd, BTN_PARTICLE_FILES_REMOVE ), hasSel );
    frantic::max3d::EnableCustButton( GetDlgItem( hwnd, BTN_PARTICLE_FILES_EDIT ), !inCreateMode );

    // use playback graph
    pm->Enable( pb_enablePlaybackGraph, !loadSingleFrame );
    // playback graph
    EnableWindow( GetDlgItem( hwnd, IDC_PLAYBACK_GRAPH_STATIC ), enablePlaybackGraph && !loadSingleFrame );
    pm->Enable( pb_playbackGraphTime, enablePlaybackGraph && !loadSingleFrame );

    // frame offset
    EnableWindow( GetDlgItem( hwnd, IDC_FRAME_OFFSET_STATIC ), !loadSingleFrame );
    pm->Enable( pb_frameOffset, !loadSingleFrame );
    // limit to range
    pm->Enable( pb_limitToRange, !loadSingleFrame );
    // range
    EnableWindow( GetDlgItem( hwnd, IDC_SET_USING_EXISTING_FRAMES ), true /*limitToRange && ! loadSingleFrame*/ );
    pm->Enable( pb_rangeFirstFrame, limitToRange && !loadSingleFrame );
    EnableWindow( GetDlgItem( hwnd, IDC_RANGE_SEPARATOR_STATIC ), limitToRange && !loadSingleFrame );
    pm->Enable( pb_rangeLastFrame, limitToRange && !loadSingleFrame );
    // hold first
    EnableWindow( GetDlgItem( hwnd, IDC_PARTICLE_FILES_BEFORE_RANGE_BEHAVIOR ), limitToRange && !loadSingleFrame );
    // hold last
    EnableWindow( GetDlgItem( hwnd, IDC_PARTICLE_FILES_AFTER_RANGE_BEHAVIOR ), limitToRange && !loadSingleFrame );

    const bool useCustomFileScale = m_obj->get_use_custom_file_scale();
    EnableWindow( GetDlgItem( hwnd, IDC_FILES_CUSTOM_SCALE_STATIC ), useCustomFileScale );
    pm->Enable( pb_fileCustomScale, useCustomFileScale );
    frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_PARTICLE_FILES_CUSTOM_SCALE_OPTIONS ),
                                      useCustomFileScale && !inCreateMode );
}

void particle_files_dlg_proc::SetThing( ReferenceTarget* obj ) { m_obj = (Frost*)obj; }

void particle_files_dlg_proc::SetFrost( Frost* frost ) {
    m_obj = frost;
    m_hwnd = 0;
}

void particle_files_dlg_proc::InvalidateUI( HWND hwnd, int element ) {
    if( !hwnd ) {
        return;
    }
    switch( element ) {
    case pb_fileList: {
        update_particle_files_view( hwnd );
        UpdateEnables();
    } break;
    case pb_beforeRangeBehavior: {
        if( m_obj && m_obj->editObj == m_obj ) {
            const int behavior = m_obj->get_before_range_behavior();
            frost_fixed_combo_box beforeRangeControl( hwnd, IDC_PARTICLE_FILES_BEFORE_RANGE_BEHAVIOR,
                                                      Frost::get_before_range_behavior_codes,
                                                      Frost::get_before_range_behavior_name, m_beforeRangeIndexToCode );
            beforeRangeControl.set_cur_sel_code( behavior );
        }
    } break;
    case pb_afterRangeBehavior: {
        if( m_obj && m_obj->editObj == m_obj ) {
            const int behavior = m_obj->get_after_range_behavior();
            frost_fixed_combo_box afterRangeControl( hwnd, IDC_PARTICLE_FILES_AFTER_RANGE_BEHAVIOR,
                                                     Frost::get_after_range_behavior_codes,
                                                     Frost::get_after_range_behavior_name, m_afterRangeIndexToCode );
            afterRangeControl.set_cur_sel_code( behavior );
        }
    } break;
    case pb_fileLengthUnit: {
        if( m_obj && m_obj->editObj == m_obj ) {
            const int fileLengthUnit = m_obj->get_file_length_unit();
            frost_fixed_combo_box fileUnitControl( hwnd, IDC_PARTICLE_FILES_UNITS,
                                                   Frost::get_file_particle_scale_factor_codes,
                                                   Frost::get_file_length_unit_name, m_fileUnitIndexToCode );
            fileUnitControl.set_cur_sel_code( fileLengthUnit );
            UpdateEnables();
        }
    } break;
    }
}

void particle_files_dlg_proc::set_file_list_selection( const std::vector<int>& selection ) {
    IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
    if( !pb ) {
        return;
    }
    IParamMap2* pm = pb->GetMap( frost_particle_files_param_map );
    if( !pm ) {
        return;
    }
    const HWND hwnd = pm->GetHWnd();
    if( !hwnd ) {
        return;
    }
    const HWND hwndFileList = GetDlgItem( hwnd, IDC_PARTICLE_FILES );
    set_list_box_selection_mxs( hwndFileList, selection );
    UpdateEnables();
}

void particle_files_dlg_proc::get_list_item_tooltip( int index, frantic::tstring& outTooltip ) {
    outTooltip.clear();
    if( m_obj && m_obj->editObj == m_obj ) {
        outTooltip.assign( m_obj->get_particle_file( index ) );
    }
}

void particle_files_dlg_proc::DeleteThis() {
    // static !
    // delete this;
}
