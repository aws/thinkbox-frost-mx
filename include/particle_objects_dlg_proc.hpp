// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "Frost.hpp"
#include "resource.h"

#include <boost/foreach.hpp>
#include <frantic/max3d/max_utility.hpp>

#include "frost_gui_resources.hpp"
#include "frost_param_block_desc.hpp"
#include "handle_listbox_contextmenu.hpp"
#include "listbox_with_inplace_tooltip.hpp"
#include "utility.hpp"

extern TCHAR* GetString( int id );

class particle_node_filter_callback : public PickNodeCallback {
    Frost* m_obj;

  public:
    particle_node_filter_callback()
        : m_obj( 0 ) {}
    particle_node_filter_callback( Frost* obj )
        : m_obj( obj ) {}
    BOOL Filter( INode* node ) {
        if( m_obj && m_obj->accept_particle_object( node ) ) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
};

class pick_particle_object_callback : public PickModeCallback, PickNodeCallback {
    Frost* m_obj;
    HWND m_hwnd;

  public:
    pick_particle_object_callback()
        : m_obj( 0 )
        , m_hwnd( 0 ) {}
    pick_particle_object_callback( Frost* obj, HWND hwnd )
        : m_obj( obj )
        , m_hwnd( hwnd ) {}
    void reset( Frost* obj, HWND hwnd ) {
        m_obj = obj;
        m_hwnd = hwnd;
    }
    BOOL HitTest( IObjParam* ip, HWND hWnd, ViewExp* /*vpt*/, IPoint2 m, int /*flags*/ ) {
        // TRUE if something was hit
        particle_node_filter_callback filter( m_obj );
        return ip->PickNode( hWnd, m, this ) ? TRUE : FALSE;
    }
    BOOL Pick( IObjParam* /*ip*/, ViewExp* vpt ) {
        // TRUE : end pick mode, FALSE : continue pick mode
        INode* node = vpt->GetClosestHit();
        if( node ) {
            if( m_obj && m_obj->accept_particle_object( node ) ) {
                if( !theHold.Holding() ) {
                    theHold.Begin();
                }
                m_obj->add_particle_object( node );
                theHold.Accept( ::GetString( IDS_ADD_PARTICLE_OBJECT ) );
                return TRUE;
            }
        }
        return FALSE;
    }
    BOOL RightClick( IObjParam* /*ip*/, ViewExp* /*vpt*/ ) { return TRUE; }
    BOOL AllowMultiSelect() { return FALSE; }
    void EnterMode( IObjParam* /*ip*/ ) {
        if( m_obj ) {
            m_obj->begin_pick_particle_object( m_hwnd );
        }
    }
    void ExitMode( IObjParam* /*ip*/ ) {
        if( m_obj ) {
            m_obj->end_pick_particle_object( m_hwnd );
        }
    }
    BOOL Filter( INode* node ) {
        if( m_obj && m_obj->accept_particle_object( node ) ) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    PickNodeCallback* GetFilter() { return this; }
};

pick_particle_object_callback* GetPickParticleObjectCallback( Frost* obj, HWND hwnd );

class add_particle_object_by_name_callback : public HitByNameDlgCallback {
    Frost* m_obj;

  public:
    add_particle_object_by_name_callback()
        : m_obj( 0 ) {}
    add_particle_object_by_name_callback( Frost* obj )
        : m_obj( obj ) {}

#if MAX_VERSION_MAJOR >= 15
    const MCHAR* dialogTitle() {
#else
    MCHAR* dialogTitle() {
#endif
        return _M( "Add Particles" );
    }

#if MAX_VERSION_MAJOR >= 15
    const MCHAR* buttonText(){
#else
    MCHAR* buttonText() {
#endif
        return _M( "OK" );
}

	int filter( INode * node ) {
    if( m_obj ) {
        if( m_obj->accept_particle_object( node ) ) {
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
                m_obj->add_particle_object( node );
            }
        }
        theHold.Accept( ::GetString( IDS_ADD_PARTICLE_OBJECTS ) );
    }
}
BOOL showHiddenAndFrozen() { return TRUE; }
}
;

class particle_objects_dlg_proc : public ParamMap2UserDlgProc {
    Frost* m_obj;
    HWND m_hwnd;
    listbox_with_inplace_tooltip m_inplaceTooltip;

    void update_node_list( HWND hwnd ) {
        if( m_obj && m_obj->editObj == m_obj ) {
            HWND hwndCtrl = GetDlgItem( hwnd, LB_PARTICLE_OBJECTS );
            if( hwndCtrl ) {
                std::vector<frantic::tstring> nodeNames;
                m_obj->get_particle_object_names( nodeNames );
                frantic::win32::ffListBox_SetStrings( hwndCtrl, nodeNames );
            }
        }
    }

