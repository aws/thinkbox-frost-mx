// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "geometry_list_validator.hpp"

geometry_list_validator* GetGeometryListValidator() {
    static geometry_list_validator geometryListValidator;
    return &geometryListValidator;
}
