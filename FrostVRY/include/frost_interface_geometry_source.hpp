// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frost/geometry_source_interface.hpp>

#include <frantic/max3d/geometry/mesh_request.hpp>

#include "FrostInterface.hpp"

class frost_interface_geometry_source : public geometry_source_interface {
  public:
    frost_interface_geometry_source( FrostInterface* frost );

    bool get_geometry( frantic::geometry::trimesh3& outMesh, double timeInSeconds, std::size_t shapeNumber );

    std::size_t get_geometry_count();

  private:
    FrostInterface* m_frost;
};

frost_interface_geometry_source::frost_interface_geometry_source( FrostInterface* frost )
    : m_frost( frost ) {

    if( !m_frost ) {
        throw std::runtime_error( "frost_interface_geometry_source Error: frost is NULL" );
    }
}

bool frost_interface_geometry_source::get_geometry( frantic::geometry::trimesh3& outMesh, double timeInSeconds,
                                                    std::size_t shapeNumber ) {
    assert( m_frost );

    TimeValue t( static_cast<int>( timeInSeconds * TIME_TICKSPERSEC ) );

    INode* meshNode = m_frost->get_custom_geometry( shapeNumber );
    if( meshNode ) {
        frantic::max3d::max_interval_t maxInterval;
        frantic::max3d::geometry::get_node_trimesh3( meshNode, t, t, outMesh, maxInterval, 0.5f, false, false, true );
        return true;
    } else {
        return false;
    }
}

std::size_t frost_interface_geometry_source::get_geometry_count() {
    assert( m_frost );

    return m_frost->get_custom_geometry_count();
}
