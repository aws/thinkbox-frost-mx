// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "Frost.hpp"
#include "resource.h"

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

#include <decomp.h>

#include <frantic/graphics/vector3f.hpp>
#include <frantic/math/utils.hpp>
#include <frantic/max3d/max_utility.hpp>

#include "frost_gui_resources.hpp"
#include "frost_param_block_desc.hpp"
#include "pick_geometry_node_callback.hpp"
#include "utility.hpp"

#include "frost_fixed_combo_box.hpp"
#include "handle_listbox_contextmenu.hpp"
#include "listbox_with_inplace_tooltip.hpp"

extern TCHAR* GetString( int id );

class get_orientation_from_node_callback : public PickModeCallback, PickNodeCallback {
    Frost* m_obj;
    HWND m_hwnd;

  public:
    get_orientation_from_node_callback()
        : m_obj( 0 )
        , m_hwnd( 0 ) {}
    get_orientation_from_node_callback( Frost* obj, HWND hwnd )
        : m_obj( obj )
        , m_hwnd( hwnd ) {}
    void reset( Frost* obj, HWND hwnd ) {
        m_obj = obj;
        m_hwnd = hwnd;
    }
    BOOL HitTest( IObjParam* ip, HWND hWnd, ViewExp* /*vpt*/, IPoint2 m, int /*flags*/ ) {
        // TRUE if something was hit
        return ip->PickNode( hWnd, m /*, this*/ ) ? TRUE : FALSE;
    }
    BOOL Pick( IObjParam* ip, ViewExp* vpt ) {
        // TRUE : end pick mode, FALSE : continue pick mode
        INode* node = vpt->GetClosestHit();
        if( node ) {
            if( m_obj ) {
                if( !theHold.Holding() ) {
                    theHold.Begin();
                }
                m_obj->set_geometry_orientation_from_node( node, ip->GetTime() );
                theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                return TRUE;
            }
        }
        return FALSE;
    }
    BOOL RightClick( IObjParam* /*ip*/, ViewExp* /*vpt*/ ) { return TRUE; }
    BOOL AllowMultiSelect() { return FALSE; }
    void EnterMode( IObjParam* /*ip*/ ) {
        if( !m_obj ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_geometry_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }

        ICustButton* iBut = GetICustButton( GetDlgItem( hwnd, IDC_GET_ORIENTATION_FROM_NODE ) );
        if( iBut )
            iBut->SetCheck( TRUE );
        ReleaseICustButton( iBut );
    }
    void ExitMode( IObjParam* /*ip*/ ) {
        if( !m_obj ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_geometry_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }

        ICustButton* iBut = GetICustButton( GetDlgItem( hwnd, IDC_GET_ORIENTATION_FROM_NODE ) );
        if( iBut )
            iBut->SetCheck( FALSE );
        ReleaseICustButton( iBut );
    }
    BOOL Filter( INode* node ) {
        if( m_obj && node ) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    PickNodeCallback* GetFilter() { return this; }
};

get_orientation_from_node_callback* GetOrientationFromNodeCallback( Frost* obj, HWND hwnd );

class get_axis_from_node_callback : public PickModeCallback, PickNodeCallback {
    Frost* m_obj;
    HWND m_hwnd;

  public:
    get_axis_from_node_callback()
        : m_obj( 0 )
        , m_hwnd( 0 ) {}
    get_axis_from_node_callback( Frost* obj, HWND hwnd )
        : m_obj( obj )
        , m_hwnd( hwnd ) {}
    void reset( Frost* obj, HWND hwnd ) {
        m_obj = obj;
        m_hwnd = hwnd;
    }
    BOOL HitTest( IObjParam* ip, HWND hWnd, ViewExp* /*vpt*/, IPoint2 m, int /*flags*/ ) {
        // TRUE if something was hit
        return ip->PickNode( hWnd, m /*, this*/ ) ? TRUE : FALSE;
    }
    BOOL Pick( IObjParam* ip, ViewExp* vpt ) {
        // TRUE : end pick mode, FALSE : continue pick mode
        INode* node = vpt->GetClosestHit();
        if( node ) {
            if( m_obj ) {
                if( !theHold.Holding() ) {
                    theHold.Begin();
                }
                m_obj->set_divergence_axis_from_node_z( node, ip->GetTime() );
                theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                return TRUE;
            }
        }
        return FALSE;
    }
    BOOL RightClick( IObjParam* /*ip*/, ViewExp* /*vpt*/ ) { return TRUE; }
    BOOL AllowMultiSelect() { return FALSE; }
    void EnterMode( IObjParam* /*ip*/ ) {
        if( !m_obj ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_geometry_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }

        ICustButton* iBut = GetICustButton( GetDlgItem( hwnd, IDC_GET_DIVERGENCE_AXIS_FROM_NODE ) );
        if( iBut )
            iBut->SetCheck( TRUE );
        ReleaseICustButton( iBut );
    }
    void ExitMode( IObjParam* /*ip*/ ) {
        if( !m_obj ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_geometry_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }

        ICustButton* iBut = GetICustButton( GetDlgItem( hwnd, IDC_GET_DIVERGENCE_AXIS_FROM_NODE ) );
        if( iBut )
            iBut->SetCheck( FALSE );
        ReleaseICustButton( iBut );
    }
    BOOL Filter( INode* node ) {
        if( m_obj && node ) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    PickNodeCallback* GetFilter() { return this; }
};

get_axis_from_node_callback* GetAxisFromNodeCallback( Frost* obj, HWND hwnd );

class add_geometry_by_name_callback : public HitByNameDlgCallback {
    Frost* m_obj;
    TimeValue m_t;

