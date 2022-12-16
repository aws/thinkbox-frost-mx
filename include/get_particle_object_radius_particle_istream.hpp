// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <boost/numeric/conversion/cast.hpp>

#include <frantic/graphics/vector3f.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <frantic/max3d/convert.hpp>

class get_particle_object_radius_particle_istream : public frantic::particles::streams::delegated_particle_istream {
    ParticleObject* m_particles;
    frantic::channels::channel_cvt_accessor<float> m_radiusAccessor;
    TimeValue m_time;

    frantic::tstring m_radiusChannelName;

    frantic::channels::channel_map m_nativeMap; // May have to add to the native map;

    void populate_particle_radius( char* p, boost::int64_t particleIndex ) {
        const float radius = m_particles->ParticleSize( m_time, boost::numeric_cast<int>( particleIndex ) );
        m_radiusAccessor.set( p, radius );
    }

  public:
    get_particle_object_radius_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin,
                                                 ParticleObject* particles, TimeValue t,
                                                 const frantic::tstring& radiusChannelName )
        : delegated_particle_istream( pin )
        , m_particles( particles )
        , m_time( t )
        , m_radiusChannelName( radiusChannelName ) {
        if( !particles ) {
            throw std::runtime_error( "get_particle_object_radius_particle_istream Error: particles is NULL" );
        }

        set_channel_map( m_delegate->get_channel_map() );

        m_nativeMap = m_delegate->get_native_channel_map();
        if( !m_nativeMap.has_channel( radiusChannelName ) ) {
            m_nativeMap.append_channel<float>( radiusChannelName );
        }
    }

    virtual ~get_particle_object_radius_particle_istream() {}

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        m_delegate->set_channel_map( pcm );

        m_radiusAccessor.reset();
        if( pcm.has_channel( m_radiusChannelName ) ) {
            m_radiusAccessor = pcm.get_cvt_accessor<float>( m_radiusChannelName );
        }
    }

    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }

    bool get_particle( char* p ) {
        if( !m_delegate->get_particle( p ) ) {
            return false;
        }

        const boost::int64_t particleIndex = m_delegate->particle_index();

        populate_particle_radius( p, particleIndex );

        return true;
    }

    bool get_particles( char* buffer, std::size_t& numParticles ) {
        for( std::size_t i = 0; i < numParticles; ++i ) {
            if( !this->get_particle( buffer ) ) {
                numParticles = i;
                return false;
            }
            buffer += m_delegate->get_channel_map().structure_size();
        }

        return true;
    }
};
