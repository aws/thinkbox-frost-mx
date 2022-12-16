// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace VUtils {
class VRayCore;
}

void report_error( VUtils::VRayCore* vray, const char* functionName, const char* message );