    void update_control_enable( HWND hwnd ) {
        bool inCreateMode = GetCOREInterface()->GetCommandPanelTaskMode() == TASK_MODE_CREATE;

        bool hasSelectedItem = false;
        HWND hwndListBox = GetDlgItem( hwnd, LB_PARTICLE_OBJECTS );
        if( hwndListBox ) {
            int selCount = frantic::win32::ffListBox_GetSelCount( hwndListBox );
            if( selCount > 0 ) {
                hasSelectedItem = true;
            }
        }

        frantic::max3d::EnableCustButton( GetDlgItem( hwnd, BTN_PARTICLE_OBJECTS_PICK ), !inCreateMode );
        frantic::max3d::EnableCustButton( GetDlgItem( hwnd, BTN_PARTICLE_OBJECTS_ADD ), !inCreateMode );
        frantic::max3d::EnableCustButton( GetDlgItem( hwnd, BTN_PARTICLE_OBJECTS_REMOVE ),
                                          hasSelectedItem && !inCreateMode );
        frantic::max3d::EnableCustButton( GetDlgItem( hwnd, BTN_PARTICLE_OBJECTS_OPTIONS ), !inCreateMode );
    }

  public:
    particle_objects_dlg_proc()
        : m_obj( 0 )
        , m_hwnd( 0 ) {}

    particle_objects_dlg_proc( Frost* obj )
        : m_obj( obj )
        , m_hwnd( 0 ) {}

