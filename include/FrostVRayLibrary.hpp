// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FrostVRY/include/FrostVRay.hpp"
#include "FrostVersionHash.hpp"
#include "scoped_hmodule.hpp"

class FrostVRayLibrary {
  public:
    typedef boost::shared_ptr<FrostVRayLibrary> ptr_type;

    FrostVRayLibrary( const std::wstring& filename );

    VUtils::VRenderObject* CreateVRenderObject( FrostInterface* frost );
    void DestroyVRenderObject( VUtils::VRenderObject* obj );

  private:
    scoped_hmodule m_module;
    create_vrender_object_function_t m_createVRenderObject;
    destroy_vrender_object_function_t m_destroyVRenderObject;
};

FrostVRayLibrary::ptr_type LoadFrostVRayLibrary();
