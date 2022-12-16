// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#if VRAY_DLL_VERSION >= 0x60000
#if !defined(MAX_RELEASE)
#define MAX_RELEASE
#include <shadedata_new.h>
#undef MAX_RELEASE
#else
#include <shadedata_new.h>
#endif
#endif

#include "FrostRenderInstance.hpp"
#include "frost_interface_geometry_source.hpp"

#include "VRayUtils.hpp"

#include <frost/channel_names.hpp>
#include <frost/geometry_meshing.hpp>
#include <frost/geometry_meshing_parameters_interface.hpp>
#include <frost/utility.hpp>

#include "FrostInterface.hpp"
#include "Trimesh3MeshInterface.hpp"

#include <frantic/max3d/max_utility.hpp>
#include <frantic/max3d/time.hpp>

#include <frantic/channels/channel_propagation_policy.hpp>
#include <frantic/graphics/camera.hpp>

#include <mesh_file.h>
#if VRAY_DLL_VERSION < 0x42000
#include <tomax.h>
#else
#include <maxutils/tomax.h>
#endif


#include <Scene/IPhysicalCamera.h>

#include <ImathVec.h>

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>

#if defined( tstring )
#undef tstring
#endif
#if defined( min )
#undef min
#endif
#if defined( max )
#undef max
#endif

#define VRAY_PROXY_CLASS_ID Class_ID( 1828010099, 536368280 )

using namespace VR;

namespace {

inline frantic::graphics::vector3f normalized( const frantic::graphics::vector3f& v ) {
    Imath::Vec3<float> imfVec( v.x, v.y, v.z );
    imfVec.normalize();
    return frantic::graphics::vector3f( imfVec.x, imfVec.y, imfVec.z );
}

inline VUtils::Transform to_vray( const frantic::graphics::transform4f& xform ) {
    VUtils::Transform result;

    for( int j = 0; j < 3; ++j ) {
        for( int i = 0; i < 3; ++i ) {
            result.m[j][i] = xform.get_column( j )[i];
        }
    }

    for( int i = 0; i < 3; ++i ) {
        result.offs[i] = xform.get_column( 3 )[i];
    }

    return result;
}

bool is_vray_proxy( INode* node ) {
    if( !node ) {
        return false;
    }

    Object* obj = frantic::max3d::get_base_object( node );
    if( !obj ) {
        return false;
    }

    return obj->ClassID() == VRAY_PROXY_CLASS_ID;
}

material_id_mode_info create_material_id_mode_info( FrostInterface* frost ) {
    if( !frost ) {
        throw std::runtime_error( "create_material_id_mode_info Error: frost is NULL" );
    }

    const std::size_t shapeCount = frost->get_custom_geometry_count();

    typedef std::map<INode*, std::size_t> node_to_shape_number_t;
    node_to_shape_number_t nodeToShapeNumber;

    for( std::size_t i = 0, ie = shapeCount; i < ie; ++i ) {
        nodeToShapeNumber[frost->get_custom_geometry( i )] = i;
    }

    const int materialMode = frost->get_material_mode();

    material_id_mode_info materialModeInfo( materialMode, frost->get_undefined_material_id() );

    std::vector<geometry_material_id_map_t>& geometryMaterialIdMaps = materialModeInfo.geometryMaterialIdMaps;
    geometryMaterialIdMaps.resize( shapeCount );
    std::size_t materialIDMapEntryCount =
        std::min( frost->get_geometry_material_id_node_count(),
                  std::min( frost->get_geometry_material_id_in_count(), frost->get_geometry_material_id_out_count() ) );

    for( std::size_t i = 0; i < materialIDMapEntryCount; ++i ) {
        node_to_shape_number_t::const_iterator iter =
            nodeToShapeNumber.find( frost->get_geometry_material_id_node( i ) );
        if( iter == nodeToShapeNumber.end() ) {
            continue;
        }
        std::size_t shapeNumber = iter->second;
        if( shapeNumber >= shapeCount ) {
            continue;
        }

        const int maxId = std::numeric_limits<MtlID>::max();
        int inId = frost->get_geometry_material_id_in( i );
        if( inId < 0 || inId > maxId ) {
            continue;
        }

        int outId = frost->get_geometry_material_id_out( i );
        if( outId < 0 || outId > maxId ) {
            continue;
        }

        geometryMaterialIdMaps[shapeNumber][static_cast<MtlID>( inId )] = static_cast<MtlID>( outId );
    }

    std::vector<int>& meshEndMaterialId = materialModeInfo.meshEndMaterialId;
    meshEndMaterialId.clear();
    meshEndMaterialId.resize( shapeCount, 1 );
    if( materialMode == frost::material_mode::material_from_geometry ) {
        for( std::size_t shapeNumber = 0; shapeNumber < shapeCount; ++shapeNumber ) {
            boost::uint16_t maxMaterialId = 0;
            if( shapeNumber < geometryMaterialIdMaps.size() ) {
                geometry_material_id_map_t& materialIdMap = geometryMaterialIdMaps[shapeNumber];
                for( geometry_material_id_map_t::iterator i = materialIdMap.begin(); i != materialIdMap.end(); ++i ) {
                    if( i->first > maxMaterialId ) {
                        maxMaterialId = i->first;
                    }
                }
            }
            meshEndMaterialId[shapeNumber] = 1 + static_cast<int>( maxMaterialId );
        }
    }

    return materialModeInfo;
}

} // anonymous namespace

FrostRenderInstance::FrostRenderInstance( FrostInterface* frost, VUtils::VRenderObject* renderObject, INode* node,
                                          VRayCore* vray, int renderID )
    : m_frost( frost )
    , m_firstPrimitiveId( -1 )
    , m_useMovingPrimitives( false )
    , m_secondsPerMotionBlurInterval( 1 )
    , m_secondsPerFrame( 1 ) {

    VRenderInstance::init( renderObject, node, vray, renderID );
}

FrostRenderInstance::~FrostRenderInstance( void ) {}

void FrostRenderInstance::generateInstances( TimeValue t, VRayCore* vray, VRayRenderGlobalContext2* globContext2 ) {
    enumParticles( t, vray, globContext2 );
}

void FrostRenderInstance::deleteInstances( VRayCore* vray ) {}

void FrostRenderInstance::renderBegin( TimeValue t, VRayCore* vray ) { VRenderInstance::renderBegin( t, vray ); }

void FrostRenderInstance::renderEnd( VRayCore* vray ) {
    VRenderInstance::renderEnd( vray );

    try {
        m_useMovingPrimitives = false;
        m_staticPrimitives.reset();
        m_movingPrimitives.reset();
        m_shapeIndices.clear();
        m_shapes.clear();
        m_particles.clear();
        m_mapChannelAccessors.clear();
        m_materialIdAccessor.reset();
        m_secondsPerMotionBlurInterval = 1;
        m_secondsPerFrame = 1;
        m_velocityAccessor.reset();
    } catch( std::exception& e ) {
        report_error( vray, "FrostRenderInstance::renderEnd", e.what() );
    }
}

/**
 * Returns the ClassID of the current render camera.
 * Returns inconsistent results when rendering from maxscript ie
 * if one has a maxscript that does `render camera:$PhysCamera01`
 * but selects the front view in 3ds Max, then the render view will state front
 * view rather than Physical Camera. The workaround is to select what camera is being used or
 * use `render` or not use maxscript.
 */
