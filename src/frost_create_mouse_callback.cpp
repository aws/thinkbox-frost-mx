// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "frost_param_block_desc.hpp"

#include "frost_create_mouse_callback.hpp"

/**
 * @param msg MOUSE_POINT click a point MOUSE_MOVE dragging MOUSE_INIT plugin first became current
 * @param point the click number. 0 first, 1 second, etc
 * @param flags state of mouse button and modifier keys
 * @param m 2d point which was clicked
 * @param mat transformation of object relative to construction plane
 * @return CREATE_CONTINUE: continue creation process CREATE_STOP creation terminated normally CREATE_ABORT creation
 * process aborted
 */
int frost_create_mouse_callback::proc( ViewExp* vpt, int msg, int point, int /*flags*/, IPoint2 m, Matrix3& mat ) {
    float r;
    Point3 p1, center;

    if( msg == MOUSE_FREEMOVE ) {
        vpt->SnapPreview( m, m, NULL, SNAP_IN_3D );
    }

    if( !m_obj ) {
        return CREATE_ABORT;
    }

    if( msg == MOUSE_POINT || msg == MOUSE_MOVE ) {
        switch( point ) {
        case 0:
            m_obj->suspendSnap = TRUE;
            m_sp0 = m;
            m_p0 = vpt->SnapPoint( m, m, NULL, SNAP_IN_3D );
            mat.SetTrans( m_p0 );
            break;
        case 1:
            if( msg == MOUSE_MOVE ) {
                p1 = vpt->SnapPoint( m, m, NULL, SNAP_IN_3D );
                r = ( p1 - m_p0 ).Length();
                IParamBlock2* pb = m_obj->GetParamBlockByID( frost_param_block );
                if( !pb ) {
                    return CREATE_ABORT;
                }
                pb->SetValue( pb_iconSize, 0, 2.f * r );
            } else if( msg == MOUSE_POINT ) {
                m_obj->suspendSnap = FALSE;
                return CREATE_STOP;
            }
            break;
        }
    }

    if( msg == MOUSE_ABORT ) {
        return CREATE_ABORT;
    }

    return TRUE;
}

void frost_create_mouse_callback::SetObj( Frost* obj ) { m_obj = obj; }
