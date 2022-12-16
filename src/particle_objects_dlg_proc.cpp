// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "particle_objects_dlg_proc.hpp"

pick_particle_object_callback* GetPickParticleObjectCallback( Frost* obj, HWND hwnd ) {
    static pick_particle_object_callback pickParticleObjectCallback;
    pickParticleObjectCallback.reset( obj, hwnd );
    return &pickParticleObjectCallback;
}
