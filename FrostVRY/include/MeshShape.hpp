// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/scoped_ptr.hpp>

#include <frantic/channels/channel_propagation_policy.hpp>
#include <frantic/geometry/trimesh3.hpp>

#include "FrostVRayAPIDefs.hpp"
#include "Trimesh3MeshInterface.hpp"

class MeshShape : boost::noncopyable {
  public:
    void init( const frantic::geometry::trimesh3& mesh, const frantic::channels::channel_propagation_policy& cpp,
               ray_server_t* rayserver );

    boost::scoped_ptr<VUtils::MeshInterface> meshInterface;

    // NB: this will be empty if the mesh has no material IDs
    std::vector<boost::uint16_t> materialIDs;

    std::vector<boost::int8_t> edgeVisibility;
};

inline void MeshShape::init( const frantic::geometry::trimesh3& mesh,
                             const frantic::channels::channel_propagation_policy& cpp, ray_server_t* rayserver ) {
    meshInterface.reset( new Trimesh3MeshInterface( mesh, cpp, rayserver ) );

    materialIDs.clear();
    if( mesh.has_face_channel( _T("MaterialID") ) && cpp.is_channel_included( _T("MaterialID") ) ) {
        frantic::geometry::const_trimesh3_face_channel_accessor<boost::uint16_t> acc =
            mesh.get_face_channel_accessor<boost::uint16_t>( _T("MaterialID") );
        materialIDs.resize( acc.size() );
        for( std::size_t i = 0, ie = acc.size(); i < ie; ++i ) {
            materialIDs[i] = acc[i];
        }
    }

    edgeVisibility.clear();
    if( mesh.has_face_channel( _T("FaceEdgeVisibility") ) && cpp.is_channel_included( _T("FaceEdgeVisibility") ) ) {
        frantic::geometry::const_trimesh3_face_channel_accessor<boost::int8_t> acc =
            mesh.get_face_channel_accessor<boost::int8_t>( _T("FaceEdgeVisibility") );
        edgeVisibility.resize( acc.size() );
        for( std::size_t i = 0, ie = acc.size(); i < ie; ++i ) {
            edgeVisibility[i] = acc[i];
        }
    }
}
