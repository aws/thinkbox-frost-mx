// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "IMaxKrakatoaPRTSource.h"

#include <frantic/particles/prt_file_header.hpp>

// copied from KrakatoaProject/MaxPlugin/src/Objects/MaxKrakatoaPRTSource.cpp
class IMaxKrakatoaPRTSourceStreamAdaptor : public std::streambuf {
    IMaxKrakatoaPRTSource::Stream* m_src;
    pos_type m_streamPos;

  public:
    IMaxKrakatoaPRTSourceStreamAdaptor( IMaxKrakatoaPRTSource::Stream* pSrc )
        : m_src( pSrc ) {}

    ~IMaxKrakatoaPRTSourceStreamAdaptor() { m_src->DeleteThis(); }

    virtual int_type uflow() {
        char result;
        int count;
        do {
            count = m_src->Read( &result, 1 );
            if( count == IMaxKrakatoaPRTSource::Stream::eof )
                return traits_type::eof();
        } while( count < 1 );

        m_streamPos += 1;

        return traits_type::to_int_type( result );
    }

    virtual std::streamsize xsgetn( char_type* _Ptr, std::streamsize _Count ) {
        int result;
        for( result = 0; result < _Count; ) {
            int thisCount = m_src->Read( _Ptr, (int)_Count );
            if( thisCount == IMaxKrakatoaPRTSource::Stream::eof )
                break;
            result += thisCount;
        }

        m_streamPos += result;

        return (std::streamsize)result;
    }

    virtual std::streamsize _Xsgetn_s( char_type* _Ptr, size_t /*_Ptr_size*/, std::streamsize _Count ) {
        return xsgetn( _Ptr, _Count );
    }

    virtual pos_type seekoff( off_type _Off, std::ios_base::seekdir _Way, std::ios_base::openmode _Which ) {
        if( _Which == std::ios_base::in && _Way != std::ios_base::end ) {
            if( _Way == std::ios::beg )
                _Off -= m_streamPos; // Convert absolute position into relative position.

            if( _Off < 0 )
                return pos_type( -1 );

            for( off_type i = 0; i < _Off; ++i ) {
                if( traits_type::eof() == sbumpc() )
                    return m_streamPos;
            }

            return m_streamPos;
        }
        return pos_type( -1 );
    }

    virtual pos_type seekpos( off_type _Off, std::ios_base::openmode _Which ) {
        return seekoff( _Off, std::ios_base::beg, _Which );
    }
};

class krakatoa_prt_source_particle_istream : public frantic::particles::streams::particle_istream {
    frantic::particles::particle_array m_particles;
    std::string m_name;
    boost::int64_t m_index;
    frantic::channels::channel_map m_outMap;
    std::vector<char> m_defaultParticle;
    frantic::channels::channel_map_adaptor m_pcmAdaptor;

  public:
    krakatoa_prt_source_particle_istream( INode* node, TimeValue t )
        : m_index( -1 )
        , m_name( "<uninitialized>" ) {
        if( !node ) {
            throw std::runtime_error( "krakatoa_prt_source_particle_istream Error: the input INode is NULL" );
        }
        ObjectState os = node->EvalWorldState( t );
        if( !os.obj ) {
            throw std::runtime_error( "krakatoa_prt_source_particle_istream Error: In node " + m_name +
                                      ": ObjectState obj is NULL" );
        }
        MCHAR* name = node->GetName();
        m_name = name ? name : "<null>";

        if( IMaxKrakatoaPRTSource* pSource =
                static_cast<IMaxKrakatoaPRTSource*>( os.obj->GetInterface( IMaxKrakatoaPRTSource_ID ) ) ) {
            Interval validity;

            IMaxKrakatoaPRTSourceStreamAdaptor adaptor( pSource->CreateStream( t, validity ) );
            std::istream dataStream( &adaptor );

            frantic::particles::prt_file_header prtHeader;
            prtHeader.set_allow_negative_count( true );
            prtHeader.read_header( dataStream, m_name.c_str() );

            m_particles.reset( prtHeader.get_channel_map() );

            // frantic::channels::channel_accessor<vector3f> posAccessor =
            // prtHeader.get_channel_map().get_accessor<frantic::graphics::vector3f>("Position");

            boost::int64_t particleCount = prtHeader.particle_count();
            if( particleCount >= 0 ) {
                m_particles.resize( (std::size_t)particleCount );

                for( frantic::particles::particle_array_iterator it = m_particles.begin(), itEnd = m_particles.end();
                     it != itEnd; ++it ) {
                    dataStream.read( *it, (int)prtHeader.get_channel_map().structure_size() );
                    if( dataStream.eof() )
                        throw std::runtime_error( "krakatoa_prt_source_particle_istream Error: In node " + m_name +
                                                  ": Unexpected EOF before expected number of particles was read" );

                    // m_internalBounds += frantic::max3d::to_max_t( posAccessor.get( *it ) );
                }
            } else {
                std::size_t cur = m_particles.size();
                m_particles.resize( 1024 );

                dataStream.read( m_particles[cur], (int)prtHeader.get_channel_map().structure_size() );
                while( !dataStream.eof() ) {
                    // m_internalBounds += frantic::max3d::to_max_t( posAccessor.get( m_particles[cur] ) );

                    if( ++cur == m_particles.size() )
                        m_particles.resize( 2 * cur );
                    dataStream.read( m_particles[cur], (int)prtHeader.get_channel_map().structure_size() );
                }

                m_particles.resize( cur );

                if( dataStream.gcount() != 0 )
                    throw std::runtime_error( "krakatoa_prt_source_particle_istream Error: In node " + m_name +
                                              ": Unexpected EOF before expected number of particles was read" );
            }
        } else {
            throw std::runtime_error( "krakatoa_prt_source_particle_istream Error: In node " + m_name +
                                      ": Could not get IMaxKrakatoaPRTSource" );
        }

        set_channel_map( m_particles.get_channel_map() );
    }

