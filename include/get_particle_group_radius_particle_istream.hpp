// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <frantic/channels/channel_map_adaptor.hpp>
#include <frantic/graphics/boundbox3f.hpp>
#include <frantic/graphics/vector3f.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <frantic/max3d/convert.hpp>

class get_particle_group_radius_particle_istream : public frantic::particles::streams::delegated_particle_istream {
    IParticleGroup* m_particles;
    frantic::channels::channel_cvt_accessor<float> m_radiusAccessor;

    frantic::tstring m_radiusChannelName;

    frantic::channels::channel_map m_nativeMap; // May have to add to the native map;

    // When an argument references a channel not specified in the user-supplied channel
    // map, it will request a modified channel map from the delegate with the extra channels.
    frantic::channels::channel_map m_outMap;
    frantic::channels::channel_map_adaptor m_adaptor;
    boost::scoped_array<char> m_tempBuffer;

    // indexed by valueIndex
    // these are the standard shapes like box, 20-sided sphere, etc.
    std::vector<frantic::graphics::vector3f> m_bboxWidths;

    // indexed by Mesh*, if valueIndex is out of range
    // these are from the shape instance operator
    typedef std::map<const Mesh*, frantic::graphics::vector3f> pmesh_to_width_map_t;
    pmesh_to_width_map_t m_pmeshWidthCache;

    struct {
        IParticleChannelPoint3R* scale;
        IParticleChannelMeshR* mesh;
    } m_channels;

    static frantic::graphics::vector3f calculate_mesh_width( const Mesh* mesh ) {
        // tempted to const_cast and getBoundingBox...
        frantic::graphics::vector3f width( 0 );
        if( mesh ) {
            frantic::graphics::boundbox3f bbox;
            // TODO: handle hidden verts?
            if( mesh->verts ) {
                const int numVerts = mesh->getNumVerts();
                for( int i = 0; i < numVerts; ++i ) {
                    bbox += frantic::max3d::from_max_t( mesh->verts[i] );
                }
            }
            if( bbox.is_empty() ) {
                width = frantic::graphics::vector3f( 0 );
            } else {
                width = frantic::graphics::vector3f( bbox.xsize(), bbox.ysize(), bbox.zsize() );
            }
        } else {
            // TODO: what?
            // 1.f appears to match what getParticleScale returns when the mesh is NULL
            // but the mesh is NULL, no geometry will appear..
            width = frantic::graphics::vector3f( 0.f );
        }
        return width;
    }

    frantic::graphics::vector3f get_mesh_width( const Mesh* mesh ) {
        frantic::graphics::vector3f width( 0 );

        pmesh_to_width_map_t::iterator i = m_pmeshWidthCache.find( mesh );
        if( i == m_pmeshWidthCache.end() ) {
            width = calculate_mesh_width( mesh );
            m_pmeshWidthCache[mesh] = width;
        } else {
            width = i->second;
        }

        return width;
    }

    void build_bbox_widths( IParticleChannelMeshR* meshChannel ) {
        m_bboxWidths.clear();
        if( meshChannel ) {
            const int valueCount = meshChannel->GetValueCount();
            m_bboxWidths.resize( std::max( 0, valueCount ) );
            for( int valueIndex = 0; valueIndex < valueCount; ++valueIndex ) {
                const Mesh* m = meshChannel->GetValueByIndex( valueIndex );
                m_bboxWidths[valueIndex] = calculate_mesh_width( m );
            }
        }
    }

    void populate_particle_radius( char* p ) {
        if( !m_radiusAccessor.is_valid() ) {
            return;
        }

        const int index = static_cast<int>( particle_index() );

        frantic::graphics::vector3f scale( 1.f );
        if( m_channels.scale ) {
            scale = frantic::max3d::from_max_t( m_channels.scale->GetValue( index ) );
        }

        frantic::graphics::vector3f bboxWidth( 1.f );
        if( m_channels.mesh ) {
            const int valueIndex = m_channels.mesh->GetValueIndex( index );
            if( valueIndex >= 0 && valueIndex < static_cast<int>( m_bboxWidths.size() ) ) {
                bboxWidth = m_bboxWidths[valueIndex];
            } else {
                bboxWidth = get_mesh_width( m_channels.mesh->GetValue( index ) );
            }
        }

        const frantic::graphics::vector3f scaledWidth =
            frantic::graphics::vector3f::component_multiply( scale, bboxWidth );
        const float radius = 0.5f * scaledWidth.max_abs_component();

        m_radiusAccessor.set( p, radius );
    }

  public:
    get_particle_group_radius_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin,
                                                IParticleGroup* particles, const frantic::tstring& radiusChannelName )
        : delegated_particle_istream( pin )
        , m_particles( particles )
        , m_radiusChannelName( radiusChannelName ) {
        if( !particles ) {
            throw std::runtime_error( "get_particle_group_radius_particle_istream Error: particles is NULL" );
        }

        IPFSystem* particleSystem = PFSystemInterface( m_particles->GetParticleSystem() );
        if( !particleSystem ) {
            throw std::runtime_error(
                "get_particle_group_radius_particle_istream() - Could not get the IPFSystem from the IParticleGroup" );
        }

        IParticleObjectExt* particleSystemParticles = GetParticleObjectExtInterface( particleSystem );
        if( !particleSystemParticles ) {
            throw std::runtime_error( "get_particle_group_radius_particle_istream() - Could not get the "
                                      "IParticleObjectExt from the IPFSystem" );
        }

        set_channel_map( m_delegate->get_channel_map() );

        m_nativeMap = m_delegate->get_native_channel_map();
        if( !m_nativeMap.has_channel( radiusChannelName ) ) {
            m_nativeMap.append_channel<float>( radiusChannelName );
        }

        IObject* particleContainer = m_particles->GetParticleContainer();
        if( !particleContainer ) {
            // Apparently PFlow has started making bunk particle event objects that don't have a particle container.
            // This allows them to silently slip away instead of stopping the render. FF_LOG(warning) <<
            // "get_particle_group_radius_particle_istream() - Could not GetParticleContainer() from the IParticleGroup"
            // << std::endl;
            return;
        }

        IParticleChannelAmountR* amountChannel = GetParticleChannelAmountRInterface( particleContainer );
        if( !amountChannel ) {
            throw std::runtime_error(
                "get_particle_group_radius_particle_istream() - Could not get the pflow IParticleChannelAmountR" );
        }

        IChannelContainer* channelContainer = GetChannelContainerInterface( particleContainer );
        if( !channelContainer ) {
            throw std::runtime_error( "get_particle_group_radius_particle_istream() - Could not get the pflow "
                                      "IParticleContainer interface from the supplied node" );
        }

        m_channels.scale = GetParticleChannelScaleRInterface( channelContainer );
        m_channels.mesh = GetParticleChannelShapeRInterface( channelContainer );

        build_bbox_widths( m_channels.mesh );
    }

    virtual ~get_particle_group_radius_particle_istream() {}

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        frantic::channels::channel_map requested = m_outMap = pcm;

        m_radiusAccessor.reset();
        if( requested.has_channel( m_radiusChannelName ) ) {
            m_radiusAccessor = requested.get_cvt_accessor<float>( m_radiusChannelName );
        }

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
