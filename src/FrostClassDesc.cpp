// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "FrostClassDesc.hpp"

ClassDesc2* GetFrostClassDesc() {
    static FrostClassDesc frostClassDesc;
    return &frostClassDesc;
}
