// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "max3d_geometry_source.hpp"

#include <frantic/max3d/geometry/mesh_request.hpp>

max3d_geometry_source::max3d_geometry_source( simple_object_accessor<INode*>& geometryList )
    : m_geometryList( geometryList ) {}

bool max3d_geometry_source::get_geometry( frantic::geometry::trimesh3& outMesh, double timeInSeconds,
                                          std::size_t shapeNumber ) {
    TimeValue t( static_cast<int>( timeInSeconds * TIME_TICKSPERSEC ) );
    INode* meshNode = m_geometryList[static_cast<int>( shapeNumber )].at_time( t );
    if( meshNode ) {
        frantic::max3d::max_interval_t maxInterval;
        frantic::max3d::geometry::get_node_trimesh3( meshNode, t, t, outMesh, maxInterval, 0.5f, false, false, true );
        return true;
    } else {
        return false;
    }
}

std::size_t max3d_geometry_source::get_geometry_count() { return m_geometryList.size(); }
