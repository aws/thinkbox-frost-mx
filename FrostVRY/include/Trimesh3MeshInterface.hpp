// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FrostVRayAPIDefs.hpp"

#include <frantic/geometry/trimesh3.hpp>

#include <mesh_file.h>
#include <simplifier.h>

#if defined( tstring )
#undef tstring
#endif
#if defined( min )
#undef min
#endif
#if defined( max )
#undef max
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>

#include <frantic/strings/tstring.hpp>

namespace {

void to_vray( VUtils::FaceTopoData& out, const frantic::graphics::vector3& in ) {
    for( int i = 0; i < 3; ++i ) {
        out.v[i] = in[i];
    }
}

VUtils::Vector to_vray( const frantic::graphics::vector3f& v ) { return VUtils::Vector( v.x, v.y, v.z ); }

vray_vector_t to_vray_vector_t( const frantic::graphics::vector3f& v ) { return vray_vector_t( v.x, v.y, v.z ); }

bool is_map_channel( const frantic::strings::tstring& name, int* outChannelNumber = 0 ) {
    bool isMapChannel = false;
    int channel = 0;
    if( name == _T("Color") ) {
        isMapChannel = true;
        channel = 0;
    } else if( name == _T("TextureCoord") ) {
        isMapChannel = true;
        channel = 1;
    } else if( boost::algorithm::starts_with( name, _T("Mapping") ) ) {
        isMapChannel = true;
        channel = boost::lexical_cast<int>( name.substr( 7 ) );
    }
    if( outChannelNumber ) {
        *outChannelNumber = channel;
    }
    return isMapChannel;
}

} // anonymous namespace

class Trimesh3MeshInterface : public VUtils::MeshInterface {
  public:
    Trimesh3MeshInterface( const frantic::geometry::trimesh3& mesh,
                           const frantic::channels::channel_propagation_policy& cpp, ray_server_t* rayserver ) {
        int numChannels = 2; // verts and faces

        std::vector<frantic::tstring> vertexChannelNames;
        mesh.get_vertex_channel_names( vertexChannelNames );

        BOOST_FOREACH( const frantic::tstring& name, vertexChannelNames ) {
            if( ( is_map_channel( name ) || name == _T("Normal") ) && cpp.is_channel_included( name ) ) {
                ++numChannels;
                frantic::geometry::const_trimesh3_vertex_channel_general_accessor acc =
                    mesh.get_vertex_channel_general_accessor( name );
                if( acc.has_custom_faces() ) {
                    ++numChannels;
                }
            }
        }

        m_mesh.init();
        m_mesh.channels = new VR::MeshChannel[numChannels];
        m_mesh.numChannels = 0;
        m_mesh.index = 0;
        m_mesh.setRayServer( rayserver );

        std::size_t numVerts = mesh.vertex_count();

        VR::MeshChannel& geomChan = m_mesh.channels[m_mesh.numChannels++];
        geomChan.init( sizeof( VR::VertGeomData ), numVerts, VERT_GEOM_CHANNEL, FACE_TOPO_CHANNEL, MF_VERT_CHANNEL );
        VR::VertGeomData* verts = (VR::VertGeomData*)geomChan.data;

        std::size_t numFaces = mesh.face_count();

        VR::MeshChannel& topoChan = m_mesh.channels[m_mesh.numChannels++];
        topoChan.init( sizeof( VR::FaceTopoData ), numFaces, FACE_TOPO_CHANNEL, 0, MF_TOPO_CHANNEL );
        VR::FaceTopoData* faces = (VR::FaceTopoData*)topoChan.data;

        for( std::size_t i = 0; i < numVerts; ++i ) {
            verts[i] = to_vray( mesh.get_vertex( i ) );
        }

        for( std::size_t i = 0; i < numFaces; ++i ) {
            to_vray( faces[i], mesh.get_face( i ) );
        }

        if( mesh.has_vertex_channel( _T("Normal") ) && cpp.is_channel_included( _T("Normal") ) ) {
            frantic::geometry::const_trimesh3_vertex_channel_accessor<frantic::graphics::vector3f> normalAcc;
            normalAcc = mesh.get_vertex_channel_accessor<frantic::graphics::vector3f>( _T("Normal") );

            {
                VR::MeshChannel& normalChan = m_mesh.channels[m_mesh.numChannels++];
                int depChannelID = normalAcc.has_custom_faces() ? VERT_NORMAL_TOPO_CHANNEL : FACE_TOPO_CHANNEL;
                normalChan.init( sizeof( VR::VertGeomData ), normalAcc.size(), VERT_NORMAL_CHANNEL, depChannelID,
                                 MF_VERT_CHANNEL );
                VR::VertGeomData* normals = (VR::VertGeomData*)normalChan.data;
                for( std::size_t i = 0, ie = normalAcc.size(); i < ie; ++i ) {
                    normals[i] = to_vray( normalAcc[i] );
                }
            }

            if( normalAcc.has_custom_faces() ) {
                VR::MeshChannel& normalTopoChan = m_mesh.channels[m_mesh.numChannels++];
                normalTopoChan.init( sizeof( VR::FaceTopoData ), normalAcc.face_count(), VERT_NORMAL_TOPO_CHANNEL, 0,
                                     MF_TOPO_CHANNEL );
                VR::FaceTopoData* normalFaces = (VR::FaceTopoData*)normalTopoChan.data;
                for( std::size_t i = 0, ie = normalAcc.face_count(); i < ie; ++i ) {
                    to_vray( normalFaces[i], normalAcc.face( i ) );
                }
            }
        }

        BOOST_FOREACH( const frantic::tstring& name, vertexChannelNames ) {
            if( !cpp.is_channel_included( name ) ) {
                continue;
            }

            int channel = 0;
            if( is_map_channel( name, &channel ) ) {
                frantic::geometry::const_trimesh3_vertex_channel_accessor<frantic::graphics::vector3f> acc;
                acc = mesh.get_vertex_channel_accessor<frantic::graphics::vector3f>( name );

                const int dataChannelID = VERT_TEX_CHANNEL0 + channel;
                const int topoChannelID =
                    acc.has_custom_faces() ? ( VERT_TEX_TOPO_CHANNEL0 + channel ) : FACE_TOPO_CHANNEL;

                {
                    VR::MeshChannel& dataChan = m_mesh.channels[m_mesh.numChannels++];
                    dataChan.init( sizeof( VUtils::VertGeomData ), acc.size(), dataChannelID, topoChannelID,
                                   MF_VERT_CHANNEL );
                    VUtils::VertGeomData* data = (VUtils::VertGeomData*)dataChan.data;
                    for( std::size_t i = 0, ie = acc.size(); i < ie; ++i ) {
                        data[i] = to_vray( acc[i] );
                    }
                }

                if( acc.has_custom_faces() ) {
                    VR::MeshChannel& topoChan = m_mesh.channels[m_mesh.numChannels++];
                    topoChan.init( sizeof( VR::FaceTopoData ), acc.face_count(), topoChannelID, 0, MF_TOPO_CHANNEL );
                    VUtils::FaceTopoData* faces = (VR::FaceTopoData*)topoChan.data;
                    for( std::size_t i = 0, ie = acc.face_count(); i < ie; ++i ) {
                        to_vray( faces[i], acc.face( i ) );
                    }
                }
            }
        }

        m_mesh.sortChannels();

        m_bbox.init();
        for( std::size_t i = 0; i < numVerts; ++i ) {
            m_bbox += verts[i];
        }
    }

