// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "frost_param_block_desc.hpp"

extern TCHAR* GetString( int id );

class pick_geometry_node_callback : public PickModeCallback, PickNodeCallback {
    Frost* m_obj;
    HWND m_hwnd;
    TimeValue m_t;

  public:
    pick_geometry_node_callback()
        : m_obj( 0 )
        , m_hwnd( 0 )
        , m_t( 0 ) {}
    pick_geometry_node_callback( Frost* obj, HWND hwnd )
        : m_obj( obj )
        , m_hwnd( hwnd )
        , m_t( 0 ) {}
    void reset( Frost* obj, HWND hwnd ) {
        m_obj = obj;
        m_hwnd = hwnd;
        m_t = 0;
    }
    BOOL HitTest( IObjParam* ip, HWND hWnd, ViewExp* /*vpt*/, IPoint2 m, int /*flags*/ ) {
        m_t = ip->GetTime();
        // TRUE if something was hit
        // particle_node_filter_callback filter( m_obj );
        return ip->PickNode( hWnd, m, this ) ? TRUE : FALSE;
    }
    BOOL Pick( IObjParam* ip, ViewExp* vpt ) {
        // TRUE : end pick mode, FALSE : continue pick mode
        INode* node = vpt->GetClosestHit();
        if( node ) {
            if( m_obj ) {
                if( !theHold.Holding() ) {
                    theHold.Begin();
                }
                m_obj->add_geometry( ip->GetTime(), node );
                theHold.Accept( ::GetString( IDS_ADD_GEOMETRY ) );
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

        ICustButton* iBut = GetICustButton( GetDlgItem( hwnd, IDC_GEOMETRY_PICK ) );
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

        ICustButton* iBut = GetICustButton( GetDlgItem( hwnd, IDC_GEOMETRY_PICK ) );
        if( iBut )
            iBut->SetCheck( FALSE );
        ReleaseICustButton( iBut );
    }
    BOOL Filter( INode* node ) {
        if( m_obj && node && m_obj->accept_geometry( m_t, node ) ) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    PickNodeCallback* GetFilter() { return this; }
};

pick_geometry_node_callback* GetPickGeometryNodeCallback( Frost* obj, HWND hwnd );
