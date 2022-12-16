// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <vraybase.h>

#include <rayserver.h>

#if defined(tstring)
#undef tstring
#endif

#if VRAY_DLL_VERSION < 0x40000
typedef VR::Vector vray_vector_t;
typedef VR::TracePoint trace_point_t;
typedef VR::TraceTransform trace_transform_t;
typedef VR::RayServerInterface2 ray_server_t;
#else
typedef VR::ShadeVec vray_vector_t;
typedef VR::ShadeVec trace_point_t;
typedef VR::simd::Transform3x4f trace_transform_t;
typedef VR::RayServerInterface ray_server_t;
#endif