    virtual ~krakatoa_prt_source_particle_istream() {}

    static IMaxKrakatoaPRTSource* GetIMaxKrakatoaPRTSource( INode* node, TimeValue t ) {
        if( !node ) {
            return 0;
        }
        ObjectState os = node->EvalWorldState( t );
        if( !os.obj ) {
            return 0;
        }
        IMaxKrakatoaPRTSource* pSource =
            static_cast<IMaxKrakatoaPRTSource*>( os.obj->GetInterface( IMaxKrakatoaPRTSource_ID ) );
        return pSource;
    }

    static bool can_get_from_node( INode* node, TimeValue t ) { return GetIMaxKrakatoaPRTSource( node, t ) != NULL; }

    void close() {}

    // The stream can return its filename or other identifier for better error messages.
    std::string name() const { return m_name; };

    // TODO: We should add a verbose_name function, which all wrapping streams are required to mark up in some way

    // This is the size of the particle structure which will be loaded, in bytes.
    virtual std::size_t particle_size() const { return m_outMap.structure_size(); }

    // Returns the number of particles, or -1 if unknown
    virtual boost::int64_t particle_count() const { return m_particles.size(); };
    virtual boost::int64_t particle_index() const { return m_index; }
    virtual boost::int64_t particle_count_left() const { return ( particle_count() - m_index - 1 ); }

    virtual boost::int64_t particle_progress_count() const { return particle_count(); }
    virtual boost::int64_t particle_progress_index() const { return m_index; }

    // If a stream does not know how many particles it has, it can optionally override this function
    // to produce a guess of how many there will be. This guess will be used to pre-allocate storage
    // for this many particles if the user is concerned about memory performance.
    virtual boost::int64_t particle_count_guess() const { return particle_count(); }

    // This allows you to change the particle layout that's being loaded on the fly, in case it couldn't
    // be set correctly at creation time.
    virtual void set_channel_map( const frantic::channels::channel_map& pcm ) {
        std::vector<char> newDefaultParticle( pcm.structure_size() );

        if( newDefaultParticle.size() > 0 ) {
            pcm.construct_structure( &newDefaultParticle[0] );

            // If we previously had a default particle set, adapt its channels to the new layout.
            if( m_defaultParticle.size() > 0 ) {
                frantic::channels::channel_map_adaptor defaultAdaptor( pcm, m_outMap );
                defaultAdaptor.copy_structure( &newDefaultParticle[0], &m_defaultParticle[0] );
            }
        }

        m_defaultParticle.swap( newDefaultParticle );

        // Set the map and the adaptor
        m_outMap = pcm;
        m_pcmAdaptor.set( m_outMap, m_particles.get_channel_map() );
    }

    // This is the particle channel map which specifies the byte layout of the particle structure that is being used.
    virtual const frantic::channels::channel_map& get_channel_map() const { return m_outMap; }

    // This is the particle channel map which specifies the byte layout of the input to this stream.
    // NOTE: This value is allowed to change after the following conditions:
    //    * set_channel_map() is called (for example, the empty_particle_istream equates the native map with the
    //    external map)
    virtual const frantic::channels::channel_map& get_native_channel_map() const {
        return m_particles.get_channel_map();
    }

    /** This provides a default particle which should be used to fill in channels of the requested channel map
     *	which are not supplied by the native channel map.
     *	IMPORTANT: Make sure the buffer you pass in is at least as big as particle_size() bytes.
     */
    virtual void set_default_particle( char* rawParticleBuffer ) {
        memcpy( &m_defaultParticle[0], rawParticleBuffer, m_outMap.structure_size() );
    }

    // This reads a particle into a buffer matching the channel_map.
    // It returns true if a particle was read, false otherwise.
    // IMPORTANT: Make sure the buffer you pass in is at least as big as particle_size() bytes.
    virtual bool get_particle( char* rawParticleBuffer ) {
        if( m_index + 1 < particle_count() ) {
            if( m_pcmAdaptor.is_identity() ) {
                memcpy( rawParticleBuffer, m_particles.at( m_index + 1 ),
                        m_particles.get_channel_map().structure_size() );
            } else {
                m_pcmAdaptor.copy_structure( rawParticleBuffer, m_particles.at( m_index + 1 ), &m_defaultParticle[0] );
            }
            ++m_index;
            return true;
        } else {
            return false;
        }
    }

    // This reads a group of particles. Returns false if the end of the source
    // was reached during the read.
    virtual bool get_particles( char* rawParticleBuffer, std::size_t& numParticles ) {
        for( std::size_t i = 0; i < numParticles; ++i ) {
            if( !this->get_particle( rawParticleBuffer ) ) {
                numParticles = i;
                return false;
            }
            rawParticleBuffer += m_outMap.structure_size();
        }

        return true;
    }
};