Class_ID FrostRenderInstance::getRenderCameraClassID() const {
    // GetCoreInterface16 is available from 3dmax2016+
    ViewExp* const renderView = GetCOREInterface16()->GetCurrentRenderView();
    if( renderView != nullptr ) {
        INode* const renderViewSpot = renderView->GetViewSpot();
        INode* const renderViewCamera = renderView->GetViewCamera();

        // If we are not rendering a view spot or camera then there is no camera to get a classid from
        // Based on 3ds Max Documentation there cannot both be a ViewCamera and ViewSpot but its possible
        // for both to be null if rendering using maxscript.
        if( renderViewCamera != nullptr ) {
            return renderViewCamera->GetObjectRef()->FindBaseObject()->ClassID();
        } else if( renderViewSpot != nullptr ) {
            return renderViewSpot->GetObjectRef()->FindBaseObject()->ClassID();
        }
    }
    return Class_ID();
}

void FrostRenderInstance::enumParticles( TimeValue t, VR::VRayCore* vray, VR::VRayRenderGlobalContext2* globContext2 ) {
    using namespace frantic::graphics;
    using namespace frantic::geometry;
    using namespace frantic::channels;
    using namespace frost;
    using frantic::tstring;
    using std::vector;

    int moblurOn = vray->getSequenceData().params.moblur.on;
    float mbDuration = vray ? vray->getSequenceData().params.moblur.duration : 1.0f;
    float mbCenter = vray->getSequenceData().params.moblur.intervalCenter;

    Class_ID renderCameraClassID = getRenderCameraClassID();

    // The physical camera overrides vray motion blur settings and requires manual conversion.
    // If rendering a physical camera, transform the mbCenter from ticks into frames.
    if( renderCameraClassID == MaxSDK::IPhysicalCamera::GetClassID() ) {
        mbCenter /= GetTicksPerFrame();
    }

    m_shapeIndices.clear();

    if( !m_frost->HasAnyParticleSource( vray->getFrameData().t ) ) {
        return;
    }
    boost::shared_ptr<frost::geometry_meshing_parameters_interface> params =
        m_frost->create_geometry_meshing_parameters( vray->getFrameData().t );
    boost::shared_ptr<geometry_source_interface> sceneGeometrySource =
        boost::make_shared<frost_interface_geometry_source>( m_frost );
    frantic::graphics::camera<float> camera;
    frantic::particles::streams::particle_istream_ptr pin =
        m_frost->GetParticleIStream( vray->getFrameData().t, camera );

    std::vector<VUtils::Transform> transforms;

    boost::optional<boundbox3f> particleCullingBox = m_frost->get_roi_box( vray->getFrameData().t );

    material_id_mode_info materialModeInfo = create_material_id_mode_info( m_frost );

    m_materialMode = m_frost->get_material_mode();

    frantic::channels::channel_propagation_policy cpp;

    m_useMovingPrimitives = false;
    m_mapChannelAccessors.clear();
    m_mapChannelAccessors.resize( MAX_MAP_CHANS );
    m_materialIdAccessor.reset();
    m_secondsPerMotionBlurInterval = mbDuration * frantic::max3d::to_seconds<float>( GetTicksPerFrame() );
    m_secondsPerFrame = frantic::max3d::to_seconds<float>( GetTicksPerFrame() );
    m_velocityAccessor.reset();

    channel_propagation_policy particleToMeshCpp( true );
    channel_propagation_policy meshCpp( false );
    {
        channel_map channelMap = pin->get_channel_map();
        int materialMode = m_frost->get_material_mode();
        channel_map dataChannelMap;

        for( size_t i = 0, ie = channelMap.channel_count(); i < ie; ++i ) {
            const tstring& name = channelMap[i].name();

            int channelNumber = 0;
            if( is_map_channel( name, &channelNumber ) ) {
                if( channelNumber == 1 && params->get_geometry_type() != frost::geometry_type::custom_geometry ) {
                    continue;
                }
                dataChannelMap.define_channel<vector3f>( name );
                meshCpp.add_channel( name );
            } else if( materialMode == frost::material_mode::mtlindex_channel && name == _T("MtlIndex") ) {
                dataChannelMap.define_channel<boost::int32_t>( name );
                meshCpp.add_channel( _T("MaterialID") );
            } else if( name == _T("Velocity") ) {
                dataChannelMap.define_channel<vector3f>( name );
            }
        }

        dataChannelMap.end_channel_definition();

        for( size_t i = 0, ie = dataChannelMap.channel_count(); i < ie; ++i ) {
            const tstring& name = dataChannelMap[i].name();

            int channelNumber = 0;
            if( is_map_channel( name, &channelNumber ) ) {
                m_mapChannelAccessors[channelNumber] = dataChannelMap.get_accessor<vector3f>( name );
            } else if( name == _T("MtlIndex") ) {
                m_materialIdAccessor = dataChannelMap.get_accessor<boost::int32_t>( name );
            } else if( name == _T("Velocity") ) {
                m_velocityAccessor = dataChannelMap.get_accessor<vector3f>( name );
            }
        }

        m_particles.reset( dataChannelMap );
    }

    m_useMovingPrimitives = moblurOn && m_velocityAccessor.is_valid();

    if( !particleCullingBox ) {
        // Only reserve if we don't have a particleCullingBox, because culling
        // may remove most of the particles.
        boost::int64_t particleCountSigned = pin->particle_count();
        if( particleCountSigned > 0 ) {
            std::size_t particleCount = static_cast<std::size_t>( particleCountSigned );

            transforms.reserve( particleCount );
            m_shapeIndices.reserve( particleCount );
            m_particles.reserve( particleCount );
        }
    }

    frantic::channels::channel_map_adaptor cma( m_particles.get_channel_map(), pin->get_channel_map() );

    std::vector<char> particle( pin->particle_size() );

    frantic::channels::channel_map emptyChannelMap;
    emptyChannelMap.end_channel_definition();

    const geometry_type::option instanceGeometryType = params->get_geometry_type();

    boost::shared_ptr<instance_geometry_factory> instanceGeometryFactory =
        get_instance_geometry_factory( instanceGeometryType, emptyChannelMap, emptyChannelMap, materialModeInfo,
                                       sceneGeometrySource, frost::hard_edge_type::vertex_normal );

    if( !instanceGeometryFactory ) {
        throw std::runtime_error( "Internal Error: instance geometry factory is NULL" );
    }

    const std::size_t shapeCount = instanceGeometryFactory->get_shape_count();

    if( shapeCount > 0 ) {

        if( instanceGeometryType != frost::geometry_type::custom_geometry ) {
            m_shapes.resize( 1 );
            m_shapes[0].init( instanceGeometryFactory->get_instance_geometry( 0, 0 )->mesh, meshCpp,
                              vray->getRayServer() );
#if VRAY_DLL_VERSION < 0x40000
            // buildTree causes an AccessException on subsequent renders without changing
            // any settings in V-Ray 4.
            // We're disabling this call in V-Ray 4 for now, since we're not sure
            // it's actually necessary in the first place.
            VUtils::MeshVoxel* meshVoxel = m_shapes[0].meshInterface->getVoxel( 0 );
            meshVoxel->buildTree( vray->getSequenceData().threadManager );
#endif
        }

        typedef std::pair<int, TimeValue> node_and_time_t;
        typedef boost::unordered_map<node_and_time_t, int, boost::hash<node_and_time_t>> node_and_time_to_shape_index_t;
        node_and_time_to_shape_index_t nodeAndTimeToShapeIndex;

        // Generate the mesh
        const bool isOrientationRestricted = params->get_orientation_restrict_divergence_axis();
        const boost::uint32_t randomSelectionSeed = params->get_geometry_selection_seed();
        const frantic::tstring orientationVector = params->get_geometry_orientation_vector_channel();
        const vector3f specifiedOrientation( params->get_geometry_orientation() );
        const float divergence = params->get_geometry_orientation_divergence(); // maximum rotation
        // Must generate random in 0 to 360 (max) then shift it to -180 to 180
        const float divergenceX2 = divergence * 2;
        const float divergenceRad =
            boost::math::constants::pi<float>() * frantic::math::clamp<float>( fabs( divergence ), 0, 180 ) / 180;

        const vector3f divergenceAxis( params->get_orientation_divergence_axis() );

        frantic::channels::channel_map pinChannelMap = pin->get_channel_map();
        frantic::channels::channel_const_cvt_accessor<boost::int32_t> idAcc( 0 );
        frantic::channels::channel_const_cvt_accessor<vector3f> vecAcc( vector3f( 0, 0, 1 ) );
        frantic::channels::channel_const_cvt_accessor<vector4f> orientationAcc( vector4f( 0, 0, 0, 1 ) );
        frantic::channels::channel_const_cvt_accessor<vector3f> positionAcc( vector3f( 0 ) );
        frantic::channels::channel_const_cvt_accessor<float> radiusAcc( 0 );
        frantic::channels::channel_const_cvt_accessor<boost::int32_t> shapeIndexAcc( 0 );

        if( pinChannelMap.has_channel( Frost_IDCHANNEL ) ) {
            idAcc = pinChannelMap.get_const_cvt_accessor<boost::int32_t>( Frost_IDCHANNEL );
        }
        if( pinChannelMap.has_channel( Frost_POSITIONCHANNEL ) ) {
            positionAcc = pinChannelMap.get_const_cvt_accessor<vector3f>( Frost_POSITIONCHANNEL );
        }
        if( pinChannelMap.has_channel( _T("Radius") ) ) {
            radiusAcc = pinChannelMap.get_const_cvt_accessor<float>( _T("Radius") );
        }

        boost::scoped_ptr<get_shape_number_strategy> randomSelectionStrategy;
        if( instanceGeometryFactory->get_shape_count() > 1 ) {
            const geometry_selection_mode::option randomSelectionModeVal = params->get_geometry_selection_mode();
            switch( randomSelectionModeVal ) {
            case geometry_selection_mode::cycle:
                randomSelectionStrategy.reset( new cycle_shapes_strategy( static_cast<boost::int32_t>( shapeCount ) ) );
                break;
            case geometry_selection_mode::random_by_id:
                randomSelectionStrategy.reset( new get_random_shape_by_id_strategy(
                    idAcc, static_cast<boost::int32_t>( shapeCount ), randomSelectionSeed ) );
                break;
            case geometry_selection_mode::shapeindex_channel:
                if( !pinChannelMap.has_channel( Frost_SHAPEINDEXCHANNEL ) ) {
                    throw std::runtime_error( "The particles don\'t have a \"ShapeIndex\" "
                                              "channel, which is required when selecting "
                                              "instance geometry by ShapeIndex." );
                }
                shapeIndexAcc = pinChannelMap.get_const_cvt_accessor<boost::int32_t>( Frost_SHAPEINDEXCHANNEL );
                randomSelectionStrategy.reset(
                    new use_shapeindex_channel_strategy( shapeIndexAcc, static_cast<boost::int32_t>( shapeCount ) ) );
                break;
            default:
                throw std::runtime_error( "Unrecognized Instance Geometry Random Selection Mode: " +
                                          boost::lexical_cast<std::string>( randomSelectionModeVal ) );
            }
        } else {
            randomSelectionStrategy.reset( new use_first_shape_strategy() );
        }
        if( randomSelectionStrategy == 0 ) {
            throw std::runtime_error( "Internal Error: No Instance Geometry Selection Mode." );
        }

        vector3f lookAtPos( 0 );
        vector3f targetObjectOrientation;
        const geometry_orientation_mode::option orientationMethod =
            params->get_geometry_orientation_mode(); // method of orientation
        switch( orientationMethod ) {
        case geometry_orientation_mode::look_at:
            lookAtPos = params->get_geometry_look_at_position();
            break;
        case geometry_orientation_mode::match_object:
            targetObjectOrientation = params->get_geometry_look_at_orientation();
            break;
        case geometry_orientation_mode::specify:
            break;
        case geometry_orientation_mode::orientation_channel:
            if( pinChannelMap.has_channel( _T("Orientation") ) ) {
                orientationAcc = pinChannelMap.get_const_cvt_accessor<vector4f>( _T("Orientation") );
            } else {
                throw std::runtime_error( "The particles don\'t have an \'Orientation\' "
                                          "channel, which was requested for instance "
                                          "geometry orientation." );
            }
            break;
        case geometry_orientation_mode::vector_channel:
            if( pinChannelMap.has_channel( orientationVector ) ) {
                vecAcc = pinChannelMap.get_const_cvt_accessor<vector3f>( orientationVector );
            } else {
                throw std::runtime_error( "The particles don\'t have a \'" +
                                          frantic::strings::to_string( orientationVector ) +
                                          "\' channel, which was requested for instance geometry "
                                          "orientation." );
            }
            break;
        default:
            throw std::runtime_error( "Unrecognized Instance Geometry Orientation Type: " +
                                      boost::lexical_cast<std::string>( orientationMethod ) );
        }

        const geometry_orientation_divergence_axis_space::option divergenceAxisSpace =
            params->get_geometry_orientation_divergence_axis_space();
        switch( divergenceAxisSpace ) {
        case geometry_orientation_divergence_axis_space::world:
            break;
        case geometry_orientation_divergence_axis_space::local:
            break;
        default:
            throw std::runtime_error( "Unrecognized Instance Geometry Divergence Axis Space: " +
                                      boost::lexical_cast<std::string>( divergenceAxisSpace ) );
        }

        bool useAbsoluteTimeChannel = false;
        frantic::channels::channel_const_cvt_accessor<float> absoluteTimeAcc;

        bool useTimeOffsetChannel = false;
        frantic::channels::channel_const_cvt_accessor<float> timeOffsetAcc;

        bool useGeomTimeChannel = false;
        frantic::channels::channel_const_cvt_accessor<float> geomTimeAcc;

        bool randomizeTimeOffsetByID = false;
        double maxRandomizedTimeOffsetInSeconds = 0;
        boost::uint32_t geometrySampleTimeSeed = 0;

        double baseTimeInSeconds = params->get_time();
        const geometry_sample_time_base_mode::option baseTimeMode = params->get_geometry_sample_time_base_mode();
        switch( baseTimeMode ) {
        case geometry_sample_time_base_mode::time_0:
            baseTimeInSeconds = 0;
            break;
        case geometry_sample_time_base_mode::current_time:
            baseTimeInSeconds = params->get_time();
            break;
        default:
            throw std::runtime_error( "unrecognized geometrySampleTimeBaseMode: " +
                                      boost::lexical_cast<std::string>( baseTimeMode ) );
        }

        if( instanceGeometryFactory->is_animated() ) {
            const int instanceGeometryAnimationMode = params->get_geometry_sample_time_offset_mode();
            switch( instanceGeometryAnimationMode ) {
            case geometry_sample_time_offset_mode::no_offset:
                break;
            case geometry_sample_time_offset_mode::random_by_id:
                maxRandomizedTimeOffsetInSeconds = params->get_geometry_sample_time_max_random_offset();
                randomizeTimeOffsetByID = true;
                geometrySampleTimeSeed = params->get_geometry_sample_time_seed();
                break;
            case geometry_sample_time_offset_mode::abstime_channel:
                if( !pinChannelMap.has_channel( _T("AbsTime") ) ) {
                    throw std::runtime_error( "The particles don\'t have an \"AbsTime\" "
                                              "channel, which is required when animating "
                                              "geometry by AbsTime channel." );
                }
                absoluteTimeAcc = pinChannelMap.get_const_cvt_accessor<float>( _T("AbsTime") );
                useAbsoluteTimeChannel = true;
                break;
            case geometry_sample_time_offset_mode::timeoffset_channel:
                if( !pinChannelMap.has_channel( _T("TimeOffset") ) ) {
                    throw std::runtime_error( "The particles don\'t have a \"TimeOffset\" "
                                              "channel, which is required when animating "
                                              "geometry by TimeOffset channel." );
                }
                timeOffsetAcc = pinChannelMap.get_const_cvt_accessor<float>( _T("TimeOffset") );
                useTimeOffsetChannel = true;
                break;
            case geometry_sample_time_offset_mode::geomtime_channel:
                if( !pinChannelMap.has_channel( _T("GeomTime") ) ) {
                    throw std::runtime_error( "The particles don\'t have a \"GeomTime\" "
                                              "channel, which is required when animating "
                                              "geometry by GeomTime channel." );
                }
                geomTimeAcc = pinChannelMap.get_const_cvt_accessor<float>( _T("GeomTime") );
                useGeomTimeChannel = true;
                break;
            default:
                throw std::runtime_error( "Unrecognized Particle Geometry Animation Timing Mode: " +
                                          boost::lexical_cast<std::string>( instanceGeometryAnimationMode ) );
            }
        }

        // Create vector of channels that should be transformed. Velocity should not
        // be transformed.
        std::vector<std::pair<frantic::tstring, frantic::geometry::trimesh3::vector_type>> transformNoChannels;
        std::vector<std::pair<frantic::tstring, frantic::geometry::trimesh3::vector_type>> transformNormalChannel;
        transformNormalChannel.push_back( std::pair<frantic::tstring, frantic::geometry::trimesh3::vector_type>(
            _T("Normal"), frantic::geometry::trimesh3::NORMAL ) );

        while( pin->get_particle( particle ) ) {

            if( particleCullingBox && !particleCullingBox->contains( positionAcc( particle ) ) ) {
                continue;
            }

            const boost::int32_t id = idAcc( particle );

            // Random Shape Selection
            boost::int32_t currentElement =
                randomSelectionStrategy->get_shape_number( pin->particle_index(), particle );

            // Orientation
            // set to identity in case invalid value is found
            transform4f rotationTransform = transform4f::identity();
            switch( orientationMethod ) {
            case geometry_orientation_mode::look_at: {
                const frantic::graphics::vector3f direction( lookAtPos - positionAcc( particle ) );
                if( !direction.is_zero() )
                    rotationTransform = transform4f::from_normal_z( normalized( direction ) );
                break;
            }
            case geometry_orientation_mode::match_object:
                rotationTransform = transform4f::from_euler_xyz_rotation( targetObjectOrientation );
                break;
            case geometry_orientation_mode::orientation_channel: {
                const vector4f q = orientationAcc.get( particle );
                if( !q.is_zero() )
                    rotationTransform = q.quaternion_to_matrix();
                // vector4f::quaternion_to_matrix() currently returns a zero matrix
                // if the quaterion's magnitude is too small.  In this case, we
                // reset to identity.
                if( rotationTransform.is_zero() )
                    rotationTransform.set_to_identity();
                break;
            }
            case geometry_orientation_mode::vector_channel: {
                const vector3f v = vecAcc.get( particle );
                if( v != vector3f( 0 ) ) {
                    vector3f vn = v.to_normalized();
                    for( int axis = 0; axis < 3; ++axis ) {
                        if( fabs( vn[axis] ) <= std::numeric_limits<float>::epsilon() ) {
                            vn[axis] = 0;
                        }
                    }
                    if( vn.get_magnitude_squared() < 0.99f ) {
                        vn = vector3f::from_zaxis();
                    }
                    rotationTransform = transform4f::from_normal_z( vn );
                }
                break;
            }
            case geometry_orientation_mode::specify:
                rotationTransform = transform4f::from_euler_xyz_rotation( specifiedOrientation );
                break;
            }

            transform4f divergenceTransform;
            if( isOrientationRestricted ) {
                // TODO: give option of object space or world space axis
                const float angleRad = id_randomized_rotation( id, 2 * divergenceRad, 177720997 ) - divergenceRad;
                divergenceTransform = transform4f::from_angle_axis( angleRad, divergenceAxis );
            } else {
                // Set the random divergence (-180 to 180 Max)
                float orientationX = id_randomized_rotation( id, divergenceX2, 1423264495 ) -
                                     divergence; // amount to rotate in x direction
                float orientationY = id_randomized_rotation( id, divergenceX2, 778162823 ) -
                                     divergence; // amount to rotate in y direction
                float orientationZ = id_randomized_rotation( id, divergenceX2, 171472941 ) -
                                     divergence; // amount to rotate in z direction
                divergenceTransform = transform4f::from_euler_xyz_rotation_degrees(
                    vector3f( orientationX, orientationY, orientationZ ) );
            }

            const transform4f rotationAndDivergenceTransform =
                ( divergenceAxisSpace == geometry_orientation_divergence_axis_space::local )
                    ? rotationTransform * divergenceTransform
                    : divergenceTransform * rotationTransform;

            double requestTimeInSeconds = baseTimeInSeconds;
            if( useAbsoluteTimeChannel ) {
                requestTimeInSeconds = params->frames_to_seconds( absoluteTimeAcc( particle ) );
            }
            if( useTimeOffsetChannel ) {
                requestTimeInSeconds += params->frames_to_seconds( timeOffsetAcc( particle ) );
            }
            if( useGeomTimeChannel ) {
                requestTimeInSeconds += params->frames_to_seconds( geomTimeAcc( particle ) );
            }
            if( randomizeTimeOffsetByID ) {
                requestTimeInSeconds +=
                    get_unit_random_from_id( id, geometrySampleTimeSeed ) * maxRandomizedTimeOffsetInSeconds;
            }

            if( currentElement < 0 || static_cast<std::size_t>( currentElement ) >= shapeCount ) {
                throw std::runtime_error( "Internal Error: Random shape selection is out of range." );
            }

            // add the particle mesh to the output
            const float scale = radiusAcc( particle );
            const frantic::graphics::transform4f xform = transform4f::from_translation( positionAcc( particle ) ) *
                                                         transform4f::from_scale( scale ) *
                                                         rotationAndDivergenceTransform;

            int shapeIndex;
            if( instanceGeometryType == frost::geometry_type::custom_geometry ) {
                // INode* node = m_frost->get_custom_geometry( currentElement );
                TimeValue requestTimeInTicks = requestTimeInSeconds * TIME_TICKSPERSEC;
                node_and_time_t key( currentElement, requestTimeInTicks );
                node_and_time_to_shape_index_t::iterator i = nodeAndTimeToShapeIndex.find( key );
                if( i == nodeAndTimeToShapeIndex.end() ) {
                    shapeIndex = m_shapes.size();
                    mesh_and_accessors* meshAndAccessors =
                        instanceGeometryFactory->get_instance_geometry( requestTimeInSeconds, currentElement );
                    m_shapes.resize( shapeIndex + 1 );
                    m_shapes[shapeIndex].init( meshAndAccessors->mesh, meshCpp, vray->getRayServer() );
#if VRAY_DLL_VERSION < 0x40000
                    // buildTree causes an AccessException on subsequent renders without changing
                    // any settings in V-Ray 4.
                    // We're disabling this call in V-Ray 4 for now, since we're not sure
                    // it's actually necessary in the first place.
                    m_shapes[shapeIndex].meshInterface->getVoxel( 0 )->buildTree(
                        vray->getSequenceData().threadManager );
#endif
                    nodeAndTimeToShapeIndex[key] = shapeIndex;
                } else {
                    shapeIndex = i->second;
                }
            } else {
                shapeIndex = currentElement;
            }

            transforms.push_back( to_vray( xform ) );
            m_shapeIndices.push_back( shapeIndex );
            if( particle.size() > 0 ) {
                m_particles.push_back( &particle[0], cma );
            }
        }
    } else {
        // TODO: what?
    }

    m_staticPrimitives.reset();
    m_movingPrimitives.reset();
    if( m_useMovingPrimitives ) {
        m_movingPrimitives.reset( new moving_primitive_t[transforms.size()] );

        for( size_t i = 0; i < transforms.size(); ++i ) {
            trace_transform_t tms[2];

            const vray_vector_t initialOffset = ( mbCenter - mbDuration / 2 ) * get_particle_motion_per_frame( i );
            const vray_vector_t motion = get_particle_motion_per_exposure( i );

            tms[0] = trace_transform_t( transforms[i] );
            tms[1] = trace_transform_t( transforms[i] );

            tms[0].offs += initialOffset;
            tms[1].offs += initialOffset + motion;

            VUtils::MeshInterface* meshInterface = m_shapes[m_shapeIndices[i]].meshInterface.get();
            VUtils::ThreadManager* threadManager = vray->getSequenceData().threadManager;
            float mbDuration = vray->getSequenceData().params.moblur.duration;

            m_movingPrimitives[i].init( meshInterface, 2, tms, threadManager, this, mbDuration );
        }
    } else {
        m_staticPrimitives.reset( new static_primitive_t[transforms.size()] );

        for( size_t i = 0; i < transforms.size(); ++i ) {
            VUtils::MeshInterface* meshInterface = m_shapes[m_shapeIndices[i]].meshInterface.get();
            VUtils::ThreadManager* threadManager = vray->getSequenceData().threadManager;

            m_staticPrimitives[i].init( meshInterface, trace_transform_t( transforms[i] ), threadManager, this );
        }
    }
}