    INT_PTR DlgProc( TimeValue t, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
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
                HWND hwndButton = GetDlgItem( hwnd, BTN_PARTICLE_OBJECTS_PICK );
                if( hwndButton ) {
                    ICustButton* iBut = GetICustButton( hwndButton );
                    if( iBut ) {
                        iBut->SetType( CBT_CHECK );
                        iBut->SetHighlightColor( GREEN_WASH );
                        ReleaseICustButton( iBut );
                    }
                }

                int fastForwardButtons[] = { BTN_PARTICLE_OBJECTS_OPTIONS };
                BOOST_FOREACH( int idc, fastForwardButtons ) {
                    GetFrostGuiResources()->apply_custbutton_fast_forward_icon( GetDlgItem( hwnd, idc ) );
                    CustButton_SetRightClickNotify( GetDlgItem( hwnd, idc ), true );
                }

                typedef std::pair<int, int> control_tooltip_t;
                control_tooltip_t buttonTooltips[] = {
                    std::make_pair( BTN_PARTICLE_OBJECTS_PICK, IDS_PARTICLE_OBJECTS_PICK_TOOLTIP ),
                    std::make_pair( BTN_PARTICLE_OBJECTS_ADD, IDS_PARTICLE_OBJECTS_ADD_TOOLTIP ),
                    std::make_pair( BTN_PARTICLE_OBJECTS_REMOVE, IDS_PARTICLE_OBJECTS_REMOVE_TOOLTIP ),
                    std::make_pair( BTN_PARTICLE_OBJECTS_OPTIONS, IDS_PARTICLE_OBJECTS_OPTIONS_TOOLTIP ) };
                BOOST_FOREACH( const control_tooltip_t& buttonTooltip, buttonTooltips ) {
                    CustButton_SetTooltip( GetDlgItem( hwnd, buttonTooltip.first ), GetString( buttonTooltip.second ) );
                }
                update_node_list( hwnd );
                update_control_enable( hwnd );
                m_inplaceTooltip.attach( GetDlgItem( hwnd, LB_PARTICLE_OBJECTS ) );
            } break;
            case WM_DESTROY:
                m_inplaceTooltip.detach( GetDlgItem( hwnd, LB_PARTICLE_OBJECTS ) );
                break;
            case WM_COMMAND:
                switch( id ) {
                case BTN_PARTICLE_OBJECTS_PICK:
                    if( m_obj->ip ) {
                        m_obj->ip->SetPickMode( GetPickParticleObjectCallback( m_obj, hwnd ) );
                    }
                    break;
                case BTN_PARTICLE_OBJECTS_ADD: {
                    if( m_obj->ip ) {
                        add_particle_object_by_name_callback callback( m_obj );
                        m_obj->ip->DoHitByNameDialog( &callback );
                    }
                } break;
                case BTN_PARTICLE_OBJECTS_REMOVE: {
                    HWND hwndListBox = GetDlgItem( hwnd, LB_PARTICLE_OBJECTS );
                    if( hwndListBox ) {
                        int selCount = frantic::win32::ffListBox_GetSelCount( hwndListBox );
                        if( selCount > 0 ) {
                            std::vector<int> selection;
                            frantic::win32::ffListBox_GetSelItems( hwndListBox, selection );
                            std::sort( selection.begin(), selection.end(), std::greater<int>() );
                            if( !theHold.Holding() ) {
                                theHold.Begin();
                            }
                            for( std::size_t i = 0; i < selection.size(); ++i ) {
                                m_obj->remove_particle_object( selection[i] );
                            }
                            theHold.Accept( ::GetString( IDS_REMOVE_PARTICLE_OBJECTS ) );
                        }
                    }
                } break;
                case BTN_PARTICLE_OBJECTS_OPTIONS:
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_NodeListOptions_pressed frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                    break;
                case BTN_PARTICLE_OBJECTS_FORCE_UPDATE:
                    break;
                case LB_PARTICLE_OBJECTS:
                    switch( notifyCode ) {
                    case LBN_DBLCLK:
                        notify_if_missing_frost_mxs();
                        frantic::max3d::mxs::expression(
                            _T("if (FrostUi!=undefined) do (FrostUi.on_NodeList_doubleClicked frostNode)") )
                            .bind( _T("frostNode"), m_obj )
                            .at_time( t )
                            .evaluate<void>();
                        break;
                    case LBN_SELCHANGE:
                        update_control_enable( hwnd );
                        break;
                    case LBN_SELCANCEL:
                        update_control_enable( hwnd );
                        break;
                    }
                    break;
                }
                break;
            case WM_CONTEXTMENU:
                if( handle_listbox_contextmenu( GetDlgItem( hwnd, LB_PARTICLE_OBJECTS ), lParam ) ) {
                    update_control_enable( hwnd );
                    notify_if_missing_frost_mxs();
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) do (FrostUi.on_NodeList_rightClicked frostNode)") )
                        .bind( _T("frostNode"), m_obj )
                        .at_time( t )
                        .evaluate<void>();
                }
                break;
            }
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("particle_objects_dlg_proc::DlgProc: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
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
            // update_control_enable( m_hwnd );
        } catch( const std::exception& e ) {
            frantic::tstring errmsg =
                _T("particle_objects_dlg_proc::Update: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            FF_LOG( error ) << errmsg << std::endl;
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
    }

    void get_node_list_selection( std::vector<int>& outSelection ) {
        outSelection.clear();
        if( ( !m_obj ) || ( m_obj != m_obj->editObj ) ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_particle_objects_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }
        const HWND hwndNodeList = GetDlgItem( hwnd, LB_PARTICLE_OBJECTS );
        std::vector<int> result;
        get_list_box_selection_mxs( hwndNodeList, result );
        outSelection.swap( result );
    }

    void set_node_list_selection( const std::vector<int>& selection ) {
        if( ( !m_obj ) || ( m_obj != m_obj->editObj ) ) {
            return;
        }
        IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
        if( !pb ) {
            return;
        }
        IParamMap2* pm = pb->GetMap( frost_particle_objects_param_map );
        if( !pm ) {
            return;
        }
        const HWND hwnd = pm->GetHWnd();
        if( !hwnd ) {
            return;
        }
        const HWND hwndNodeList = GetDlgItem( hwnd, LB_PARTICLE_OBJECTS );
        set_list_box_selection_mxs( hwndNodeList, selection );
        update_control_enable( hwnd );
    }

    void invalidate_node_list_labels() {
        if( m_obj && m_obj->editObj == m_obj && m_hwnd ) {
            IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
            if( !pb ) {
                return;
            }
            IParamMap2* pm = pb->GetMap( frost_particle_objects_param_map );
            if( !pm ) {
                return;
            }
            const HWND hwnd = pm->GetHWnd();
            if( !hwnd ) {
                return;
            }
            HWND hwndCtrl = GetDlgItem( hwnd, LB_PARTICLE_OBJECTS );
            if( hwndCtrl ) {
                std::vector<frantic::tstring> nodeNames;
                m_obj->get_particle_object_names( nodeNames );
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

    void InvalidateUI( HWND hwnd, int element ) {
        if( !hwnd ) {
            return;
        }
        switch( element ) {
        case pb_nodeList:
            if( m_obj && m_obj->editObj == m_obj ) {
                update_node_list( hwnd );
                update_control_enable( hwnd );
            }
            break;
        }
    }

    void DeleteThis() {
        // static !
        // delete this;
    }
};
