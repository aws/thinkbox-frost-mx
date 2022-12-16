// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <string>
#include <vector>

#pragma warning( push, 3 )
#pragma warning( disable : 4100 ) // unreferenced formal parameter
#include <gizmo.h>
#pragma warning( pop )

#pragma warning( push, 3 )
#pragma warning( disable : 4239 ) // nonstandard extension used : 'default argument' : conversion from 'DefaultRemapDir'
                                  // to 'RemapDir &'
#include <gizmoimp.h>
#pragma warning( pop )

#include <frantic/graphics/boundbox3f.hpp>
#include <frantic/max3d/max_utility.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

class gizmo_particle_istream : public frantic::particles::streams::particle_istream {
    GizmoObject* m_gizmoObject;

    TimeValue m_time;
    TimeValue m_dt;

    int m_index;
    frantic::tstring m_name;

    frantic::channels::channel_map m_nativeMap;
    frantic::channels::channel_map m_outMap;

    std::vector<char> m_defaultParticleBuffer;

    struct {
        bool hasPosition;
        bool hasVelocity;
        bool hasRadius;

        frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> position;
        frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> velocity;
        frantic::channels::channel_cvt_accessor<float> radius;
    } m_accessors;

    void init_accessors( const frantic::channels::channel_map& pcm ) {
        if( m_accessors.hasPosition = pcm.has_channel( _T("Position") ) )
            m_accessors.position = pcm.get_cvt_accessor<frantic::graphics::vector3f>( _T("Position") );
        if( m_accessors.hasVelocity = pcm.has_channel( _T("Velocity") ) )
            m_accessors.velocity = pcm.get_cvt_accessor<frantic::graphics::vector3f>( _T("Velocity") );
        if( m_accessors.hasRadius = pcm.has_channel( _T("Radius") ) )
            m_accessors.radius = pcm.get_cvt_accessor<float>( _T("Radius") );
    }

  public:
    gizmo_particle_istream( INode* pNode, TimeValue t, TimeValue dt )
        : m_gizmoObject( 0 )
        , m_time( t )
        , m_dt( dt )
        , m_index( -1 ) {
        if( pNode == 0 ) {
            throw std::runtime_error( "gizmo_particle_istream Error: INode is NULL" );
        }
        m_gizmoObject = 0;

        Object* prtObject = pNode->GetObjectRef();
        if( prtObject == 0 ) {
            throw std::runtime_error( "gizmo_particle_istream Error: Object is NULL" );
        }
        Object* baseObj = frantic::max3d::get_base_object( pNode );
        if( baseObj == 0 ) {
            throw std::runtime_error( "gizmo_particle_istream Error: Node\'s object is NULL" );
        }
        ObjectState os = pNode->EvalWorldState( t );

        if( baseObj->CanConvertToType( SPHEREGIZMO_CLASSID ) ) {
            m_gizmoObject = reinterpret_cast<GizmoObject*>( baseObj->ConvertToType( t, SPHEREGIZMO_CLASSID ) );
        } else if( baseObj->CanConvertToType( CYLGIZMO_CLASSID ) ) {
            m_gizmoObject = reinterpret_cast<GizmoObject*>( baseObj->ConvertToType( t, CYLGIZMO_CLASSID ) );
        } else if( baseObj->CanConvertToType( BOXGIZMO_CLASSID ) ) {
            m_gizmoObject = reinterpret_cast<GizmoObject*>( baseObj->ConvertToType( t, BOXGIZMO_CLASSID ) );
        } else {
            throw std::runtime_error( "gizmo_particle_istream Error: unable to get gizmo from node" );
        }

        m_name = pNode->GetName();

        m_nativeMap.define_channel<frantic::graphics::vector3f>( _T("Position") );
        m_nativeMap.define_channel<frantic::graphics::vector3f>( _T("Velocity") );
        m_nativeMap.define_channel<float>( _T("Radius") );
        m_nativeMap.end_channel_definition();

        set_channel_map( m_nativeMap );
    }

