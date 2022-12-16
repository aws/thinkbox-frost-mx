// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <frantic/channels/channel_map_adaptor.hpp>
#include <frantic/graphics/vector3f.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <frantic/max3d/convert.hpp>

class get_legacy_thinking_particles_radius_particle_istream
    : public frantic::particles::streams::delegated_particle_istream {
    IParticleObjectExt* m_particles;
    frantic::channels::channel_cvt_accessor<float> m_radiusAccessor;
    frantic::channels::channel_cvt_accessor<int> m_idAccessor;

    frantic::tstring m_radiusChannelName;
    frantic::tstring m_idChannelName;

    frantic::channels::channel_map m_nativeMap; // May have to add to the native map;

    // When an argument references a channel not specified in the user-supplied channel
    // map, it will request a modified channel map from the delegate with the extra channels.
    frantic::channels::channel_map m_outMap;
    frantic::channels::channel_map_adaptor m_adaptor;
    boost::scoped_array<char> m_tempBuffer;

    void populate_particle_radius( char* p ) {
        if( !m_radiusAccessor.is_valid() ) {
            return;
        }

        if( !m_idAccessor.is_valid() ) {
            throw std::runtime_error( "get_thinking_particles_radius_particle_istream.populate_particle_radius Error: "
                                      "the particles do not have the required \'" +
                                      frantic::strings::to_string( m_idChannelName ) + "\' ID channel." );
        }

        const int id = m_idAccessor( p );
        float scale = 1.f;
        Point3* pScale = m_particles->GetParticleScaleXYZByIndex( id );
        if( pScale ) {
            const frantic::graphics::vector3f scaleXYZ = frantic::max3d::from_max_t( *pScale );
            scale = scaleXYZ.max_abs_component();
        }
        const float radius = 0.5f * scale * m_particles->GetParticleScaleByIndex( id );

        m_radiusAccessor.set( p, radius );
    }

  public:
    get_legacy_thinking_particles_radius_particle_istream(
        boost::shared_ptr<frantic::particles::streams::particle_istream> pin, IParticleObjectExt* particles,
        const frantic::tstring& idChannelName, const frantic::tstring& radiusChannelName )
        : delegated_particle_istream( pin )
        , m_particles( particles )
        , m_idChannelName( idChannelName )
        , m_radiusChannelName( radiusChannelName ) {
        if( !particles ) {
            throw std::runtime_error( "get_thinking_particles_radius_particle_istream Error: particles is NULL" );
        }

        set_channel_map( m_delegate->get_channel_map() );

        m_nativeMap = m_delegate->get_native_channel_map();
        if( !m_nativeMap.has_channel( radiusChannelName ) ) {
            m_nativeMap.append_channel<float>( radiusChannelName );
        }
    }

    virtual ~get_legacy_thinking_particles_radius_particle_istream() {}

    static bool has_required_channels( boost::shared_ptr<frantic::particles::streams::particle_istream> pin,
                                       const frantic::tstring& idChannelName ) {
        if( pin->get_native_channel_map().has_channel( idChannelName ) ||
            pin->get_channel_map().has_channel( idChannelName ) ) {
            return true;
        }
        return false;
    }

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        frantic::channels::channel_map requested = m_outMap = pcm;
        const frantic::channels::channel_map& nativeMap = m_delegate->get_native_channel_map();

        if( !requested.has_channel( m_idChannelName ) && nativeMap.has_channel( m_idChannelName ) ) {
            const frantic::channels::channel& ch = nativeMap[m_idChannelName];
            requested.append_channel( m_idChannelName, ch.arity(), ch.data_type() );
        }

        m_radiusAccessor.reset();
        if( requested.has_channel( m_radiusChannelName ) ) {
            m_radiusAccessor = requested.get_cvt_accessor<float>( m_radiusChannelName );
        }

        m_idAccessor.reset();
        m_idAccessor = requested.get_cvt_accessor<int>( m_idChannelName );

        m_delegate->set_channel_map( requested );
        m_adaptor.set( m_outMap, requested );
        m_tempBuffer.reset( m_adaptor.is_identity() ? NULL : new char[requested.structure_size()] );
    }

    std::size_t particle_size() const { return m_outMap.structure_size(); }

    const frantic::channels::channel_map& get_channel_map() const { return m_outMap; }

    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }

    void set_default_particle( char* buffer ) {
        if( m_adaptor.is_identity() )
            m_delegate->set_default_particle( buffer );
        else {
            frantic::channels::channel_map_adaptor tempAdaptor( m_delegate->get_channel_map(), m_outMap );

            boost::scoped_ptr<char> pDefault( new char[tempAdaptor.dest_size()] );
            memset( pDefault.get(), 0, tempAdaptor.dest_size() );
            tempAdaptor.copy_structure( pDefault.get(), buffer );

            m_delegate->set_default_particle( pDefault.get() );
        }
    }

    bool get_particle( char* outBuffer ) {
        char* inBuffer = ( m_adaptor.is_identity() ) ? outBuffer : m_tempBuffer.get();

        if( !m_delegate->get_particle( inBuffer ) )
            return false;

        populate_particle_radius( inBuffer );

        if( inBuffer != outBuffer )
            m_adaptor.copy_structure( outBuffer, inBuffer );

        return true;
    }

    bool get_particles( char* buffer, std::size_t& numParticles ) {
        for( std::size_t i = 0; i < numParticles; ++i ) {
            if( !this->get_particle( buffer ) ) {
                numParticles = i;
                return false;
            }
            buffer += particle_size();
        }
        return true;
    }
};
