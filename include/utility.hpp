// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_accessor.hpp>
#include <frantic/channels/channel_map.hpp>
#include <frantic/channels/named_channel_data.hpp>
#include <frantic/geometry/trimesh3.hpp>
#include <frantic/graphics/boundbox3.hpp>
#include <frantic/graphics/camera.hpp>
#include <frantic/graphics/vector3f.hpp>
#include <frantic/math/hash.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>

#include <ImathMatrixAlgo.h>
#include <decomp.h>

#include "set_render_in_scope_collection.hpp"

// copied from IMaxKrakatoaPRTObject
// Not required as of IMaxKrakatoaPRTObject v3
// class RenderContextHolderParticleIStream : public frantic::particles::streams::delegated_particle_istream {
//	boost::scoped_ptr<frantic::max3d::particles::IMaxKrakatoaPRTObject::IEvalContext> m_globContext;
//
// public:
//	RenderContextHolderParticleIStream( frantic::max3d::particles::IMaxKrakatoaPRTObject::IEvalContext* globContext,
//boost::shared_ptr<frantic::particles::streams::particle_istream> delegateStream ) 		: delegated_particle_istream(
//delegateStream ), m_globContext( globContext )
//	{}
//
//	virtual bool get_particle( char* rawParticleBuffer ){ return m_delegate->get_particle( rawParticleBuffer ); }
//	virtual bool get_particles( char* buffer, std::size_t& numParticles ){ return m_delegate->get_particles( buffer,
//numParticles ); }
//};

class hold_render_particle_istream : public frantic::particles::streams::delegated_particle_istream {
    boost::shared_ptr<set_render_in_scope_collection> m_setRenderCollection;

  public:
    hold_render_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> delegateStream,
                                  boost::shared_ptr<set_render_in_scope_collection> setRenderCollection )
        : delegated_particle_istream( delegateStream )
        , m_setRenderCollection( setRenderCollection ) {}
    virtual bool get_particle( char* rawParticleBuffer ) { return m_delegate->get_particle( rawParticleBuffer ); }
    virtual bool get_particles( char* buffer, std::size_t& numParticles ) {
        return m_delegate->get_particles( buffer, numParticles );
    }
};

class redirect_stream_in_scope {
    std::basic_ostream<frantic::tchar>* m_stream;
    std::basic_streambuf<frantic::tchar>* m_oldBuff;

    redirect_stream_in_scope()
        : m_stream( 0 )
        , m_oldBuff( 0 ) {}

  public:
    redirect_stream_in_scope( std::basic_ostream<frantic::tchar>& stream,
                              std::basic_streambuf<frantic::tchar>* newBuff )
        : m_stream( 0 )
        , m_oldBuff( 0 ) {
        m_oldBuff = stream.rdbuf( newBuff );
        m_stream = &stream;
    }
    ~redirect_stream_in_scope() {
        if( m_stream ) {
            m_stream->rdbuf( m_oldBuff );
        }
    }
};

inline double get_volume_double( const frantic::graphics::boundbox3& bounds ) {
    if( bounds.is_empty() )
        return 0;
    else
        return double( bounds.xsize() ) * double( bounds.ysize() ) * double( bounds.zsize() );
}

inline frantic::graphics::vector3f scale_vector( const frantic::graphics::vector3f& v, float scale ) {
    return scale * v;
}

inline frantic::graphics::vector3f add_velocity_to_pos( const frantic::graphics::vector3f& pos,
                                                        const frantic::graphics::vector3f& velocity, float scale ) {
    return pos + ( scale * velocity );
}

inline double get_system_scale_checked( int type /*, const std::string & typeStringForErrorMessage*/ ) {
#if MAX_VERSION_MAJOR >= 24
    double scale = GetSystemUnitScale( type );
#else
    double scale = GetMasterScale( type );
#endif
    if( scale < 0 ) {
        throw std::runtime_error( "Unrecognized unit type code (" + boost::lexical_cast<std::string>( type ) +
                                  ")" ); // from unit string: " + typeStringForErrorMessage );
    } else {
        return scale;
    }
}

template <class T>
inline bool is_empty_frame_range( const std::pair<T, T> interval ) {
    if( interval.second < interval.first ) {
        return true;
    } else {
        return false;
    }
}

inline bool accept_orientation_vector_channel( const frantic::channels::channel& ch ) {
    if( frantic::channels::is_channel_data_type_float( ch.data_type() ) && ch.arity() == 3 ) {
        return true;
    } else {
        return false;
    }
}

inline void get_vector_channel_names( boost::shared_ptr<frantic::particles::streams::particle_istream>& pin,
                                      std::vector<frantic::tstring>& out ) {
    out.clear();

    std::set<frantic::tstring> channels;
    frantic::channels::channel_map channelMap = pin->get_channel_map();
    for( std::size_t i = 0; i < channelMap.channel_count(); ++i ) {
        if( accept_orientation_vector_channel( channelMap[i] ) ) {
            channels.insert( channelMap[i].name() );
        }
    }
    std::copy( channels.begin(), channels.end(), std::back_insert_iterator<std::vector<frantic::tstring>>( out ) );
}

bool is_node_particle_flow( INode* node );

// find the referring inodes which support ParticleGroupInterface
void extract_connected_particle_groups( INode* node, std::set<INode*>& groups );
void extract_particle_groups( INode* node, std::set<INode*>& outGroups );

// get thinkingParticles groups
void extract_tp_groups( INode* node, std::set<ReferenceTarget*>& outGroups );

// logic taken from get_mesh_from_inode -- return false if get_mesh_from_inode() would throw an exception
bool can_get_mesh_from_inode( TimeValue t, INode* node );

