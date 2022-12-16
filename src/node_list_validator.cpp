// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "node_list_validator.hpp"

node_list_validator* GetNodeListValidator() {
    static node_list_validator nodeListValidator;
    return &nodeListValidator;
}