void FrostRenderInstance::frameBegin( TimeValue t, VRayCore* vray ) {
    try {
        VRenderInstance::frameBegin( t, vray );

        // TODO: do we need to reset this every frame?
        m_firstPrimitiveId = -1;

        objToWorld = node->GetObjTMAfterWSM( t );
        Matrix3 worldToObj = Inverse( objToWorld );

#if VRAY_DLL_VERSION < 0x40000
        camToObj = toMatrix3( toTransform( worldToObj ) * VUtils::Transform( vray->getFrameData().camToWorld ) );
        objToCam = toMatrix3( VUtils::Transform( vray->getFrameData().worldToCam ) * toTransform( objToWorld ) );
#else
        camToObj = toMatrix3( toTransform3f( worldToObj ) * vray->getFrameData().camToWorld );
        objToCam = toMatrix3( vray->getFrameData().worldToCam * toTransform3f( objToWorld ) );
#endif

        tm = toTransform( objToWorld );
        itm = inverse( tm );

        mesh = &m_dummyMesh;

#if VRAY_DLL_VERSION < 0x60000
        VR::VRayRenderGlobalContext2* globContext2 = VR::getRenderGlobalContext2();
#else
        VR::VRayRenderGlobalContext2* globContext2 = VR::getRenderGlobalContext2(VR::VRayServerType::vrayServer_production);
#endif
        generateInstances( t, vray, globContext2 );
    } catch( std::exception& e ) {
        report_error( vray, "FrostRenderInstance::frameBegin", e.what() );
    }
}