    // Virtual destructor so that we can use allocated pointers (generally with boost::shared_ptr)
    virtual ~gizmo_particle_istream() {}

    void close() {}

    // The stream can return its filename or other identifier for better error messages.
    frantic::tstring name() const { return m_name; };

    // TODO: We should add a verbose_name function, which all wrapping streams are required to mark up in some way

    // This is the size of the particle structure which will be loaded, in bytes.
    virtual std::size_t particle_size() const { return m_outMap.structure_size(); }

    // Returns the number of particles, or -1 if unknown
    virtual boost::int64_t particle_count() const { return 1; };
    virtual boost::int64_t particle_index() const { return m_index; }
    virtual boost::int64_t particle_count_left() const { return ( m_index == -1 ? 1 : 0 ); }

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
        if( m_defaultParticleBuffer.size() > 0 ) {
            frantic::channels::channel_map_adaptor defaultAdaptor( pcm, m_outMap );
            defaultAdaptor.copy_structure( &newDefaultParticle[0], &m_defaultParticleBuffer[0] );
        } else if( newDefaultParticle.size() ) {
            memset( &newDefaultParticle[0], 0, pcm.structure_size() );
        }
        m_defaultParticleBuffer.swap( newDefaultParticle );
        m_outMap = pcm;
        init_accessors( pcm );
    }

    // This is the particle channel map which specifies the byte layout of the particle structure that is being used.
    virtual const frantic::channels::channel_map& get_channel_map() const { return m_outMap; }

    // This is the particle channel map which specifies the byte layout of the input to this stream.
    // NOTE: This value is allowed to change after the following conditions:
    //    * set_channel_map() is called (for example, the empty_particle_istream equates the native map with the
    //    external map)
    virtual const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }

    /** This provides a default particle which should be used to fill in channels of the requested channel map
     *	which are not supplied by the native channel map.
     *	IMPORTANT: Make sure the buffer you pass in is at least as big as particle_size() bytes.
     */
    virtual void set_default_particle( char* rawParticleBuffer ) {
        memcpy( &m_defaultParticleBuffer[0], rawParticleBuffer, m_outMap.structure_size() );
    }

    // This reads a particle into a buffer matching the channel_map.
    // It returns true if a particle was read, false otherwise.
    // IMPORTANT: Make sure the buffer you pass in is at least as big as particle_size() bytes.
    virtual bool get_particle( char* rawParticleBuffer ) {
        if( m_index == -1 ) {
            m_index = 0;
            if( m_defaultParticleBuffer.size() ) {
                memcpy( rawParticleBuffer, &m_defaultParticleBuffer[0], m_outMap.structure_size() );
            }
            Matrix3 m;
            m.IdentityMatrix();
            Box3 box;
            m_gizmoObject->GetBoundBox( m, m_time, box );
            const frantic::graphics::boundbox3f bounds( frantic::max3d::from_max_t( box ) );
            const frantic::graphics::vector3f position = bounds.center();
            if( m_accessors.hasPosition ) {
                m_accessors.position.set( rawParticleBuffer, position );
            }
            if( m_accessors.hasVelocity ) {
                // TODO : this is untested; velocity comes from node transform
                Box3 box1;
                m_gizmoObject->GetBoundBox( m, m_time + m_dt, box1 );
                const frantic::graphics::boundbox3f bounds1( frantic::max3d::from_max_t( box1 ) );
                const frantic::graphics::vector3f position1 = bounds1.center();
                const frantic::graphics::vector3f velocity =
                    ( position1 - position ) * static_cast<float>( TIME_TICKSPERSEC / m_dt );
                m_accessors.velocity.set( rawParticleBuffer, velocity );
            }
            if( m_accessors.hasRadius ) {
                const float radius = 0.5f * bounds.get_max_dimension();
                m_accessors.radius.set( rawParticleBuffer, radius );
            }
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
