// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <boost/foreach.hpp>

#include <frantic/geometry/trimesh3.hpp>

namespace {

bool contains_face( const frantic::graphics::boundbox3f& bounds, const frantic::geometry::trimesh3& mesh,
                    std::size_t faceIndex ) {
    using namespace frantic::graphics;

    const vector3 face = mesh.get_face( faceIndex );
    for( int i = 0; i < 3; ++i ) {
        if( !bounds.contains( mesh.get_vertex( face[i] ) ) ) {
            return false;
        }
    }
    return true;
}

} // anonymous namespace

void cull( frantic::geometry::trimesh3& mesh, const frantic::graphics::boundbox3f& bounds ) {
    using namespace frantic::graphics;
    using namespace frantic::geometry;

    std::size_t outIndex = 0;
    std::vector<trimesh3_face_channel_general_accessor> faceAccessors;
    std::vector<frantic::tstring> faceChannelNames;
    mesh.get_face_channel_names( faceChannelNames );
    BOOST_FOREACH( const frantic::tstring& name, faceChannelNames ) {
        faceAccessors.push_back( mesh.get_face_channel_general_accessor( name ) );
    }

    std::vector<trimesh3_vertex_channel_general_accessor> vertexAccessors;
    std::vector<frantic::tstring> vertexChannelNames;
    mesh.get_vertex_channel_names( vertexChannelNames );
    BOOST_FOREACH( const frantic::tstring& name, vertexChannelNames ) {
        trimesh3_vertex_channel_general_accessor acc = mesh.get_vertex_channel_general_accessor( name );
        if( acc.has_custom_faces() ) {
            throw std::runtime_error( "cull Error: vertex channel \"" + frantic::strings::to_string( name ) +
                                      "\" has custom faces, which is not supported by this function." );
        }
        vertexAccessors.push_back( acc );
    }

    for( std::size_t faceIndex = 0, faceIndexEnd = mesh.face_count(); faceIndex < faceIndexEnd; ++faceIndex ) {
        if( contains_face( bounds, mesh, faceIndex ) ) {
            if( faceIndex != outIndex ) {
                mesh.get_face( outIndex ) = mesh.get_face( faceIndex );
                BOOST_FOREACH( trimesh3_face_channel_general_accessor& acc, faceAccessors ) {
                    memcpy( acc.data( outIndex ), acc.data( faceIndex ), acc.primitive_size() );
                }
            }
            ++outIndex;
        }
    }
    mesh.set_face_count( outIndex );
    BOOST_FOREACH( trimesh3_face_channel_general_accessor& acc, faceAccessors ) {
        acc.set_face_count( outIndex );
    }

    std::vector<int> vertexIndexMap( mesh.vertex_count(), -1 );
    for( std::size_t faceIndex = 0, faceIndexEnd = mesh.face_count(); faceIndex < faceIndexEnd; ++faceIndex ) {
        const vector3 face = mesh.get_face( faceIndex );
        for( int i = 0; i < 3; ++i ) {
            vertexIndexMap[face[i]] = 1;
        }
    }

    std::size_t newVertexCount = 0;
    for( std::size_t i = 0, ie = vertexIndexMap.size(); i < ie; ++i ) {
        if( vertexIndexMap[i] >= 0 ) {
            vertexIndexMap[i] = static_cast<int>( newVertexCount );
            ++newVertexCount;
        }
    }

    for( std::size_t faceIndex = 0, faceIndexEnd = mesh.face_count(); faceIndex < faceIndexEnd; ++faceIndex ) {
        vector3& face = mesh.get_face( faceIndex );
        for( int i = 0; i < 3; ++i ) {
            face[i] = vertexIndexMap[face[i]];
        }
    }

    for( std::size_t i = 0, ie = mesh.vertex_count(); i < ie; ++i ) {
        int targetIndex = vertexIndexMap[i];
        if( targetIndex >= 0 && targetIndex != i ) {
            mesh.get_vertex( targetIndex ) = mesh.get_vertex( i );
            BOOST_FOREACH( trimesh3_vertex_channel_general_accessor& acc, vertexAccessors ) {
                memcpy( acc.data( targetIndex ), acc.data( i ), acc.primitive_size() );
            }
        }
    }

    mesh.set_vertex_count( newVertexCount );
    BOOST_FOREACH( trimesh3_vertex_channel_general_accessor& acc, vertexAccessors ) {
        acc.set_vertex_count( newVertexCount );
    }
}
