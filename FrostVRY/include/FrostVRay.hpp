// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

#define FROST_DLL_EXPORT __declspec( dllexport )

// Forward declarations
class FrostInterface;

namespace VUtils {
class VRenderObject;
} // namespace VUtils

// Note, for 3ds Max 2017 + V-Ray 3.40: as far as I can tell, V-Ray plugins
// must be build with VS2012, while our Frost 3ds Max plugin is built with
// VS2015.  This means that we have a mismatched runtime library, so you must
// be careful about what you pass between the two DLLs.  In particular, you
// must avoid allocating memory in one DLL and freeing it in the other.

typedef VUtils::VRenderObject* frost_vrender_object_ptr_t;

FROST_DLL_EXPORT
frost_vrender_object_ptr_t CreateVRenderObject( FrostInterface* frost );

FROST_DLL_EXPORT
void DestroyVRenderObject( frost_vrender_object_ptr_t obj );

FROST_DLL_EXPORT
boost::uint64_t GetFrostVersionHash();

typedef frost_vrender_object_ptr_t ( *create_vrender_object_function_t )( FrostInterface* );
typedef void ( *destroy_vrender_object_function_t )( frost_vrender_object_ptr_t );
typedef boost::uint64_t ( *get_frost_version_hash_function_t )();

#define CREATE_VRENDER_OBJECT_DECORATED_FUNCTION_NAME                                                                  \
    "?CreateVRenderObject@@YAPEAVVRenderObject@VUtils@@PEAVFrostInterface@@@Z"
#define DESTROY_VRENDER_OBJECT_DECORATED_FUNCTION_NAME "?DestroyVRenderObject@@YAXPEAVVRenderObject@VUtils@@@Z"
#define GET_FROST_VERSION_HASH_DECORATED_FUNCTION_NAME "?GetFrostVersionHash@@YA_KXZ"