void FrostRenderInstance::frameEnd( VRayCore* vray ) {
    VRenderInstance::frameEnd( vray );

    try {
        deleteInstances( vray );
    } catch( std::exception& e ) {
        report_error( vray, "FrostRenderInstance::frameEnd", e.what() );
    }
}

void FrostRenderInstance::compileGeometry( VRayCore* vray ) {
    ray_server_t* rayserver = vray->getRayServer();
    if( m_firstPrimitiveId == -1 ) {
        m_firstPrimitiveId = rayserver->getNewRenderIDArray( m_shapeIndices.size() );
    }
    if( m_useMovingPrimitives ) {
        for( int i = 0, ie = m_shapeIndices.size(); i < ie; ++i ) {
            rayserver->storeMovingPrimitive( &m_movingPrimitives[i], 0, m_firstPrimitiveId + i );
        }
    } else {
        for( int i = 0, ie = m_shapeIndices.size(); i < ie; ++i ) {
            rayserver->storeStaticPrimitive( &m_staticPrimitives[i], 0, m_firstPrimitiveId + i );
        }
    }
}

void FrostRenderInstance::setIntersectionData( RSRay& rsray, void* isd ) {
    VR::IntersectionData& isData = *( (VR::IntersectionData*)isd );
    VR::RSIntersection& is = rsray.is;

    VR::GeometryGenerator* voxelPrimitive = static_cast<VR::GeometryGenerator*>( rsray.is.primitive->owner );

    int faceIndex = rsray.is.extrai[0];

    isData.primitive = is.primitive;
    isData.skipTag = is.skipTag;
    isData.faceIndex = faceIndex;
    isData.sb = getShadeable();
    isData.sd = getExtTexMapping();
    isData.si = getExtShadeData();
    isData.volume = NULL;

    isData.bary = vray_vector_t( is.bary );
    isData.wpointCoeff = is.t;
    isData.gnormal = voxelPrimitive->getGNormal( rsray );
    isData.normal = voxelPrimitive->getNormal( rsray );
    isData.wpoint = rsray.p + trace_point_t( rsray.dir ) * rsray.is.t;

    isData.extra_int[0] = voxelPrimitive->owner->ownerIndex - m_firstPrimitiveId;

    VR::Vector p[3];
    if( !m_useMovingPrimitives ) {
        VR::StaticMeshVoxelPrimitive* mesh = static_cast<VR::StaticMeshVoxelPrimitive*>( voxelPrimitive );
        VR::StaticMeshInterfacePrimitive* meshOwner = static_cast<VR::StaticMeshInterfacePrimitive*>( mesh->owner );
        VR::MeshVoxel* voxel = mesh->voxel;

        VR::FaceTopoData* faces = voxel->getFaceTopoData();
        VR::VertGeomData* verts = voxel->getVertGeomData();
        const trace_transform_t tm = trace_transform_t( meshOwner->tm );

        p[0] = tm * ( verts[faces[faceIndex].v[0]] );
        p[1] = tm * ( verts[faces[faceIndex].v[1]] );
        p[2] = tm * ( verts[faces[faceIndex].v[2]] );
    } else {
        VR::MovingMeshVoxelPrimitive* mesh = static_cast<VR::MovingMeshVoxelPrimitive*>( voxelPrimitive );
        VR::MovingMeshInterfacePrimitive* meshOwner = static_cast<VR::MovingMeshInterfacePrimitive*>( mesh->owner );
        VR::MeshVoxel* voxel = mesh->voxel;

        VR::FaceTopoData* faces = voxel->getFaceTopoData();
        VR::VertGeomData* verts = voxel->getVertGeomData();
        const trace_transform_t tm0 = trace_transform_t( meshOwner->tms[mesh->timeIndex] );
        const trace_transform_t tm1 = trace_transform_t( meshOwner->tms[mesh->timeIndex + 1] );
        const float t0 = meshOwner->times[mesh->timeIndex];
        const float t1 = meshOwner->times[mesh->timeIndex + 1];
        const float k = ( rsray.time - t0 ) / ( t1 - t0 );

        for( int i = 0; i < 3; ++i ) {
            const VUtils::Vector v = verts[faces[faceIndex].v[i]];
            VR::Vector p0 = tm0 * v;
            VR::Vector p1 = tm1 * v;
            p[i] = p0 * ( 1 - k ) + p1 * k;
        }
    }

    isData.faceBase = vray_vector_t( p[0] );
    isData.faceEdge0 = vray_vector_t( p[1] - p[0] );
    isData.faceEdge1 = vray_vector_t( p[2] - p[0] );

    isData.surfaceProps = getExtSurfaceProperties();
}