  public:
    add_geometry_by_name_callback()
        : m_obj( 0 )
        , m_t( 0 ) {}
    add_geometry_by_name_callback( TimeValue t, Frost* obj )
        : m_obj( obj )
        , m_t( t ) {}

#if MAX_VERSION_MAJOR >= 15
    const MCHAR* dialogTitle() {
#else
    MCHAR* dialogTitle() {
#endif
        return _M( "Add Geometry" );
    }

#if MAX_VERSION_MAJOR >= 15
    const MCHAR* buttonText(){
#else
    MCHAR* buttonText() {
#endif
        return _M( "OK" );
} int filter( INode* node ) {
    if( m_obj ) {
        if( m_obj->accept_geometry( m_t, node ) ) {
            return TRUE;
        }
    }
    return FALSE;
}
void proc( INodeTab& nodeTab ) {
    if( m_obj && nodeTab.Count() ) {
        if( !theHold.Holding() ) {
            theHold.Begin();
        }
        for( int i = 0; i < nodeTab.Count(); ++i ) {
            INode* node = nodeTab[i];
            if( node ) {
                m_obj->add_geometry( m_t, node );
            }
        }
        theHold.Accept( ::GetString( IDS_ADD_GEOMETRY ) );
    }
}
BOOL showHiddenAndFrozen() { return TRUE; }
}
;

class geometry_dlg_proc : public ParamMap2UserDlgProc {
    Frost* m_obj;
    listbox_with_inplace_tooltip m_inplaceTooltip;
    std::vector<frantic::tstring> m_vectorChannelNames;

    std::vector<int> m_geometryTypeIndexToCode;
    std::vector<int> m_geometrySelectionIndexToCode;
    std::vector<int> m_geometryTimingBaseIndexToCode;
    std::vector<int> m_geometryTimingOffsetIndexToCode;
    std::vector<int> m_geometryOrientationIndexToCode;
    std::vector<int> m_divergenceAxisIndexToCode;

    std::vector<int> m_optionButtons;
    void init() {
        m_optionButtons.clear();
        m_optionButtons = { IDC_GEOMETRY_OPTIONS, IDC_GEOMETRY_RANDOM_SELECTION_SEED_OPTIONS,
                            IDC_GEOMETRY_SAMPLE_TIME_MAX_RANDOM_OFFSET_OPTIONS, IDC_GEOMETRY_SAMPLE_TIME_SEED_OPTIONS,
                            IDC_ORIENTATION_DIVERGENCE_OPTIONS };
    }

    void update_geometry_list( HWND hwnd ) {
        if( m_obj && m_obj->editObj == m_obj ) {
            HWND hwndCtrl = GetDlgItem( hwnd, IDC_GEOMETRY_LIST );
            if( hwndCtrl ) {
                std::vector<frantic::tstring> nodeNames;
                m_obj->get_geometry_names( nodeNames );
                frantic::win32::ffListBox_SetStrings( hwndCtrl, nodeNames );
            }
        }
    }

    void update_vector_channel_list( TimeValue t, HWND hwnd, bool reload = false ) {
        if( m_obj && m_obj->editObj == m_obj ) {
            if( reload ) {
                m_obj->invalidate_vector_channel_name_cache();
            }
            HWND hwndComboBox = GetDlgItem( hwnd, IDC_GEOMETRY_VECTOR_CHANNELS );
            if( hwndComboBox ) {
                m_vectorChannelNames.clear();
                std::vector<frantic::tstring> vectorChannels;
                m_obj->get_vector_channel_names( vectorChannels, t );
                const frantic::tstring currentChannel = m_obj->get_geometry_orientation_vector_channel();

                frantic::win32::ffComboBox_Clear( hwndComboBox );

                for( std::size_t i = 0; i < vectorChannels.size(); ++i ) {
                    int result = frantic::win32::ffComboBox_AddString( hwndComboBox, vectorChannels[i] );
                    if( result == CB_ERR ) {
                        const frantic::tstring errmsg = _T("Frost Internal Error: Unable to add channel \'") +
                                                        vectorChannels[i] + _T("\' to the vector channel control.");
                        FF_LOG( error ) << errmsg << std::endl;
                    } else if( result == CB_ERRSPACE ) {
                        const frantic::tstring errmsg =
                            _T("Frost Internal Error: Insufficient space to store channel name \'") +
                            vectorChannels[i] + _T("\' in vector channel control.");
                        FF_LOG( error ) << errmsg << std::endl;
                    } else if( result >= 0 ) {
                        m_vectorChannelNames.push_back( vectorChannels[i] );
                    } else {
                        const frantic::tstring errmsg =
                            _T("Frost Internal Error: An unknown error occurred while attempting to add channel \'") +
                            vectorChannels[i] + _T("\' to the vector channel control.");
                        FF_LOG( error ) << errmsg << std::endl;
                    }
                }
                int sel = -1;
                for( std::size_t i = 0; i < m_vectorChannelNames.size(); ++i ) {
                    if( m_vectorChannelNames[i] == currentChannel ) {
                        sel = (int)i;
                    }
                }
                if( sel >= 0 ) {
                    ComboBox_SetCurSel( hwndComboBox, sel );
                } else {
                    const frantic::tstring displayString = currentChannel.empty() ? _T("<none>") : currentChannel;
                    int result = frantic::win32::ffComboBox_AddString( hwndComboBox, displayString );
                    if( result == CB_ERR ) {
                        const frantic::tstring errmsg =
                            _T("Frost Internal Error: Unable to add the currently selected vector channel \'") +
                            currentChannel + _T("\' to the vector channel control.");
                        FF_LOG( error ) << errmsg << std::endl;
                    } else if( result == CB_ERRSPACE ) {
                        const frantic::tstring errmsg = _T("Frost Internal Error: Insufficient space to store the ")
                                                        _T("currently selected vector channel name \'") +
                                                        currentChannel + _T("\' in vector channel control.");
                        FF_LOG( error ) << errmsg << std::endl;
                    } else if( result >= 0 ) {
                        m_vectorChannelNames.push_back( currentChannel );
                        ComboBox_SetCurSel( hwndComboBox, result );
                    } else {
                        const frantic::tstring errmsg =
                            _T("Frost Internal Error: An unknown error occurred while attempting to add the currently ")
                            _T("selected vector channel \'") +
                            currentChannel + _T("\' to the vector channel control.");
                        FF_LOG( error ) << errmsg << std::endl;
                    }
                }
            } else {
                const frantic::tstring errmsg = _T("Frost Internal Error: could not find vector channel control.");
                FF_LOG( error ) << errmsg << std::endl;
            }
        }
    }

  public:
    geometry_dlg_proc()
        : m_obj( 0 ) {
        init();
    }

    geometry_dlg_proc( Frost* obj )
        : m_obj( obj ) {
        init();
    }

    INT_PTR DlgProc( TimeValue t, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
        try {
            if( !map ) {
                throw std::runtime_error( "Error: parameter map is NULL" );
            }
            if( !m_obj ) {
                throw std::runtime_error( "Error: class object is NULL" );
            }
            int id = LOWORD( wParam );
            int notifyCode = HIWORD( wParam );
            switch( msg ) {
            case WM_INITDIALOG: {
                update_geometry_list( hwnd );
                HWND hwndButton = GetDlgItem( hwnd, IDC_GEOMETRY_PICK );
                if( hwndButton ) {
                    ICustButton* iBut = GetICustButton( hwndButton );
                    if( iBut ) {
                        iBut->SetType( CBT_CHECK );
                        iBut->SetHighlightColor( GREEN_WASH );
                        ReleaseICustButton( iBut );
                    }
                }
                hwndButton = GetDlgItem( hwnd, IDC_LOOK_AT_NODE );
                ICustButton* iBut = GetICustButton( hwndButton );
                if( iBut ) {
                    INode* lookAtNode = m_obj->get_orientation_look_at_node();
                    if( lookAtNode ) {
                        iBut->SetText( lookAtNode->NodeName() );
                    } else {
                        iBut->SetText( _T("None") );
                    }
                    ReleaseICustButton( iBut );
                }

                hwndButton = GetDlgItem( hwnd, IDC_GET_ORIENTATION_FROM_NODE );
                if( hwndButton ) {
                    ICustButton* iBut = GetICustButton( hwndButton );
                    if( iBut ) {
                        iBut->SetType( CBT_CHECK );
                        iBut->SetHighlightColor( GREEN_WASH );
                        ReleaseICustButton( iBut );
                    }
                }
                hwndButton = GetDlgItem( hwnd, IDC_GET_DIVERGENCE_AXIS_FROM_NODE );
                if( hwndButton ) {
                    ICustButton* iBut = GetICustButton( hwndButton );
                    if( iBut ) {
                        iBut->SetType( CBT_CHECK );
                        iBut->SetHighlightColor( GREEN_WASH );
                        ReleaseICustButton( iBut );
                    }
                }
                BOOST_FOREACH( int idc, m_optionButtons ) {
                    GetFrostGuiResources()->apply_custbutton_fast_forward_icon( GetDlgItem( hwnd, idc ) );
                    CustButton_SetRightClickNotify( GetDlgItem( hwnd, idc ), true );
                }
                int xButtons[] = { IDC_CLEAR_LOOK_AT_NODE };
                BOOST_FOREACH( int idc, xButtons ) {
                    GetFrostGuiResources()->apply_custbutton_x_icon( GetDlgItem( hwnd, idc ) );
                }

                frost_fixed_combo_box geometryTypeControl( hwnd, IDC_GEOMETRY_TYPE, Frost::get_geometry_type_codes,
                                                           Frost::get_geometry_type_name, m_geometryTypeIndexToCode );
                geometryTypeControl.reset_strings();
                geometryTypeControl.set_cur_sel_code( m_obj->get_geometry_type() );

                frost_fixed_combo_box geometrySelectionControl(
                    hwnd, IDC_GEOMETRY_SHAPE_SELECTION, Frost::get_geometry_selection_mode_codes,
                    Frost::get_geometry_selection_mode_name, m_geometrySelectionIndexToCode );
                geometrySelectionControl.reset_strings();
                geometrySelectionControl.set_cur_sel_code( m_obj->get_geometry_selection_mode() );

                frost_fixed_combo_box geometryBaseTimeControl(
                    hwnd, IDC_GEOMETRY_ANIMATION_BASE_MODE, Frost::get_geometry_timing_base_mode_codes,
                    Frost::get_geometry_timing_base_mode_name, m_geometryTimingBaseIndexToCode );
                geometryBaseTimeControl.reset_strings();
                geometryBaseTimeControl.set_cur_sel_code( m_obj->get_geometry_timing_base_mode() );

                frost_fixed_combo_box geometryTimingControl(
                    hwnd, IDC_GEOMETRY_ANIMATION_OFFSET_MODE, Frost::get_geometry_timing_offset_mode_codes,
                    Frost::get_geometry_timing_offset_mode_name, m_geometryTimingOffsetIndexToCode );
                geometryTimingControl.reset_strings();
                geometryTimingControl.set_cur_sel_code( m_obj->get_geometry_timing_offset_mode() );

                frost_fixed_combo_box geometryOrientationControl(
                    hwnd, IDC_GEOMETRY_ORIENTATION, Frost::get_geometry_orientation_mode_codes,
                    Frost::get_geometry_orientation_mode_name, m_geometryOrientationIndexToCode );
                geometryOrientationControl.reset_strings();
                geometryOrientationControl.set_cur_sel_code( m_obj->get_geometry_orientation_mode() );

                frost_fixed_combo_box divergenceAxisSpaceControl(
                    hwnd, IDC_DIVERGENCE_AXIS_SPACE, Frost::get_divergence_axis_space_codes,
                    Frost::get_divergence_axis_space_name, m_divergenceAxisIndexToCode );
                divergenceAxisSpaceControl.reset_strings();
                divergenceAxisSpaceControl.set_cur_sel_code( m_obj->get_divergence_axis_space() );

                update_vector_channel_list( t, hwnd );

                typedef std::pair<int, int> control_tooltip_t;
                control_tooltip_t buttonTooltips[] = {
                    std::make_pair( IDC_GEOMETRY_PICK, IDS_GEOMETRY_PICK_TOOLTIP ),
                    std::make_pair( IDC_GEOMETRY_ADD, IDS_GEOMETRY_ADD_TOOLTIP ),
                    std::make_pair( IDC_GEOMETRY_REMOVE, IDS_GEOMETRY_REMOVE_TOOLTIP ),
                    std::make_pair( IDC_GEOMETRY_OPTIONS, IDS_GEOMETRY_OPTIONS_TOOLTIP ),
                    std::make_pair( IDC_LOOK_AT_NODE, IDS_LOOK_AT_NODE_TOOLTIP ),
                    std::make_pair( IDC_CLEAR_LOOK_AT_NODE, IDS_CLEAR_LOOK_AT_NODE_TOOLTIP ),
                    std::make_pair( IDC_REFRESH_VECTOR_CHANNELS, IDS_REFRESH_VECTOR_CHANNELS_TOOLTIP ),
                    std::make_pair( IDC_GET_ORIENTATION_FROM_NODE, IDS_GET_ORIENTATION_FROM_NODE_TOOLTIP ),
                    std::make_pair( IDC_GET_DIVERGENCE_AXIS_FROM_NODE, IDS_GET_DIVERGENCE_AXIS_FROM_NODE_TOOLTIP ) };
                BOOST_FOREACH( const control_tooltip_t& buttonTooltip, buttonTooltips ) {
                    CustButton_SetTooltip( GetDlgItem( hwnd, buttonTooltip.first ), GetString( buttonTooltip.second ) );
                }

                UpdateEnables();

                m_inplaceTooltip.attach( GetDlgItem( hwnd, IDC_GEOMETRY_LIST ) );
            }
                return TRUE;
            case WM_DESTROY:
                m_inplaceTooltip.detach( GetDlgItem( hwnd, IDC_GEOMETRY_LIST ) );
                break;
            case WM_COMMAND:
                switch( id ) {
                case IDC_GEOMETRY_TYPE:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box geometryTypeControl(
                            hwnd, IDC_GEOMETRY_TYPE, Frost::get_geometry_type_codes, Frost::get_geometry_type_name,
                            m_geometryTypeIndexToCode );
                        int sel = geometryTypeControl.get_cur_sel_code();

                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_geometry_type( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                        return TRUE;
                    }
                    break;
                case IDC_GEOMETRY_PICK:
                    if( m_obj->ip ) {
                        m_obj->ip->SetPickMode( GetPickGeometryNodeCallback( m_obj, hwnd ) );
                    }
                    return TRUE;
                case IDC_GEOMETRY_ADD:
                    if( m_obj->ip ) {
                        add_geometry_by_name_callback callback( t, m_obj );
                        m_obj->ip->DoHitByNameDialog( &callback );
                    }
                    return TRUE;
                case IDC_GEOMETRY_REMOVE: {
                    std::vector<int> selIndices;
                    frantic::win32::ffListBox_GetSelItems( GetDlgItem( hwnd, IDC_GEOMETRY_LIST ), selIndices );
                    if( selIndices.size() > 0 ) {
                        int response = IDYES;
                        /*
                        std::string message = "Are you sure you want to REMOVE the ";
                        message += boost::lexical_cast<std::string>( selIndices.size() ) + " ";
                        message += ( selIndices.size() > 1 ) ? "geometry objects" : "geometry object";
                        message += "?";
                        std::string caption = "Frost : Remove Geometry Object";
                        int response = MessageBox( GetCOREInterface()->GetMAXHWnd(), message.c_str(), caption.c_str(),
                        MB_YESNO );
                        */
                        if( response == IDYES ) {
                            if( !theHold.Holding() ) {
                                theHold.Begin();
                            }
                            m_obj->remove_geometry( selIndices );
                            theHold.Accept( ::GetString( IDS_REMOVE_GEOMETRY ) );
                        }
                    }
                }
                    return TRUE;
                case IDC_GEOMETRY_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_GeometryListOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case LB_GEOMETRY:
                    switch( notifyCode ) {
                    case LBN_DBLCLK:
                        notify_if_missing_frost_mxs();
                        frantic::max3d::mxs::expression(
                            _T("if (FrostUi!=undefined) do (FrostUi.on_GeometryList_doubleClicked frostNode)") )
                            .bind( _T("frostNode"), m_obj )
                            .at_time( t )
                            .evaluate<void>();
                        break;
                    case LBN_SELCHANGE:
                        UpdateEnables();
                        return TRUE;
                    case LBN_SELCANCEL:
                        UpdateEnables();
                        return TRUE;
                    }
                    break;
                case IDC_GEOMETRY_SHAPE_SELECTION:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box geometrySelectionControl(
                            hwnd, IDC_GEOMETRY_SHAPE_SELECTION, Frost::get_geometry_selection_mode_codes,
                            Frost::get_geometry_selection_mode_name, m_geometrySelectionIndexToCode );
                        int sel = geometrySelectionControl.get_cur_sel_code();

                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_geometry_selection_mode( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );

                        return TRUE;
                    }
                    break;
                case IDC_GEOMETRY_RANDOM_SELECTION_SEED_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_GeometrySelectionSeedOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case IDC_GEOMETRY_ANIMATION_BASE_MODE:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box geometryBaseTimeControl(
                            hwnd, IDC_GEOMETRY_ANIMATION_BASE_MODE, Frost::get_geometry_timing_base_mode_codes,
                            Frost::get_geometry_timing_base_mode_name, m_geometryTimingBaseIndexToCode );
                        int sel = geometryBaseTimeControl.get_cur_sel_code();

                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_geometry_timing_base_mode( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );

                        return TRUE;
                    }
                    break;
                case IDC_GEOMETRY_ANIMATION_OFFSET_MODE:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box geometryTimingControl(
                            hwnd, IDC_GEOMETRY_ANIMATION_OFFSET_MODE, Frost::get_geometry_timing_offset_mode_codes,
                            Frost::get_geometry_timing_offset_mode_name, m_geometryTimingOffsetIndexToCode );
                        int sel = geometryTimingControl.get_cur_sel_code();

                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_geometry_timing_offset_mode( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );

                        return TRUE;
                    }
                    break;
                case IDC_GEOMETRY_SAMPLE_TIME_MAX_RANDOM_OFFSET_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_GeometrySampleTimeMaxRandomOffsetOptions_pressed ")
                        _T("frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case IDC_GEOMETRY_SAMPLE_TIME_SEED_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_GeometrySampleTimeSeedOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case IDC_GEOMETRY_ORIENTATION:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box geometryOrientationControl(
                            hwnd, IDC_GEOMETRY_ORIENTATION, Frost::get_geometry_orientation_mode_codes,
                            Frost::get_geometry_orientation_mode_name, m_geometryOrientationIndexToCode );
                        int sel = geometryOrientationControl.get_cur_sel_code();

                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_geometry_orientation_mode( sel );

                        return TRUE;
                    }
                    break;
                case IDC_REFRESH_VECTOR_CHANNELS: {
                    update_vector_channel_list( t, hwnd, true );
                }
                    return TRUE;
                case IDC_GEOMETRY_VECTOR_CHANNELS:
                    if( CBN_SELCHANGE == notifyCode ) {
                        HWND hwndCtrl = GetDlgItem( hwnd, id );
                        int sel = ComboBox_GetCurSel( hwndCtrl );
                        if( sel != CB_ERR ) {
                            if( sel >= 0 && static_cast<std::size_t>( sel ) < m_vectorChannelNames.size() ) {
                                const frantic::tstring s = m_vectorChannelNames[sel];
                                if( !s.empty() ) {
                                    if( !theHold.Holding() ) {
                                        theHold.Begin();
                                    }
                                    m_obj->set_geometry_orientation_vector_channel( s );
                                    theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                                }
                            } else {
                                const frantic::tstring errmsg = _T("Frost Internal Error: selected vector channel is ")
                                                                _T("out of range of the cached vector channels.");
                                FF_LOG( error ) << errmsg << std::endl;
                            }
                        }
                        return TRUE;
                    }
                    break;
                case IDC_DIVERGENCE_AXIS_SPACE:
                    if( CBN_SELCHANGE == notifyCode ) {
                        frost_fixed_combo_box divergenceAxisSpaceControl(
                            hwnd, IDC_DIVERGENCE_AXIS_SPACE, Frost::get_divergence_axis_space_codes,
                            Frost::get_divergence_axis_space_name, m_divergenceAxisIndexToCode );
                        int sel = divergenceAxisSpaceControl.get_cur_sel_code();

                        if( !theHold.Holding() ) {
                            theHold.Begin();
                        }
                        m_obj->set_divergence_axis_space( sel );
                        theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );

                        return TRUE;
                    }
                    break;
                case IDC_CLEAR_LOOK_AT_NODE:
                    if( !theHold.Holding() ) {
                        theHold.Begin();
                    }
                    m_obj->set_orientation_look_at_node( 0 );
                    theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
                    return TRUE;
                case IDC_GET_ORIENTATION_FROM_NODE: {
                    if( m_obj->ip ) {
                        m_obj->ip->SetPickMode( GetOrientationFromNodeCallback( m_obj, hwnd ) );
                    }
                }
                    return TRUE;
                case IDC_ORIENTATION_DIVERGENCE_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_GeometryOrientationDivergenceOptions_pressed ")
                        _T("frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    return TRUE;
                case IDC_GET_DIVERGENCE_AXIS_FROM_NODE: {
                    if( m_obj->ip ) {
                        m_obj->ip->SetPickMode( GetAxisFromNodeCallback( m_obj, hwnd ) );
                    }
                }
                    return TRUE;
                }
                break;
            case WM_CONTEXTMENU:
                if( handle_listbox_contextmenu( GetDlgItem( hwnd, LB_GEOMETRY ), lParam ) ) {
                    UpdateEnables();
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_GeometryList_rightClicked frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                }
                break;
            }
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("geometry_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
        return FALSE;
    }

