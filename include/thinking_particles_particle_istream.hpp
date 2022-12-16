// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_map.hpp>

#include <frantic/max3d/time.hpp>

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

class thinking_particles_particle_istream : public frantic::particles::streams::particle_istream {
  private:
    frantic::tstring m_name;
    frantic::channels::channel_map m_nativeMap;
    frantic::channels::channel_map m_outMap;

    INode* m_pNode;
    TimeValue m_time;
    IParticleObjectExt* m_pParticles;
    int m_currentIndex;
    int m_totalIndex;
    int m_currentParticle;
    int m_totalParticles;

    float m_fps;

    std::vector<char> m_defaultParticleBuffer;

    typedef frantic::graphics::vector4f vector4f;
    typedef frantic::graphics::vector3f vector3f;
    typedef frantic::graphics::color3f color3f;

    struct {
        bool hasVelocity;
        bool hasScale;
        bool hasOrientation;
        bool hasSpin;
        bool hasAge;
        bool hasLifespan;
        bool hasID;

        frantic::channels::channel_accessor<vector3f> position;
        frantic::channels::channel_cvt_accessor<vector3f> velocity;
        frantic::channels::channel_cvt_accessor<vector3f> scale;
        frantic::channels::channel_cvt_accessor<vector4f> orientation;
        frantic::channels::channel_cvt_accessor<vector4f> spin;
        frantic::channels::channel_cvt_accessor<double> age;
        frantic::channels::channel_cvt_accessor<double> lifespan;
        frantic::channels::channel_cvt_accessor<int> id;
    } m_accessors;

  private:
    void init_stream( INode* pNode, TimeValue t ) {
        m_pNode = pNode;
        m_time = t;
        m_name = pNode->GetName();

        ObjectState os = pNode->EvalWorldState( t );
        m_pParticles = GetParticleObjectExtInterface( os.obj );
        if( !m_pParticles )
            throw std::runtime_error( "thinking_particles_particle_istream::init_stream() - The node: " +
                                      frantic::strings::to_string( m_name ) +
                                      " did not have the interface IParticleObjExt" );

        m_pParticles->UpdateParticles( pNode, t );

        // Thinking Particles messed up the Spin & Velocity channels. They are supposed to be per-frame according to the
        // Max header documentation but Cebas has made them per tick. m_fps = static_cast<float>( GetFrameRate() );
        m_fps = static_cast<float>( TIME_TICKSPERSEC );

        m_currentParticle = -1;
        m_currentIndex = -1;
        m_totalIndex = m_pParticles->NumParticles();

        m_nativeMap.define_channel<vector3f>( _T("Position") );
        m_nativeMap.define_channel<vector3f>( _T("Velocity") );
        m_nativeMap.define_channel<vector3f>( _T("Scale") );
        m_nativeMap.define_channel<vector4f>( _T("Orientation") );
        m_nativeMap.define_channel<vector4f>( _T("Spin") );
        m_nativeMap.define_channel<double>( _T("Age") );
        m_nativeMap.define_channel<double>( _T("LifeSpan") );
        m_nativeMap.define_channel<int>( _T("ID") );
        m_nativeMap.end_channel_definition();
    }

    void init_accessors( const frantic::channels::channel_map& pcm ) {
        m_accessors.position = pcm.get_accessor<vector3f>( _T("Position") );

        if( m_accessors.hasVelocity = pcm.has_channel( _T("Velocity") ) )
            m_accessors.velocity = pcm.get_cvt_accessor<vector3f>( _T("Velocity") );

        if( m_accessors.hasScale = pcm.has_channel( _T("Scale") ) )
            m_accessors.scale = pcm.get_cvt_accessor<vector3f>( _T("Scale") );

        if( m_accessors.hasOrientation = pcm.has_channel( _T("Orientation") ) )
            m_accessors.orientation = pcm.get_cvt_accessor<vector4f>( _T("Orientation") );

        if( m_accessors.hasSpin = pcm.has_channel( _T("Spin") ) )
            m_accessors.spin = pcm.get_cvt_accessor<vector4f>( _T("Spin") );

        if( m_accessors.hasAge = pcm.has_channel( _T("Age") ) )
            m_accessors.age = pcm.get_cvt_accessor<double>( _T("Age") );

        if( m_accessors.hasLifespan = pcm.has_channel( _T("LifeSpan") ) )
            m_accessors.lifespan = pcm.get_cvt_accessor<double>( _T("LifeSpan") );

        if( m_accessors.hasID = pcm.has_channel( _T("ID") ) )
            m_accessors.id = pcm.get_cvt_accessor<int>( _T("ID") );
    }

