// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

inline bool handle_listbox_contextmenu( HWND hwndListBox, LPARAM lParam ) {
    int xPos = GET_X_LPARAM( lParam );
    int yPos = GET_Y_LPARAM( lParam );
    RECT rc;
    GetWindowRect( hwndListBox, &rc );
    if( xPos >= rc.left && xPos < rc.right && yPos >= rc.top && yPos < rc.bottom ) {
        int xPosLocal = xPos - rc.left;
        int yPosLocal = yPos - rc.top;
        LRESULT result = SendMessage( hwndListBox, LB_ITEMFROMPOINT, 0, MAKELPARAM( xPosLocal, yPosLocal ) );
        int index = LOWORD( result );
        int isOutside = HIWORD( result );
        if( !isOutside ) {
            LRESULT isSelected = SendMessage( hwndListBox, LB_GETSEL, index, 0 );
            if( !isSelected ) {
                SendMessage( hwndListBox, LB_SETSEL, FALSE, -1 );
                SendMessage( hwndListBox, LB_SETSEL, TRUE, index );
            }
            return true;
        }
    }
    return false;
}
