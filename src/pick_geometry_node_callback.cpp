// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "geometry_dlg_proc.hpp"

pick_geometry_node_callback* GetPickGeometryNodeCallback( Frost* obj, HWND hwnd ) {
    static pick_geometry_node_callback pickGeometryNodeCallback;
    pickGeometryNodeCallback.reset( obj, hwnd );
    return &pickGeometryNodeCallback;
}
