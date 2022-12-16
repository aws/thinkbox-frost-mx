// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "FrostVRenderObject.hpp"

#include "FrostRenderInstance.hpp"

#include "VRayUtils.hpp"

FrostVRenderObject::FrostVRenderObject( FrostInterface* frost )
    : m_frost( frost ) {}

VUtils::VRenderInstance* FrostVRenderObject::newRenderInstance( INode* node, VUtils::VRayCore* vray, int renderID ) {
    try {
        return new FrostRenderInstance( m_frost, this, node, vray, renderID );
    } catch( const std::exception& e ) {
        report_error( vray, "FrostVRenderObject::newRenderInstance", e.what() );
    }

    return 0;
}

void FrostVRenderObject::deleteRenderInstance( VUtils::VRenderInstance* ri ) { delete ri; }

void FrostVRenderObject::frameBegin( TimeValue t, VUtils::VRayCore* vray ) {}
