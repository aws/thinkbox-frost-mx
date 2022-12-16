// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/foreach.hpp>

#include <frantic/channels/channel_map_adaptor.hpp>
#include <frantic/channels/channel_propagation_policy.hpp>
#include <frantic/math/utils.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

class convert_byte_color_to_float_particle_istream : public frantic::particles::streams::particle_istream {
  protected:
    boost::shared_ptr<particle_istream> m_delegate;
    frantic::channels::channel_map m_nativeChannelMap;
    frantic::channels::channel_map m_channelMap;
    std::vector<char> m_delegateParticleBuffer;
    frantic::channels::channel_map_adaptor m_adaptor;
    float m_scale;

    struct channel_io_accessors {
        frantic::channels::channel_general_accessor in; // boost::uint8[3]
        frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> out;
        channel_io_accessors( const frantic::channels::channel_general_accessor& in,
                              const frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f>& out )
            : in( in )
            , out( out ) {}
    };
    std::vector<channel_io_accessors> m_convertChannelAccessors;

    frantic::tstring get_unique_channel_name( const frantic::channels::channel_map& channelMapA,
                                              const frantic::channels::channel_map& channelMapB ) {
        frantic::tstring randomChannelName = _T("__Tmp");
        for( boost::uint32_t i = 0; i < std::numeric_limits<boost::uint32_t>::max(); ++i ) {
            const frantic::tstring channelName = _T("__Tmp") + boost::lexical_cast<frantic::tstring>( i );
            if( !channelMapA.has_channel( channelName ) && !channelMapB.has_channel( channelName ) ) {
                return channelName;
            }
        }
        throw std::runtime_error( "get_unique_channel_name Error: could not find unique channel name" );
    }

    frantic::channels::channel_map_adaptor
    get_channel_map_adaptor( const frantic::channels::channel_map& dest, const frantic::channels::channel_map& src,
                             const frantic::channels::channel_propagation_policy& cpp ) {
        frantic::channels::channel_map tempDest( dest );
        for( std::size_t i = 0; i < tempDest.channel_count(); ++i ) {
            const frantic::tstring channelName = tempDest[i].name();
            if( !cpp.is_channel_included( channelName ) ) {
                const frantic::tstring tempChannelName = get_unique_channel_name( tempDest, src );
                tempDest.rename_channel( channelName, tempChannelName );
            }
        }
        return frantic::channels::channel_map_adaptor( tempDest, src );
    }

  public:
    convert_byte_color_to_float_particle_istream( boost::shared_ptr<particle_istream> pin )
        : m_delegate( pin )
        , m_scale( 1.f / 255 ) {
        const frantic::channels::channel_map& delegateNativeChannelMap = pin->get_native_channel_map();

        for( std::size_t i = 0; i < delegateNativeChannelMap.channel_count(); ++i ) {
            const frantic::channels::channel& ch = delegateNativeChannelMap[i];
            if( ch.name() == _T("Color") && ch.arity() == 3 && ch.data_type() == frantic::channels::data_type_uint8 ) {
                m_nativeChannelMap.define_channel( ch.name(), ch.arity(),
                                                   frantic::channels::channel_data_type_traits<half>::data_type() );
            } else {
                m_nativeChannelMap.define_channel( ch.name(), ch.arity(), ch.data_type() );
            }
        }
        m_nativeChannelMap.end_channel_definition( 1, true, true );
        set_channel_map( m_nativeChannelMap );
    }

    virtual ~convert_byte_color_to_float_particle_istream() {}

    void close() { m_delegate->close(); }
    std::size_t particle_size() const { return m_channelMap.structure_size(); }
    boost::int64_t particle_count() const { return m_delegate->particle_count(); }
    boost::int64_t particle_index() const { return m_delegate->particle_index(); }
    boost::int64_t particle_count_left() const { return m_delegate->particle_count_left(); }
    boost::int64_t particle_progress_count() const { return m_delegate->particle_progress_count(); }
    boost::int64_t particle_progress_index() const { return m_delegate->particle_progress_index(); }
    boost::int64_t particle_count_guess() const { return m_delegate->particle_count_guess(); }
    frantic::tstring name() const { return m_delegate->name(); }

