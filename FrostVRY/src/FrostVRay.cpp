// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
// VRayFrost.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "FrostVRay.hpp"

#include <boost/make_shared.hpp>

#include "../FrostVersionHash.hpp"
#include "FrostVRenderObject.hpp"

namespace {

// Make sure that function pointer definitions match the functions
void test_function_t() {
    { create_vrender_object_function_t f = &CreateVRenderObject; }

    { destroy_vrender_object_function_t f = &DestroyVRenderObject; }

    { get_frost_version_hash_function_t f = &GetFrostVersionHash; }
}

} // anonymous namespace

frost_vrender_object_ptr_t CreateVRenderObject( FrostInterface* frost ) { return new FrostVRenderObject( frost ); }

void DestroyVRenderObject( frost_vrender_object_ptr_t obj ) { delete obj; }

boost::uint64_t GetFrostVersionHash() { return FROST_VERSION_HASH; }