    /// Number of voxels in this mesh for the current frame.
    int getNumVoxels( void ) { return 1; }

    /// Bounding box of the i-th voxel for the current frame.
    VUtils::Box getVoxelBBox( int i ) { return m_bbox; }

    /// Returns the voxel flags for the current frame. This can be either MVF_GEOMETRY_VOXEL to show that the voxel
    /// contains actual geometry, or MVF_PREVIEW_VOXEL to show that a voxel contains preview information (VRayProxy
    /// objects in 3dsmax use these to show a preview in the viewports).
    uint32 getVoxelFlags( int i ) { return MVF_GEOMETRY_VOXEL; }

    /// Returns the i-th voxel for the current frame. When the voxel is no longer needed, releaseVoxel() must be called.
    /// @param i The zero-based index of the voxel.
    /// @param memUsage If non-NULL, the memory taken up by the voxel is stored here.
    /// @return A pointer to the voxel. Call releaseVoxel() when you no longer need it.
    VUtils::MeshVoxel* getVoxel( int i, uint64* memUsage = NULL ) {
        if( memUsage ) {
            *memUsage = 0;
        }
        return &m_mesh;
    }

    /// Releases a voxel obtained with getVoxel(). This must be called for each call to getVoxel().
    /// @param voxel A pointer to a voxel returned by getVoxel().
    /// @param memUsage If non-NULL, the memory that was released is stored here.
    void releaseVoxel( VUtils::MeshVoxel* voxel, uint64* memUsage = NULL ) {
        if( memUsage ) {
            *memUsage = 0;
        }
    }

    // need to implement?
    /// Releases a voxel by its index. The voxel must have been obtained earlier with a corresponding call to
    /// getVoxel().
    /// @param voxelIndex A voxel index.
    /// @param memUsage If non-NULL, the memory that was released is stored here.
    // virtual void releaseVoxel(int voxelIndex, uint64 *memUsage=NULL) {}

    /// Returns the i-th voxel for a specific time index. When the voxel is no longer needed, releaseVoxel() must be
    /// called.
    /// @param i The zero-based index of the voxel.
    /// @param timeFlags Used for motion blur. Lowest 16 bits represent the time index and the highest represent number
    /// of time indices.
    /// @param memUsage If non-NULL, the memory taken up by the voxel is stored here.
    /// @param ctxtDataCallback A callback used to compute the level of detail for the point cloud data. If NULL, no
    /// point cloud data is loaded.
    /// @return A pointer to the voxel. Call releaseVoxel() when you no longer need it.
    VUtils::MeshVoxel* getVoxel( int i, int /*timeFlags*/, uint64* memUsage,
                                 VUtils::GetContextDataCallbackBase* /*ctxtDataCallback*/ ) {
        return getVoxel( i, memUsage );
    }

    /// Bounding box of the i-th voxel, corresponding the given time flags.
    VUtils::Box getVoxelBBox( int i, int /*timeFlags*/ ) { return getVoxelBBox( i ); }

  private:
    VUtils::MeshVoxel m_mesh;
    VUtils::Box m_bbox;
};
