// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "VRayUtils.hpp"

#include <vraycore.h>

#if VRAY_DLL_VERSION >= 0x40000
#include <progress.h>
#endif

void report_error( VUtils::VRayCore* vray, const char* functionName, const char* message ) {
    vray->getSequenceData().progress->error( "%s: %s", functionName, message );
}
