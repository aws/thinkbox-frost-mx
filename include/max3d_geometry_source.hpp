// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "simple_object_accessor.hpp"
#include <frost/geometry_source_interface.hpp>

class max3d_geometry_source : public geometry_source_interface {
  public:
    max3d_geometry_source( simple_object_accessor<INode*>& geometryList );

    virtual bool get_geometry( frantic::geometry::trimesh3& outMesh, double timeInSeconds, std::size_t shapeNumber );
    virtual std::size_t get_geometry_count();

  private:
    simple_object_accessor<INode*>& m_geometryList;

    max3d_geometry_source& operator=( const max3d_geometry_source& ); // not implemented
};
