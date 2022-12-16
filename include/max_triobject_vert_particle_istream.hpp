// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/particles/max3d_particle_streams.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

class max_triobject_vert_particle_istream : public frantic::particles::streams::particle_istream {
  private:
    typedef frantic::graphics::vector3f vector3f;

    TriObject* m_poNow;
    TriObject* m_poFuture;

    bool m_bDeletePONow;
    bool m_bDeletePOFuture;

    INode* m_pNode;
    TimeValue m_time;
    TimeValue m_timeStep;

    int m_index;
    boost::int64_t m_totalParticles;
    std::string m_name;
    float m_interval;

    frantic::channels::channel_map m_nativeMap;
    frantic::channels::channel_map m_outMap;

    std::vector<char> m_defaultParticleBuffer;

    // index of a face that contains the vertex
    // -1 if no faces are incident on the vertex
    std::vector<std::pair<boost::int32_t, boost::int32_t>> m_vertexToFaceAndCornerMap;

    struct {
        bool hasPosition;
        bool hasVelocity;
        bool hasNormal;
        bool hasID;

        frantic::channels::channel_cvt_accessor<vector3f> position;
        frantic::channels::channel_cvt_accessor<vector3f> velocity;
        frantic::channels::channel_cvt_accessor<vector3f> normal;
        frantic::channels::channel_cvt_accessor<int> id;
        std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<vector3f>>> channels;
    } m_accessors;

    void init_accessors( const frantic::channels::channel_map& pcm ) {
        if( m_accessors.hasPosition = pcm.has_channel( "Position" ) )
            m_accessors.position = pcm.get_cvt_accessor<vector3f>( "Position" );
        if( m_accessors.hasVelocity = pcm.has_channel( "Velocity" ) )
            m_accessors.velocity = pcm.get_cvt_accessor<vector3f>( "Velocity" );
        if( m_accessors.hasNormal = pcm.has_channel( "Normal" ) )
            m_accessors.normal = pcm.get_cvt_accessor<vector3f>( "Normal" );
        if( m_accessors.hasID = pcm.has_channel( "ID" ) )
            m_accessors.id = pcm.get_cvt_accessor<int>( "ID" );

        m_accessors.channels.clear();

        if( pcm.has_channel( "Color" ) )
            m_accessors.channels.push_back( std::make_pair( 0, pcm.get_cvt_accessor<vector3f>( "Color" ) ) );
        if( pcm.has_channel( "TextureCoord" ) )
            m_accessors.channels.push_back( std::make_pair( 1, pcm.get_cvt_accessor<vector3f>( "TextureCoord" ) ) );

        for( std::size_t i = 0; i < pcm.channel_count(); ++i ) {
            if( pcm[i].name().substr( 0, 7 ) == "Mapping" ) {
                int channel = boost::lexical_cast<int>( pcm[i].name().substr( 7 ) );
                m_accessors.channels.push_back(
                    std::make_pair( channel, pcm.get_cvt_accessor<vector3f>( pcm[i].name() ) ) );
            }
        }
    }

    void build_vertex_to_face_map( const Mesh& mesh ) {
        m_vertexToFaceAndCornerMap.clear();
        m_vertexToFaceAndCornerMap.resize( mesh.numVerts, std::make_pair( -1, -1 ) );

        for( int faceIndex = 0; faceIndex < mesh.numFaces; ++faceIndex ) {
            const Face face = mesh.faces[faceIndex];
            for( int corner = 0; corner < 3; ++corner ) {
                const int vertIndex = face.v[corner];
                if( vertIndex >= 0 && vertIndex < m_vertexToFaceAndCornerMap.size() ) {
                    if( m_vertexToFaceAndCornerMap[vertIndex].first == -1 ) {
                        m_vertexToFaceAndCornerMap[vertIndex] = std::make_pair( faceIndex, corner );
                    }
                }
            }
        }
    }

    void init_stream( INode* pNode, TimeValue t, TimeValue timeStep ) {
        m_pNode = pNode;
        m_time = t;
        m_timeStep = timeStep;

        m_poNow = m_poFuture = NULL;
        m_bDeletePONow = m_bDeletePOFuture = false;

        ObjectState os = pNode->EvalWorldState( t );
        if( !os.obj->CanConvertToType( triObjectClassID ) )
            throw std::runtime_error( "max_triobject_vert_particle_istream::init_stream() - Cannot convert node: \"" +
                                      std::string( pNode->GetName() ) + "\" to a poly object" );

        m_poNow = static_cast<TriObject*>( os.obj->ConvertToType( t, triObjectClassID ) );
        m_bDeletePONow = ( m_poNow != os.obj );
        int nVerts = m_poNow->mesh.getNumVerts();

        build_vertex_to_face_map( m_poNow->mesh );

        m_index = -1;
        m_totalParticles = nVerts;
        m_name = pNode->GetName();
        m_interval = float( timeStep ) / TIME_TICKSPERSEC;

        m_nativeMap.define_channel<vector3f>( "Position" );
        m_nativeMap.define_channel<vector3f>( "Velocity" );
        m_nativeMap.define_channel<vector3f>( "Normal" );
        m_nativeMap.define_channel<int>( "ID" );

        if( os.obj->HasUVW( 0 ) )
            m_nativeMap.define_channel<vector3f>( "Color" );
        if( os.obj->HasUVW( 1 ) )
            m_nativeMap.define_channel<vector3f>( "TextureCoord" );
        for( int i = 2; i < os.obj->NumMapsUsed(); ++i ) {
            if( os.obj->HasUVW( i ) )
                m_nativeMap.define_channel<vector3f>( "Mapping" + boost::lexical_cast<std::string>( i ) );
        }
        m_nativeMap.end_channel_definition();
    }