#if VRAY_DLL_VERSION < 0x40000
// This hook is not present in the V-Ray 4 SDK.

void FrostRenderInstance::setIntersectionData( const RayBunchParams& params, PrimitiveIntersections& result,
                                               const RAY_IDX* idxs, size_t count ) {
    // Iterate over the active rays
    for( int ridx = 0; ridx < count; ridx++ ) {
        const RAY_IDX index = idxs[ridx];

        VUtils::GenericPrimitive* genPrim = static_cast<VUtils::GenericPrimitive*>( result.primitives()[index]->owner );

        const VR::Vector gnormal = genPrim->getGNormal( params, result, index );
        const VR::Vector normal = genPrim->getNormal( params, result, index );

        for( int d = 0; d < 3; d++ ) {
            result.isectPoints( d )[index] =
                params.origins( d )[index] + params.dirs( d )[index] * result.rayDistances()[index];
            result.geomNormals( d )[index] = gnormal[d];
            result.smoothNormals( d )[index] = normal[d];
        }

        VR::Vector p[3];

        if( !m_useMovingPrimitives ) {
            VUtils::StaticMeshVoxelPrimitive* mesh = static_cast<VR::StaticMeshVoxelPrimitive*>( genPrim );
            VUtils::StaticMeshInterfacePrimitive* meshOwner =
                static_cast<VUtils::StaticMeshInterfacePrimitive*>( mesh->owner );
            VUtils::MeshVoxel* voxel = mesh->voxel;

            const int faceIndex = result.extraInts( 0 )[index];
            result.faceIndices()[index] = faceIndex;

            const VUtils::TraceTransform tm = VUtils::TraceTransform( meshOwner->tm );

            VUtils::FaceTopoData* faces = voxel->getFaceTopoData();
            VUtils::VertGeomData* verts = voxel->getVertGeomData();

            for( int i = 0; i < 3; i++ ) {
                p[i] = tm * ( verts[faces[faceIndex].v[i]] );
            }

            for( int d = 0; d < 3; d++ ) {
                result.facesEdge0( d )[index] = p[1][d] - p[0][d];
                result.facesEdge1( d )[index] = p[2][d] - p[0][d];
            }

            for( int d = 0; d < 3; d++ ) {
                result.facesBase( d )[index] = p[0][d];
            }
        } else {
            VUtils::MovingMeshVoxelPrimitive* mesh = static_cast<VR::MovingMeshVoxelPrimitive*>( genPrim );
            VUtils::MovingMeshInterfacePrimitive* meshOwner =
                static_cast<VUtils::MovingMeshInterfacePrimitive*>( mesh->owner );
            VUtils::MeshVoxel* voxel = mesh->voxel;

            const int faceIndex = result.extraInts( 0 )[index];
            result.faceIndices()[index] = faceIndex;

            VUtils::FaceTopoData* faces = voxel->getFaceTopoData();
            VUtils::VertGeomData* verts = voxel->getVertGeomData();

            const VUtils::TraceTransform tm0 = VUtils::TraceTransform( meshOwner->tms[mesh->timeIndex] );
            const VUtils::TraceTransform tm1 = VUtils::TraceTransform( meshOwner->tms[mesh->timeIndex + 1] );

            const float t0 = meshOwner->times[mesh->timeIndex];
            const float t1 = meshOwner->times[mesh->timeIndex + 1];

            float k = ( ( *( params.times() + index ) ) - t0 ) / ( t1 - t0 );

            for( int i = 0; i < 3; i++ ) {
                VR::Vector v = verts[faces[faceIndex].v[i]];
                VR::Vector p0 = tm0 * v;
                VR::Vector p1 = tm1 * v;
                p[i] = p0 * ( 1.0f - k ) + p1 * k;
            }

            for( int d = 0; d < 3; d++ ) {
                result.facesBase( d )[index] = p[0][d];
                result.facesEdge0( d )[index] = p[1][d] - p[0][d];
                result.facesEdge1( d )[index] = p[2][d] - p[0][d];
            }
        }

        // extraInts( 6 ) is copied by V-Ray into VRayContext::rayresult.extra_int[0]
        result.extraInts( 6 )[index] = genPrim->owner->ownerIndex - m_firstPrimitiveId;
    }
}