    void SetThing( ReferenceTarget* obj ) { m_obj = (Frost*)obj; }

    void SetFrost( Frost* frost ) { m_obj = frost; }

    void Update( TimeValue /*t*/ ) {
        try {
            UpdateEnables();
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("geometry_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
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
        IParamMap2* pm = pb->GetMap( frost_geometry_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }

        const bool inCreateMode = GetCOREInterface()->GetCommandPanelTaskMode() == TASK_MODE_CREATE;
        const bool hasGeometryListSel = ListBox_GetSelCount( GetDlgItem( hwnd, LB_GEOMETRY ) ) > 0;

        const bool useGeometryList = ( m_obj->get_geometry_type() == GEOMETRY_TYPE::CUSTOM_GEOMETRY );
        const bool randomShapeById = ( m_obj->get_geometry_selection_mode() == GEOMETRY_SELECTION_MODE::RANDOM_BY_ID );
        const bool randomTimeById =
            ( m_obj->get_geometry_timing_offset_mode() == GEOMETRY_TIMING_OFFSET_MODE::RANDOM_BY_ID );

        const int orientationMode = m_obj->get_geometry_orientation_mode();
        const bool useLookAt = ( orientationMode == GEOMETRY_ORIENTATION_MODE::LOOK_AT ) ||
                               ( orientationMode == GEOMETRY_ORIENTATION_MODE::MATCH_OBJECT );
        const bool useVectorChannel = ( orientationMode == GEOMETRY_ORIENTATION_MODE::VECTOR_CHANNEL );
        const bool specifyOrientation = ( orientationMode == GEOMETRY_ORIENTATION_MODE::SPECIFY );

        const bool restrictDivergence = pb->GetInt( pb_geometryOrientationRestrictDivergenceAxis ) != 0;

        BOOST_FOREACH( int idc, m_optionButtons ) {
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, idc ), !inCreateMode );
        }

        {
            const bool enable = useGeometryList && !inCreateMode;
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_GEOMETRY_PICK ), enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_GEOMETRY_ADD ), enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_GEOMETRY_REMOVE ), enable && hasGeometryListSel );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_GEOMETRY_OPTIONS ), enable );
            EnableWindow( GetDlgItem( hwnd, IDC_GEOMETRY_LIST ), enable );
            EnableWindow( GetDlgItem( hwnd, IDC_GEOMETRY_SHAPE_SELECTION ), enable );
        }

        {
            const bool enable = useGeometryList && randomShapeById;
            EnableWindow( GetDlgItem( hwnd, IDC_GEOMETRY_SEED_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_GEOMETRY_SEED_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_geometrySelectionSeed, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_GEOMETRY_RANDOM_SELECTION_SEED_OPTIONS ),
                                              enable && !inCreateMode );
        }

        {
            const bool enable = randomTimeById;
            EnableWindow( GetDlgItem( hwnd, IDC_GEOMETRY_MAX_RANDOM_OFFSET_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_GEOMETRY_MAX_RANDOM_OFFSET_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_geometrySampleTimeMaxRandomOffset, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_GEOMETRY_SAMPLE_TIME_MAX_RANDOM_OFFSET_OPTIONS ),
                                              enable && !inCreateMode );

            EnableWindow( GetDlgItem( hwnd, IDC_GEOMETRY_SAMPLE_TIME_SEED_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_GEOMETRY_SAMPLE_TIME_SEED_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_geometrySampleTimeSeed, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_GEOMETRY_SAMPLE_TIME_SEED_OPTIONS ),
                                              enable && !inCreateMode );
        }

        {
            const bool enable = useLookAt;
            EnableWindow( GetDlgItem( hwnd, IDC_LOOK_AT_NODE_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_LOOK_AT_NODE_STATIC ), 0, 0, RDW_INVALIDATE );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_LOOK_AT_NODE ), enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_CLEAR_LOOK_AT_NODE ), enable );
        }

        {
            const bool enable = useVectorChannel;
            EnableWindow( GetDlgItem( hwnd, IDC_VECTOR_CHANNEL_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_VECTOR_CHANNEL_STATIC ), 0, 0, RDW_INVALIDATE );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_REFRESH_VECTOR_CHANNELS ), enable );
            EnableWindow( GetDlgItem( hwnd, IDC_GEOMETRY_VECTOR_CHANNELS ), enable );
        }

        {
            const bool enable = specifyOrientation;
            EnableWindow( GetDlgItem( hwnd, IDC_GEOMETRY_ORIENTATION_X_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_GEOMETRY_ORIENTATION_X_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_geometryOrientationX, enable );
            EnableWindow( GetDlgItem( hwnd, IDC_GEOMETRY_ORIENTATION_Y_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_GEOMETRY_ORIENTATION_Y_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_geometryOrientationY, enable );
            EnableWindow( GetDlgItem( hwnd, IDC_GEOMETRY_ORIENTATION_Z_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_GEOMETRY_ORIENTATION_Z_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_geometryOrientationZ, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_GET_ORIENTATION_FROM_NODE ), enable );
        }

        {
            const bool enable = restrictDivergence;
            EnableWindow( GetDlgItem( hwnd, IDC_DIVERGENCE_AXIS_SPACE ), enable );
            EnableWindow( GetDlgItem( hwnd, IDC_DIVERGENCE_X_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_DIVERGENCE_X_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_geometryOrientationDivergenceAxisX, enable );
            EnableWindow( GetDlgItem( hwnd, IDC_DIVERGENCE_Y_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_DIVERGENCE_Y_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_geometryOrientationDivergenceAxisY, enable );
            EnableWindow( GetDlgItem( hwnd, IDC_DIVERGENCE_Z_STATIC ), enable );
            RedrawWindow( GetDlgItem( hwnd, IDC_DIVERGENCE_Z_STATIC ), 0, 0, RDW_INVALIDATE );
            pm->Enable( pb_geometryOrientationDivergenceAxisZ, enable );
            frantic::max3d::EnableCustButton( GetDlgItem( hwnd, IDC_GET_DIVERGENCE_AXIS_FROM_NODE ), enable );
        }
    }

    void InvalidateUI( HWND hwnd, int element ) {
        if( !hwnd ) {
            return;
        }
        switch( element ) {
        case pb_geometryType:
            if( m_obj && m_obj->editObj == m_obj ) {
                const int s = m_obj->get_geometry_type();
                frost_fixed_combo_box geometryTypeControl( hwnd, IDC_GEOMETRY_TYPE, Frost::get_geometry_type_codes,
                                                           Frost::get_geometry_type_name, m_geometryTypeIndexToCode );
                geometryTypeControl.set_cur_sel_code( s );
            }
            UpdateEnables();
            break;
        case pb_geometryList:
            update_geometry_list( hwnd );
            UpdateEnables();
            break;
        case pb_geometrySelectionMode:
            if( m_obj && m_obj->editObj == m_obj ) {
                const int s = m_obj->get_geometry_selection_mode();
                frost_fixed_combo_box geometrySelectionControl(
                    hwnd, IDC_GEOMETRY_SHAPE_SELECTION, Frost::get_geometry_selection_mode_codes,
                    Frost::get_geometry_selection_mode_name, m_geometrySelectionIndexToCode );
                geometrySelectionControl.set_cur_sel_code( s );
            }
            UpdateEnables();
            break;
        case pb_geometrySampleTimeBaseMode:
            if( m_obj && m_obj->editObj == m_obj ) {
                const int s = m_obj->get_geometry_timing_base_mode();
                frost_fixed_combo_box geometryBaseTimeControl(
                    hwnd, IDC_GEOMETRY_ANIMATION_BASE_MODE, Frost::get_geometry_timing_base_mode_codes,
                    Frost::get_geometry_timing_base_mode_name, m_geometryTimingBaseIndexToCode );
                geometryBaseTimeControl.set_cur_sel_code( s );
            }
            UpdateEnables();
            break;
        case pb_geometrySampleTimeOffsetMode:
            if( m_obj && m_obj->editObj == m_obj ) {
                const int s = m_obj->get_geometry_timing_offset_mode();
                frost_fixed_combo_box geometryTimingControl(
                    hwnd, IDC_GEOMETRY_ANIMATION_OFFSET_MODE, Frost::get_geometry_timing_offset_mode_codes,
                    Frost::get_geometry_timing_offset_mode_name, m_geometryTimingOffsetIndexToCode );
                geometryTimingControl.set_cur_sel_code( s );
            }
            UpdateEnables();
            break;
        case pb_geometryOrientationMode:
            if( m_obj && m_obj->editObj == m_obj ) {
                const int s = m_obj->get_geometry_orientation_mode();
                frost_fixed_combo_box geometryOrientationControl(
                    hwnd, IDC_GEOMETRY_ORIENTATION, Frost::get_geometry_orientation_mode_codes,
                    Frost::get_geometry_orientation_mode_name, m_geometryOrientationIndexToCode );
                geometryOrientationControl.set_cur_sel_code( s );
            }
            UpdateEnables();
            break;
        case pb_geometryOrientationLookAtNode:
            if( m_obj && m_obj->editObj == m_obj ) {
                HWND hwndCtrl = GetDlgItem( hwnd, IDC_LOOK_AT_NODE );
                ICustButton* iBut = GetICustButton( hwndCtrl );
                if( iBut ) {
                    INode* lookAtNode = m_obj->get_orientation_look_at_node();
                    if( lookAtNode ) {
                        iBut->SetText( lookAtNode->NodeName() );
                    } else {
                        iBut->SetText( _T("None") );
                    }
                    ReleaseICustButton( iBut );
                }
            }
            break;
        case pb_geometryOrientationVectorChannel:
            if( m_obj && m_obj->editObj == m_obj ) {
                update_vector_channel_list( GetCOREInterface()->GetTime(), hwnd );
            }
            break;
        case pb_geometryOrientationDivergenceAxisSpace:
            if( m_obj && m_obj->editObj == m_obj ) {
                const int s = m_obj->get_divergence_axis_space();
                frost_fixed_combo_box divergenceAxisSpaceControl(
                    hwnd, IDC_DIVERGENCE_AXIS_SPACE, Frost::get_divergence_axis_space_codes,
                    Frost::get_divergence_axis_space_name, m_divergenceAxisIndexToCode );
                divergenceAxisSpaceControl.set_cur_sel_code( s );
            }
            UpdateEnables();
            break;
        }
    }

    void get_geometry_list_selection( std::vector<int>& outSelection ) {
        outSelection.clear();
        if( ( !m_obj ) || ( m_obj != m_obj->editObj ) ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_geometry_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }
        const HWND hwndGeometryList = GetDlgItem( hwnd, IDC_GEOMETRY_LIST );
        std::vector<int> result;
        get_list_box_selection_mxs( hwndGeometryList, result );
        outSelection.swap( result );
    }

    void set_geometry_list_selection( const std::vector<int>& selection ) {
        if( ( !m_obj ) || ( m_obj != m_obj->editObj ) ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_geometry_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }
        const HWND hwndGeometryList = GetDlgItem( hwnd, IDC_GEOMETRY_LIST );
        set_list_box_selection_mxs( hwndGeometryList, selection );
        UpdateEnables();
    }

    void invalidate_geometry_list_labels() {
        if( m_obj && m_obj->editObj == m_obj ) {
            IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
            if( !pb ) {
                return;
            }
            IParamMap2* pm = pb->GetMap( frost_geometry_param_map );
            if( !pm ) {
                return;
            }
            const HWND hwnd = pm->GetHWnd();
            if( !hwnd ) {
                return;
            }
            HWND hwndCtrl = GetDlgItem( hwnd, IDC_GEOMETRY_LIST );
            if( hwndCtrl ) {
                std::vector<frantic::tstring> nodeNames;
                m_obj->get_geometry_names( nodeNames );
                if( nodeNames.size() == ListBox_GetCount( hwndCtrl ) ) {
                    // try to keep selection and scroll pos
                    const int topIndex = ListBox_GetTopIndex( hwndCtrl );
                    std::vector<int> selection;
                    frantic::win32::ffListBox_GetSelItems( hwndCtrl, selection );
                    frantic::win32::ffListBox_SetStrings( hwndCtrl, nodeNames );
                    frantic::win32::ffListBox_SetSelItems( hwndCtrl, selection );
                    ListBox_SetTopIndex( hwndCtrl, topIndex );
                } else {
                    frantic::win32::ffListBox_SetStrings( hwndCtrl, nodeNames );
                }
            }
        }
    }

    void DeleteThis() {
        // static !
        /*delete this;*/
    }
};