  public:
    max_triobject_vert_particle_istream( INode* pNode, TimeValue t, TimeValue timeStep ) {
        init_stream( pNode, t, timeStep );
        set_channel_map( m_nativeMap );
    }

    max_triobject_vert_particle_istream( INode* pNode, TimeValue t, TimeValue timeStep,
                                         const frantic::channels::channel_map& pcm ) {
        init_stream( pNode, t, timeStep );
        set_channel_map( pcm );
    }

    virtual ~max_triobject_vert_particle_istream() { close(); }

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        std::vector<char> newDefaultParticle( pcm.structure_size() );
        if( newDefaultParticle.size() ) {
            if( m_defaultParticleBuffer.size() > 0 ) {
                frantic::channels::channel_map_adaptor defaultAdaptor( pcm, m_outMap );
                defaultAdaptor.copy_structure( &newDefaultParticle[0], &m_defaultParticleBuffer[0] );
            } else
                memset( &newDefaultParticle[0], 0, pcm.structure_size() );
        }
        m_defaultParticleBuffer.swap( newDefaultParticle );

        m_outMap = pcm;
        init_accessors( pcm );
    }

    void set_default_particle( char* buffer ) {
        memcpy( &m_defaultParticleBuffer[0], buffer, m_outMap.structure_size() );
    }

    void close() {
        if( m_bDeletePONow ) {
            m_poNow->DeleteThis();
            m_poNow = NULL;
            m_bDeletePONow = false;
        }

        if( m_bDeletePOFuture ) {
            m_poFuture->DeleteThis();
            m_poFuture = NULL;
            m_bDeletePOFuture = false;
        }
    }

    const frantic::channels::channel_map& get_channel_map() const { return m_outMap; }
    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }
    std::string name() const { return m_name; }

    std::size_t particle_size() const { return m_outMap.structure_size(); }
    boost::int64_t particle_count() const { return m_totalParticles; }
    boost::int64_t particle_index() const { return m_index; }
    boost::int64_t particle_count_left() const { return m_totalParticles - m_index - 1; }
    boost::int64_t particle_progress_count() const { return m_totalParticles; }
    boost::int64_t particle_progress_index() const { return m_index; }

    bool get_particle( char* buffer ) {
        // If this is the first call to get_particle, then the meshes need to be re-initialized
        if( !m_poFuture ) {
            ObjectState os = m_pNode->EvalWorldState( m_time + m_timeStep );
            m_poFuture = static_cast<TriObject*>( os.obj->ConvertToType( m_time + m_timeStep, triObjectClassID ) );
            m_bDeletePOFuture = ( m_poFuture != os.obj );

            m_poNow->mesh.buildNormals();
            m_poFuture->mesh.buildNormals();
        }

        if( ++m_index < m_totalParticles ) {
            memcpy( buffer, &m_defaultParticleBuffer[0], m_outMap.structure_size() ); // TODO: optimize

            Mesh& mesh = m_poNow->mesh;
            Mesh& mesh2 = m_poFuture->mesh;

            if( m_accessors.hasPosition )
                m_accessors.position.set( buffer, frantic::max3d::from_max_t( mesh.verts[m_index] ) );
            if( mesh2.numVerts == mesh.numVerts ) {
                if( m_accessors.hasVelocity )
                    m_accessors.velocity.set(
                        buffer,
                        frantic::max3d::from_max_t( ( mesh2.verts[m_index] - mesh.verts[m_index] ) / m_interval ) );
            }
            if( m_accessors.hasNormal )
                m_accessors.normal.set( buffer, frantic::max3d::from_max_t( mesh.getNormal( m_index ) ) );
            if( m_accessors.hasID )
                m_accessors.id.set( buffer, m_index );

            std::pair<boost::int32_t, boost::int32_t> faceAndCorner = m_vertexToFaceAndCornerMap[m_index];
            if( faceAndCorner.first >= 0 && faceAndCorner.second >= 0 ) {
                for( std::size_t i = 0; i < m_accessors.channels.size(); ++i ) {
                    const int mp = m_accessors.channels[i].first;
                    TVFace* tvFaces = mesh.mapFaces( mp );
                    UVVert* uvVerts = mesh.mapVerts( mp );
                    if( tvFaces && uvVerts ) {
                        const DWORD tvertIndex = tvFaces[faceAndCorner.first].getTVert( faceAndCorner.second );
                        const int tvertCount = mesh.getNumMapVerts( mp );
                        if( static_cast<int>( tvertIndex ) < tvertCount ) {
                            UVVert uv = uvVerts[tvertIndex];
                            m_accessors.channels[i].second.set( buffer, frantic::max3d::from_max_t( uv ) );
                        }
                    }
                }
            }

            return true;
        }
        return false;
    }

    bool get_particles( char* buffer, std::size_t& numParticles ) {
        for( std::size_t i = 0; i < numParticles; ++i ) {
            if( !this->get_particle( buffer ) ) {
                numParticles = i;
                return false;
            }
            buffer += m_outMap.structure_size();
        }

        return true;
    }
};

} // namespace streams
} // namespace particles
} // namespace max3d
} // namespace frantic