    void set_channel_map( const frantic::channels::channel_map& particleChannelMap ) {
        m_channelMap = particleChannelMap;

        frantic::channels::channel_map delegateChannelMap;
        for( std::size_t i = 0; i < particleChannelMap.channel_count(); ++i ) {
            const frantic::channels::channel& ch = particleChannelMap[i];
            if( ch.name() == _T("Color") && ch.arity() == 3 ) {
                delegateChannelMap.define_channel(
                    ch.name(), ch.arity(), frantic::channels::channel_data_type_traits<boost::uint8_t>::data_type() );
            } else {
                delegateChannelMap.define_channel( ch.name(), ch.arity(), ch.data_type() );
            }
        }
        delegateChannelMap.end_channel_definition( 1, true, true );
        m_delegate->set_channel_map( delegateChannelMap );
        m_delegateParticleBuffer.resize( delegateChannelMap.structure_size() );

        frantic::channels::channel_propagation_policy cpp;
        cpp.add_channel( _T("Color") );

        m_adaptor = get_channel_map_adaptor( particleChannelMap, delegateChannelMap, cpp );

        m_convertChannelAccessors.clear();
        for( std::size_t i = 0; i < particleChannelMap.channel_count(); ++i ) {
            const frantic::tstring& channelName = particleChannelMap[i].name();
            if( !cpp.is_channel_included( channelName ) ) {
                frantic::channels::channel_general_accessor in = delegateChannelMap.get_general_accessor( channelName );
                frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> out =
                    particleChannelMap.get_cvt_accessor<frantic::graphics::vector3f>( channelName );
                m_convertChannelAccessors.push_back( channel_io_accessors( in, out ) );
            }
        }
    }

    void set_default_particle( char* rawParticleBuffer ) {
        frantic::channels::channel_propagation_policy cpp;
        cpp.add_channel( _T("Color") );
        frantic::channels::channel_map_adaptor adaptor =
            get_channel_map_adaptor( m_delegate->get_channel_map(), get_channel_map(), cpp );

        std::vector<char> delegateChannelBuffer( m_delegate->get_channel_map().structure_size(), 0 );
        if( delegateChannelBuffer.size() ) {
            adaptor.copy_structure( &delegateChannelBuffer[0], rawParticleBuffer );
            BOOST_FOREACH( channel_io_accessors& accessors, m_convertChannelAccessors ) {
                boost::uint8_t* delegateDefaultColor =
                    reinterpret_cast<boost::uint8_t*>( accessors.in.get_channel_data_pointer( delegateChannelBuffer ) );
                const frantic::graphics::vector3f defaultColor = accessors.out.get( rawParticleBuffer );
                for( int i = 0; i < 3; ++i ) {
                    delegateDefaultColor[i] = static_cast<boost::uint8_t>( frantic::math::clamp<int>(
                        static_cast<int>( frantic::math::round( 255.f * defaultColor[i] ) ), 0, 255 ) );
                }
            }
        }
        m_delegate->set_default_particle( delegateChannelBuffer );
    }

    const frantic::channels::channel_map& get_channel_map() const { return m_channelMap; }
    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeChannelMap; }

    bool get_particle( char* rawParticleBuffer ) {
        const bool success = m_delegate->get_particle( m_delegateParticleBuffer );

        if( success ) {
            if( m_delegateParticleBuffer.size() ) {
                m_adaptor.copy_structure( rawParticleBuffer, &m_delegateParticleBuffer[0] );
            }

            BOOST_FOREACH( channel_io_accessors& accessors, m_convertChannelAccessors ) {
                boost::uint8_t* v = reinterpret_cast<boost::uint8_t*>(
                    accessors.in.get_channel_data_pointer( m_delegateParticleBuffer ) );
                accessors.out.set( rawParticleBuffer,
                                   frantic::graphics::vector3f( m_scale * v[0], m_scale * v[1], m_scale * v[2] ) );
            }
        }

        return success;
    }

    bool get_particles( char* buffer, std::size_t& numParticles ) {
        const std::size_t particleSize = m_channelMap.structure_size();
        for( std::size_t i = 0; i < numParticles; ++i ) {
            if( !get_particle( buffer + i * particleSize ) ) {
                numParticles = i;
                return false;
            }
        }

        return true;
    }
};