#endif

PluginInterface* FrostRenderInstance::newInterface( InterfaceID id ) {
    PluginInterface* pInteface = VR::Shadeable::newInterface( id );
    if( pInteface == NULL ) {
        pInteface = VR::ShadeData::newInterface( id );

        if( pInteface == NULL )
            pInteface = VR::ShadeInstance::newInterface( id );
    }

    return pInteface;
}

/// Return non-const shadeable object
Shadeable* FrostRenderInstance::getShadeable() { return static_cast<Shadeable*>( this ); }

/// Return additional shading data
VRayShadeInstance* FrostRenderInstance::getExtShadeData() { return static_cast<ShadeInstance*>( this ); }

/// Return additional texture mapping information
VRayShadeData* FrostRenderInstance::getExtTexMapping() { return static_cast<ShadeData*>( this ); }

VRayVolume* FrostRenderInstance::getVolumeShader() { return NULL; }

#if VRAY_DLL_VERSION < 0x60000
VRaySurfaceProperties* FrostRenderInstance::getExtSurfaceProperties() { return &surfaceProps; }
#else
const VRaySurfaceProperties* FrostRenderInstance::getExtSurfaceProperties() { return getSurfaceProps(); }
#endif

// From Shadeable
void FrostRenderInstance::shade( VRayContext& vri ) { VRenderInstance::fullShade( (VRayInterface&)vri ); }

namespace {

vray_vector_t get_barycentric( const VUtils::VertGeomData* data, const VUtils::FaceTopoData& face,
                               const vray_vector_t& bary ) {
    return vray_vector_t( bary[0] * data[face.v[0]] + bary[1] * data[face.v[1]] + bary[2] * data[face.v[2]] );
}

} // anonymous namespace

// From ShadeData
vray_vector_t FrostRenderInstance::getUVWcoords( const VR::VRayContext& vri, int channel ) {
    try {
        int particleIndex = vri.rayresult.extra_int[0];

        // Special case for -2:Alpha and -1:Illumination.  I added this
        // because excessive "out of bounds" errors for these channels
        // were causing problems for a client.
        if( channel == -2 || channel == -1 ) {
            return vray_vector_t( 0, 0, 0 );
        }

        if( channel < 0 || channel >= m_mapChannelAccessors.size() ) {
            throw frantic::exception_stream() << "Channel index (" << channel << ") is out of bounds [0, "
                                              << m_mapChannelAccessors.size() << ") in particle channel accessors";
        }

        if( m_mapChannelAccessors[channel].is_valid() ) {
            if( particleIndex < 0 || particleIndex >= m_particles.size() ) {
                throw frantic::exception_stream() << "Particle index (" << particleIndex << ") is out of bounds [0, "
                                                  << m_particles.size() << ") in particles";
            }
            return to_vray_vector_t( m_mapChannelAccessors[channel]( m_particles[particleIndex] ) );
        } else {
            if( particleIndex < 0 || particleIndex >= m_shapeIndices.size() ) {
                throw frantic::exception_stream() << "Particle index (" << particleIndex << ") is out of bounds [0, "
                                                  << m_shapeIndices.size() << ") in shape indices";
            }
            int shapeIndex = m_shapeIndices[particleIndex];
            int faceIndex = vri.rayresult.faceIndex;

            VUtils::MeshInterface* mesh = get_shape_mesh( shapeIndex );

            VUtils::MeshVoxel* voxel = mesh->getVoxel( 0 );

            VUtils::MeshChannel* dataChannel = voxel->getUVWChannel( channel );
            if( dataChannel ) {
                VUtils::MeshChannel* topoChannel = voxel->getChannel( dataChannel->depChannelID );
                if( topoChannel ) {
                    VUtils::VertGeomData* data = (VUtils::VertGeomData*)dataChannel->data;
                    if( !data ) {
                        throw frantic::exception_stream() << "Data is NULL for map channel " << channel;
                    }
                    VUtils::FaceTopoData* faces = (VUtils::FaceTopoData*)topoChannel->data;
                    if( !faces ) {
                        throw frantic::exception_stream() << "Faces are NULL for map channel " << channel;
                    }

                    return get_barycentric( data, faces[faceIndex], vri.rayresult.bary );
                } else {
                    throw frantic::exception_stream() << "Missing topology channel for map channel " << channel;
                }
            } else {
                return vray_vector_t( 0, 0, 0 );
            }
        }
    } catch( std::exception& e ) {
        report_error( vri.vray, "FrostRenderInstance::getUVWcoords", e.what() );
    }

    return vray_vector_t( 0, 0, 0 );
}