  public:
    thinking_particles_particle_istream( INode* pNode, TimeValue t ) {
        init_stream( pNode, t );
        set_channel_map( m_nativeMap );
    }

    thinking_particles_particle_istream( INode* pNode, TimeValue t, const frantic::channels::channel_map& pcm ) {
        init_stream( pNode, t );
        set_channel_map( pcm );
    }

    virtual ~thinking_particles_particle_istream() {}

    static bool can_get_from_node( INode* pNode ) {
        if( pNode ) {
            Object* pObj = pNode->GetObjectRef();
            if( pObj ) {
                if( pObj->FindBaseObject()->ClassID() == Class_ID( 0x0490E5A33, 0x045DA39CF ) ) {
                    return true;
                }
            }
        }
        return false;
    }

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        std::vector<char> newDefaultParticle( pcm.structure_size() );
        if( m_defaultParticleBuffer.size() > 0 ) {
            frantic::channels::channel_map_adaptor defaultAdaptor( pcm, m_outMap );
            defaultAdaptor.copy_structure( &newDefaultParticle[0], &m_defaultParticleBuffer[0] );
        } else
            memset( &newDefaultParticle[0], 0, pcm.structure_size() );
        m_defaultParticleBuffer.swap( newDefaultParticle );

        m_outMap = pcm;
        init_accessors( pcm );
    }

    void set_default_particle( char* buffer ) {
        memcpy( &m_defaultParticleBuffer[0], buffer, m_outMap.structure_size() );
    }

    void close() {}

    frantic::tstring name() const { return m_name; }
    std::size_t particle_size() const { return m_outMap.structure_size(); }
    boost::int64_t particle_count() const { return -1; }
    boost::int64_t particle_index() const { return m_currentParticle; }
    boost::int64_t particle_count_left() const { return -1; }
    boost::int64_t particle_progress_count() const { return m_totalIndex; }
    boost::int64_t particle_progress_index() const { return m_currentIndex; }

    const frantic::channels::channel_map& get_channel_map() const { return m_outMap; }
    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }

    bool get_particle( char* buffer ) {
        if( m_currentIndex < 0 ) {
            m_pParticles = GetParticleObjectExtInterface( m_pNode->GetObjectRef()->FindBaseObject() );

            m_pParticles->UpdateParticles( m_pNode, m_time );
            m_totalIndex = m_pParticles->NumParticles();
        }

        if( ++m_currentIndex >= m_totalIndex )
            return false;

        while( m_pParticles->GetParticleAgeByIndex( m_currentIndex ) < 0 ) {
            if( ++m_currentIndex >= m_totalIndex )
                return false;
        }

        ++m_currentParticle;

        memcpy( buffer, &m_defaultParticleBuffer[0], m_outMap.structure_size() ); // TODO: optimize

        m_accessors.position.get( buffer ) =
            frantic::max3d::from_max_t( *m_pParticles->GetParticlePositionByIndex( m_currentIndex ) );
        if( m_accessors.hasVelocity )
            m_accessors.velocity.set(
                buffer, frantic::max3d::from_max_t( *m_pParticles->GetParticleSpeedByIndex( m_currentIndex ) *
                                                    m_fps ) ); // Units per frame to units per sec.
        if( m_accessors.hasScale )
            m_accessors.scale.set(
                buffer, frantic::max3d::from_max_t( *m_pParticles->GetParticleScaleXYZByIndex( m_currentIndex ) ) );
        if( m_accessors.hasAge )
            m_accessors.age.set(
                buffer, frantic::max3d::to_seconds<double>( m_pParticles->GetParticleAgeByIndex( m_currentIndex ) ) );
        if( m_accessors.hasLifespan )
            m_accessors.lifespan.set( buffer, frantic::max3d::to_seconds<double>(
                                                  m_pParticles->GetParticleLifeSpanByIndex( m_currentIndex ) ) );
        // if(m_accessors.hasID) m_accessors.id.set(buffer, m_pParticles->GetParticleBornIndex(m_currentIndex));
        if( m_accessors.hasID )
            m_accessors.id.set( buffer, m_currentIndex );

        if( m_accessors.hasSpin ) {
            AngAxis* a = m_pParticles->GetParticleSpinByIndex( m_currentIndex );
            m_accessors.spin.set( buffer,
                                  QFromAngAxis( a->angle * m_fps, a->axis ) ); // radians per frame to radians per sec
        }

        if( m_accessors.hasOrientation ) {
            Quat q;
            EulerToQuat( *m_pParticles->GetParticleOrientationByIndex( m_currentIndex ), q );
            m_accessors.orientation.set( buffer, q );
        }

        return true;
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
