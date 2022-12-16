// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "geometry_dlg_proc.hpp"

get_orientation_from_node_callback* GetOrientationFromNodeCallback( Frost* obj, HWND hwnd ) {
    static get_orientation_from_node_callback getOrientationFromNodeCallback;
    getOrientationFromNodeCallback.reset( obj, hwnd );
    return &getOrientationFromNodeCallback;
}

get_axis_from_node_callback* GetAxisFromNodeCallback( Frost* obj, HWND hwnd ) {
    static get_axis_from_node_callback getAxisFromNodeCallback;
    getAxisFromNodeCallback.reset( obj, hwnd );
    return &getAxisFromNodeCallback;
}