#if VRAY_DLL_VERSION < 0x50000
void FrostRenderInstance::getUVWderivs( const VR::VRayContext& vri, int channel, vray_vector_t derivs[2] ) {
#else
void FrostRenderInstance::getUVWderivs( const VR::VRayContext& vri, int channel, vray_vector_t derivs[2],
                                        const UVWFlags uvwFlags ) {
#endif
    const int particleIndex = vri.rayresult.extra_int[0];
    const int shapeIndex = m_shapeIndices[particleIndex];
    const int faceIndex = vri.rayresult.faceIndex;
    VUtils::MeshInterface* mesh = get_shape_mesh( shapeIndex );
    VUtils::MeshVoxel* voxel = mesh->getVoxel( 0 );

    VUtils::MeshChannel* dataChannel = voxel->getUVWChannel( channel );
    if( dataChannel ) {
        VUtils::MeshChannel* topoChannel = voxel->getChannel( dataChannel->depChannelID );
        if( topoChannel ) {
            VUtils::VertGeomData* data = (VUtils::VertGeomData*)dataChannel->data;
            if( !data ) {
                throw frantic::exception_stream() << "Data is NULL for map channel " << channel;
            }
            VUtils::FaceTopoData* faces = (VUtils::FaceTopoData*)topoChannel->data;
            if( !faces ) {
                throw frantic::exception_stream() << "Faces are NULL for map channel " << channel;
            }
            const VR::FaceTopoData face = faces[faceIndex];
            const int vertex0 = face.v[0], vertex1 = face.v[1], vertex2 = face.v[2];
            const VR::VRayFrameData& frameData = vri.vray->getFrameData();
            const trace_transform_t worldToCameraMatrix = frameData.worldToCam;

            // Information about the triangle vertices
            const vray_vector_t uvw0 = vray_vector_t( data[vertex0] );
            const vray_vector_t uvw1 = vray_vector_t( data[vertex1] );
            const vray_vector_t uvw2 = vray_vector_t( data[vertex2] );
            const vray_vector_t worldPos0 = vri.rayresult.faceBase; // world space position
            const vray_vector_t worldPos1 = vri.rayresult.faceBase + vri.rayresult.faceEdge0;
            const vray_vector_t worldPos2 = vri.rayresult.faceBase + vri.rayresult.faceEdge1;
            const vray_vector_t cameraPos0 = worldToCameraMatrix * worldPos0; // camera space position
            const vray_vector_t cameraPos1 = worldToCameraMatrix * worldPos1;
            const vray_vector_t cameraPos2 = worldToCameraMatrix * worldPos2;
            const vray_vector_t cameraPos = worldToCameraMatrix * vri.rayresult.origPoint;

            // Apply the perspective transformation if applicable and convert units to pixels
            float x0, x1, x2, y0, y1, y2, z0, z1, z2, x, y, z; // screen space positions, after perspective transform
            if( frameData.projType == 0 ) {
                const float scaling = frameData.imgWidth / ( 2 * std::tan( frameData.fov / 2 ) );
                x0 = cameraPos0[0] * scaling / cameraPos0[2];
                y0 = cameraPos0[1] * scaling / cameraPos0[2];
                z0 = cameraPos0[2] * scaling;
                x1 = cameraPos1[0] * scaling / cameraPos1[2];
                y1 = cameraPos1[1] * scaling / cameraPos1[2];
                z1 = cameraPos1[2] * scaling;
                x2 = cameraPos2[0] * scaling / cameraPos2[2];
                y2 = cameraPos2[1] * scaling / cameraPos2[2];
                z2 = cameraPos2[2] * scaling;
                x = cameraPos[0] * scaling / cameraPos[2];
                y = cameraPos[1] * scaling / cameraPos[2];
                z = cameraPos[2] * scaling;
            } else {
#if VRAY_DLL_VERSION < 0x50000
                const float scaling = frameData.imgWidth / ( 2 * frameData.zoomFractor ); // [sic]
#else
                const float scaling = frameData.imgWidth / ( 2 * frameData.zoomFactor );
#endif
                x0 = cameraPos0[0] * scaling;
                y0 = cameraPos0[1] * scaling;
                z0 = cameraPos0[2] * scaling;
                x1 = cameraPos1[0] * scaling;
                y1 = cameraPos1[1] * scaling;
                z1 = cameraPos1[2] * scaling;
                x2 = cameraPos2[0] * scaling;
                y2 = cameraPos2[1] * scaling;
                z2 = cameraPos2[2] * scaling;
                x = cameraPos[0] * scaling;
                y = cameraPos[1] * scaling;
                z = cameraPos[2] * scaling;
            }

            const float alpha0 = y1 - y2;
            const float beta0 = x2 - x1;
            const float gamma0 = x1 * y2 - x2 * y1;
            const float A0 = alpha0 * x + beta0 * y + gamma0;

            const float alpha1 = y2 - y0;
            const float beta1 = x0 - x2;
            const float gamma1 = x2 * y0 - x0 * y2;
            const float A1 = alpha1 * x + beta1 * y + gamma1;

            const float alpha2 = y0 - y1;
            const float beta2 = x1 - x0;
            const float gamma2 = x0 * y1 - x1 * y0;
            const float A2 = alpha2 * x + beta2 * y + gamma2;

            const float totalArea = A0 + A1 + A2;
            const vray_vector_t uvw =
                get_barycentric( data, face, vray_vector_t( A0 / totalArea, A1 / totalArea, A2 / totalArea ) );

            const vray_vector_t duvwdx = ( z1 * z2 * alpha0 * ( uvw0 - uvw ) + z2 * z0 * alpha1 * ( uvw1 - uvw ) +
                                           z0 * z1 * alpha2 * ( uvw2 - uvw ) ) /
                                         ( z1 * z2 * A0 + z2 * z0 * A1 + z0 * z1 * A2 );
            const vray_vector_t duvwdy = ( z1 * z2 * beta0 * ( uvw0 - uvw ) + z2 * z0 * beta1 * ( uvw1 - uvw ) +
                                           z0 * z1 * beta2 * ( uvw2 - uvw ) ) /
                                         ( z1 * z2 * A0 + z2 * z0 * A1 + z0 * z1 * A2 );

            derivs[0] = duvwdx;
            derivs[1] = duvwdy;

        } else {
            throw frantic::exception_stream() << "Missing topology channel for map channel " << channel;
        }
    } else {
        // Nonsense value.
        derivs[0].makeZero();
        derivs[1].makeZero();
    }
}
#if VRAY_DLL_VERSION < 0x50000
void FrostRenderInstance::getUVWbases( const VR::VRayContext& vri, int channel, vray_vector_t bases[3] ) {
#else
void FrostRenderInstance::getUVWbases( const VR::VRayContext& vri, int channel, vray_vector_t bases[3],
                                       const UVWFlags uvwFlags ) {
#endif
    // Return the u, v and w direction (normalized)
    bases[0] = vray_vector_t( normalize( tm.m[0] ) );
    bases[1] = vray_vector_t( normalize( tm.m[1] ) );
    bases[2] = vray_vector_t( normalize( tm.m[2] ) );
}

vray_vector_t FrostRenderInstance::getUVWnormal( const VR::VRayContext& vri, int channel ) {
    return vray_vector_t( normalize( tm.m[2] ) );
}

int FrostRenderInstance::getMtlID( const VRayContext& vri ) {
    try {
        typedef boost::uint16_t material_id_t;

        int particleIndex = vri.rayresult.extra_int[0];

        switch( m_materialMode ) {
        case frost::material_mode::single:
            return 0;
        case frost::material_mode::mtlindex_channel:
            if( m_materialIdAccessor.is_valid() ) {
                if( particleIndex < 0 || particleIndex >= m_particles.size() ) {
                    throw frantic::exception_stream()
                        << "Particle index (" << particleIndex << ") is out of bounds [0, " << m_particles.size()
                        << ") in particles";
                }
                return m_materialIdAccessor( m_particles[particleIndex] );
            }
            break;
        case frost::material_mode::shape_number: {
            if( particleIndex < 0 || particleIndex >= m_shapeIndices.size() ) {
                throw frantic::exception_stream() << "Particle index (" << particleIndex << ") is out of bounds [0, "
                                                  << m_shapeIndices.size() << ") in shape indices";
            }
            int shapeIndex = m_shapeIndices[particleIndex];
            return shapeIndex % ( static_cast<int>( std::numeric_limits<material_id_t>::max() ) + 1 );
        } break;
        case frost::material_mode::material_id_from_geometry:
        case frost::material_mode::material_from_geometry: {
            if( particleIndex < 0 || particleIndex >= m_shapeIndices.size() ) {
                throw frantic::exception_stream() << "Particle index (" << particleIndex << ") is out of bounds [0, "
                                                  << m_shapeIndices.size() << ") in shape indices";
            }

            int shapeIndex = m_shapeIndices[particleIndex];
            if( shapeIndex < 0 || shapeIndex >= m_shapes.size() ) {
                throw frantic::exception_stream()
                    << "Shape index (" << shapeIndex << ") is out of bounds [0, " << m_shapes.size() << ") in shapes";
            }

            int faceIndex = vri.rayresult.faceIndex;

            const std::vector<boost::uint16_t>& materialIDs = m_shapes[shapeIndex].materialIDs;

            if( materialIDs.empty() ) {
                return 0;
            } else {
                if( faceIndex < 0 || faceIndex >= materialIDs.size() ) {
                    throw frantic::exception_stream() << "Face index (" << faceIndex << ") is out of bounds [0, "
                                                      << materialIDs.size() << ") in shape material IDs";
                }
                return materialIDs[faceIndex];
            }
        } break;
        }
    } catch( std::exception& e ) {
        report_error( vri.vray, "FrostRenderInstance::getMtlID", e.what() );
    }

    return 0;
}

int FrostRenderInstance::getEdgeVisibility( const VRayContext& vri ) {
    try {
        int particleIndex = vri.rayresult.extra_int[0];
        if( particleIndex < 0 || particleIndex >= m_shapeIndices.size() ) {
            throw frantic::exception_stream() << "Particle index (" << particleIndex << ") is out of bounds [0, "
                                              << m_shapeIndices.size() << ") in shape indices";
        }
        int shapeIndex = m_shapeIndices[particleIndex];
        if( shapeIndex < 0 || shapeIndex >= m_shapes.size() ) {
            throw frantic::exception_stream()
                << "Shape index (" << shapeIndex << ") is out of bounds [0, " << m_shapes.size() << ") in shapes";
        }
        const std::vector<boost::int8_t>& edgeVisibility = m_shapes[shapeIndex].edgeVisibility;
        if( edgeVisibility.empty() ) {
            return 7;
        } else {
            int faceIndex = vri.rayresult.faceIndex;
            if( faceIndex < 0 || faceIndex >= edgeVisibility.size() ) {
                throw frantic::exception_stream() << "Face index (" << faceIndex << ") is out of bounds [0, "
                                                  << edgeVisibility.size() << ") in shape edge visibility";
            }
            return edgeVisibility[faceIndex];
        }
    } catch( std::exception& e ) {
        report_error( vri.vray, "FrostRenderInstance::getEdgeVisibility", e.what() );
    }

    return 7;
}

// From ShadeInstance
vray_vector_t FrostRenderInstance::getBasePt( const VR::VRayContext& vri, BaseTime baseTime ) {
    return vri.rayresult.wpoint;
}
vray_vector_t FrostRenderInstance::getBasePtObj( const VR::VRayContext& vri ) { return itm * vri.rayresult.wpoint; }

vray_vector_t FrostRenderInstance::worldToObjectVec( const VR::VRayContext& vri, const vray_vector_t& d ) {
    return itm.m * d;
}
vray_vector_t FrostRenderInstance::worldToObjectPt( const VR::VRayContext& vri, const vray_vector_t& p ) {
    return vray_vector_t( itm * p );
}
vray_vector_t FrostRenderInstance::objectToWorldVec( const VR::VRayContext& vri, const vray_vector_t& d ) {
    return vray_vector_t( tm.m * d );
}
vray_vector_t FrostRenderInstance::objectToWorldPt( const VR::VRayContext& vri, const vray_vector_t& p ) {
    return vray_vector_t( tm * p );
}

vray_vector_t FrostRenderInstance::getBaseGNormal( const VR::VRayContext& vri, BaseTime baseTime ) {
    return vri.rayresult.gnormal;
}
vray_vector_t FrostRenderInstance::getBaseNormal( const VR::VRayContext& vri, BaseTime baseTime ) {
    return vri.rayresult.origNormal;
}
vray_vector_t FrostRenderInstance::getBaseBumpNormal( const VR::VRayContext& vri, BaseTime baseTime ) {
    return vri.rayresult.normal;
}

trace_point_t FrostRenderInstance::getShadowPt( const VR::VRayContext& vri ) { return vri.rayresult.wpoint; }
vray_vector_t FrostRenderInstance::getVelocity( const VR::VRayContext& vri ) {
    try {
        if( m_velocityAccessor.is_valid() ) {
            int particleIndex = vri.rayresult.extra_int[0];

            if( particleIndex < 0 || particleIndex >= m_particles.size() ) {
                throw frantic::exception_stream() << "Particle index (" << particleIndex << ") is out of bounds [0, "
                                                  << m_particles.size() << ") in particles";
            }

            return to_vray_vector_t( m_secondsPerFrame * m_velocityAccessor( m_particles[particleIndex] ) );
        }
    } catch( std::exception& e ) {
        report_error( vri.vray, "FrostRenderInstance::getVelocity", e.what() );
    }

    return vray_vector_t( 0, 0, 0 );
}

vray_vector_t FrostRenderInstance::get_particle_motion_per_exposure( int particleIndex ) {
    if( m_velocityAccessor.is_valid() ) {
        return to_vray_vector_t( m_secondsPerMotionBlurInterval * m_velocityAccessor( m_particles[particleIndex] ) );
    } else {
        return vray_vector_t( 0, 0, 0 );
    }
}

vray_vector_t FrostRenderInstance::get_particle_motion_per_frame( int particleIndex ) {
    if( m_velocityAccessor.is_valid() ) {
        return to_vray_vector_t( m_secondsPerFrame * m_velocityAccessor( m_particles[particleIndex] ) );
    } else {
        return vray_vector_t( 0, 0, 0 );
    }
}