inline frantic::graphics::vector3f extract_euler_xyz( const Matrix3& tm ) {
    AffineParts parts;

    decomp_affine( tm, &parts );

    Matrix3 m;
    parts.q.MakeMatrix( m );

    const Point3& row1 = ( (const Matrix3)m )[0];
    const Point3& row2 = ( (const Matrix3)m )[1];
    const Point3& row3 = ( (const Matrix3)m )[2];

    Imath::M44f mat( row1.x, row1.y, row1.z, 0.0f, row2.x, row2.y, row2.z, 0.0f, row3.x, row3.y, row3.z, 0.0f, 0, 0, 0,
                     1.0f );
    Imath::V3f rot;
    extractEulerXYZ( mat, rot );

    return frantic::graphics::vector3f( rot.x, rot.y, rot.z );
}

void conform_output_mesh_channel_types( frantic::geometry::trimesh3& mesh );

void CustButton_SetRightClickNotify( HWND hwndButton, bool enable );

void CustButton_SetTooltip( HWND hwndButton, const TCHAR* tooltip, bool enable = true );

void notify_if_missing_frost_mxs();

// indices start at 1 for maxscript
void get_list_box_selection_mxs( HWND hwndListBox, std::vector<int>& out );

// indices start at 1 for maxscript
void set_list_box_selection_mxs( HWND hwndListBox, const std::vector<int>& selection );

frantic::graphics::camera<float> get_default_camera();

frantic::graphics::camera<float> get_camera_from_view( View& view );

inline bool is_degenerate_face( const frantic::graphics::vector3& face ) {
    if( face.x == face.y || face.x == face.z || face.y == face.z ) {
        return true;
    } else {
        return false;
    }
}

template <class remove_vertex_indicator>
void cull_mesh_vertices( frantic::geometry::trimesh3& mesh, remove_vertex_indicator& removeVertexIndicator ) {
    // get vertex channel names
    std::vector<std::string> vertexChannelNames;
    mesh.get_vertex_channel_names( vertexChannelNames );

    // get vertex channel accessors
    std::vector<frantic::geometry::trimesh3_vertex_channel_general_accessor> vertexChannelAccessors;
    vertexChannelAccessors.reserve( vertexChannelNames.size() );
    for( std::size_t i = 0; i < vertexChannelNames.size(); ++i ) {
        vertexChannelAccessors.push_back( mesh.get_vertex_channel_general_accessor( vertexChannelNames[i] ) );
    }

    // keep track of the output vertex number for each input vertex number
    std::vector<std::size_t> outVertexNumber( mesh.vertex_count(), 0 );

    // move kept vertices to the beginning of the arrays
    std::size_t outIndex = 0;
    for( std::size_t vertexIndex = 0; vertexIndex < mesh.vertex_count(); ++vertexIndex ) {
        if( !removeVertexIndicator( vertexIndex ) ) {
            if( outIndex < vertexIndex ) {
                mesh.vertices_ref()[outIndex] = mesh.vertices_ref()[vertexIndex];
                for( std::size_t channelIndex = 0; channelIndex < vertexChannelAccessors.size(); ++channelIndex ) {
                    memcpy( vertexChannelAccessors[channelIndex].data( outIndex ),
                            vertexChannelAccessors[channelIndex].data( vertexIndex ),
                            vertexChannelAccessors[channelIndex].primitive_size() );
                }
            }
            outVertexNumber[vertexIndex] = outIndex;
            ++outIndex;
        }
    }

    // erase the culled vertices
    mesh.set_vertex_count( outIndex );
    for( std::size_t channelIndex = 0; channelIndex < vertexChannelAccessors.size(); ++channelIndex ) {
        vertexChannelAccessors[channelIndex].set_vertex_count( outIndex );
    }

    // fix face vertex numbers
    for( std::size_t faceNumber = 0; faceNumber < mesh.face_count(); ++faceNumber ) {
        frantic::graphics::vector3& faceRef = mesh.faces_ref()[faceNumber];
        for( int corner = 0; corner < 3; ++corner ) {
            faceRef[corner] = outVertexNumber[faceRef[corner]];
        }
    }
}

inline frantic::graphics::vector3f normalize( const frantic::graphics::vector3f& v ) {
    const float magnitudeSquared = v.get_magnitude_squared();
    // TODO perhaps this should test == 0 instead of < 0.000000001f?
    if( magnitudeSquared == 0 )
        return frantic::graphics::vector3f::from_axis( v.get_largest_axis() );
    const float length = sqrtf( magnitudeSquared );
    if( length == 0 )
        return frantic::graphics::vector3f::from_axis( v.get_largest_axis() );
    return frantic::graphics::vector3f( v.x / length, v.y / length, v.z / length );
}

boost::shared_ptr<frantic::particles::streams::particle_istream>
prepare_file_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin );

boost::shared_ptr<frantic::particles::streams::particle_istream>
get_thinking_particles_radius_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin );

// return max_legacy_particle_istream, with a workaround for RealFlow Particle Loader
boost::shared_ptr<frantic::particles::streams::particle_istream>
get_max_legacy_particle_istream( INode* node, TimeValue t, const frantic::channels::channel_map& channelMap );

namespace RENDERER {
enum renderer_id_enum { VRAY };
};

bool is_renderer( RENDERER::renderer_id_enum rendererID );

bool is_render_active( bool defaultValue = true );

void set_channel_map_to_native_channel_map( boost::shared_ptr<frantic::particles::streams::particle_istream> pin );

inline bool is_network_render_server() {
    return GetCOREInterface()->IsNetworkRenderServer();
}
