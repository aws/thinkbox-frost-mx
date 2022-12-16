// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "orientation_look_at_validator.hpp"

orientation_look_at_validator* GetOrientationLookAtValidator() {
    static orientation_look_at_validator orientationLookAtValidator;
    return &orientationLookAtValidator;
}
