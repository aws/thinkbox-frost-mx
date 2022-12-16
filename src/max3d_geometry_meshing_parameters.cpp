// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "Frost.hpp"

#include "max3d_geometry_meshing_parameters.hpp"

#include <ImathMatrixAlgo.h>
#include <decomp.h>

using namespace frost;

using frost::max3d::max3d_geometry_meshing_parameters;

namespace {

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

} // anonymous namespace

max3d_geometry_meshing_parameters::max3d_geometry_meshing_parameters( Frost* node, TimeValue t )
    : m_t( t )
    , m_node( node ) {}

double max3d_geometry_meshing_parameters::get_time() { return m_t / static_cast<double>( TIME_TICKSPERSEC ); }

geometry_type::option max3d_geometry_meshing_parameters::get_geometry_type() {
    const int instanceGeometryType = m_node->m_geometryType.at_time( m_t );

    switch( instanceGeometryType ) {
    case GEOMETRY_TYPE::PLANE:
        return frost::geometry_type::plane;
    case GEOMETRY_TYPE::SPRITE:
        return frost::geometry_type::sprite;
    case GEOMETRY_TYPE::TETRAHEDRON:
        return frost::geometry_type::tetrahedron;
    case GEOMETRY_TYPE::CUSTOM_GEOMETRY:
        return frost::geometry_type::custom_geometry;
    case GEOMETRY_TYPE::BOX:
        return frost::geometry_type::box;
    case GEOMETRY_TYPE::SPHERE20:
        return frost::geometry_type::sphere20;
    default:
        throw std::runtime_error( "get_geometry_type Error: unknown geometry type: " +
                                  boost::lexical_cast<std::string>( instanceGeometryType ) );
    }
}

hard_edge_type::option max3d_geometry_meshing_parameters::get_hard_edge_type() {
    return hard_edge_type::smoothing_group;
}

geometry_selection_mode::option max3d_geometry_meshing_parameters::get_geometry_selection_mode() {
    const int randomSelectionMode = m_node->m_geometrySelectionMode.at_time( m_t );

    switch( randomSelectionMode ) {
    case GEOMETRY_SELECTION_MODE::CYCLE:
        return frost::geometry_selection_mode::cycle;
    case GEOMETRY_SELECTION_MODE::RANDOM_BY_ID:
        return frost::geometry_selection_mode::random_by_id;
    case GEOMETRY_SELECTION_MODE::SHAPEINDEX_CHANNEL:
        return frost::geometry_selection_mode::shapeindex_channel;
    default:
        throw std::runtime_error( "get_geometry_selection_mode Error: unknown geometry selection mode: " +
                                  boost::lexical_cast<std::string>( randomSelectionMode ) );
    }
}
boost::uint32_t max3d_geometry_meshing_parameters::get_geometry_selection_seed() {
    return m_node->m_geometrySelectionSeed.at_time( m_t );
}

geometry_sample_time_base_mode::option max3d_geometry_meshing_parameters::get_geometry_sample_time_base_mode() {
    const int baseTimeMode = m_node->m_geometrySampleTimeBaseMode.at_time( m_t );
    switch( baseTimeMode ) {
    case GEOMETRY_TIMING_BASE_MODE::TIME_0:
        return frost::geometry_sample_time_base_mode::time_0;
    case GEOMETRY_TIMING_BASE_MODE::CURRENT_TIME:
        return frost::geometry_sample_time_base_mode::current_time;
    default:
        throw std::runtime_error( "get_geometry_sample_time_base Error: unknown sample time base: " +
                                  boost::lexical_cast<std::string>( baseTimeMode ) );
    }
}
geometry_sample_time_offset_mode::option max3d_geometry_meshing_parameters::get_geometry_sample_time_offset_mode() {
    const int instanceGeometryAnimationMode = m_node->m_geometrySampleTimeOffsetMode.at_time( m_t );
    switch( instanceGeometryAnimationMode ) {
    case GEOMETRY_TIMING_OFFSET_MODE::NO_OFFSET:
        return frost::geometry_sample_time_offset_mode::no_offset;
    case GEOMETRY_TIMING_OFFSET_MODE::RANDOM_BY_ID:
        return frost::geometry_sample_time_offset_mode::random_by_id;
    case GEOMETRY_TIMING_OFFSET_MODE::ABSTIME_CHANNEL:
        return frost::geometry_sample_time_offset_mode::abstime_channel;
    case GEOMETRY_TIMING_OFFSET_MODE::TIMEOFFSET_CHANNEL:
        return frost::geometry_sample_time_offset_mode::timeoffset_channel;
    case GEOMETRY_TIMING_OFFSET_MODE::GEOMTIME_CHANNEL:
        return frost::geometry_sample_time_offset_mode::geomtime_channel;
    default:
        throw std::runtime_error( "get_geometry_sample_time_offset_mode Error: unknown sample time offset mode: " +
                                  boost::lexical_cast<std::string>( instanceGeometryAnimationMode ) );
    }
}
double max3d_geometry_meshing_parameters::get_geometry_sample_time_max_random_offset() {
    return m_node->m_geometrySampleTimeMaxRandomOffset.at_time( m_t ) * GetTicksPerFrame() /
           static_cast<double>( TIME_TICKSPERSEC );
}
boost::uint32_t max3d_geometry_meshing_parameters::get_geometry_sample_time_seed() {
    return m_node->m_geometrySampleTimeSeed.at_time( m_t );
}

double max3d_geometry_meshing_parameters::frames_to_seconds( double frames ) {
    return frames * GetTicksPerFrame() / TIME_TICKSPERSEC;
}

geometry_orientation_mode::option max3d_geometry_meshing_parameters::get_geometry_orientation_mode() {
    const int orientationMethod = m_node->m_geometryOrientationMode.at_time( m_t ); // method of orientation
    switch( orientationMethod ) {
    case GEOMETRY_ORIENTATION_MODE::LOOK_AT:
        return frost::geometry_orientation_mode::look_at;
    case GEOMETRY_ORIENTATION_MODE::MATCH_OBJECT:
        return frost::geometry_orientation_mode::match_object;
    case GEOMETRY_ORIENTATION_MODE::SPECIFY:
        return frost::geometry_orientation_mode::specify;
    case GEOMETRY_ORIENTATION_MODE::ORIENTATION_CHANNEL:
        return frost::geometry_orientation_mode::orientation_channel;
    case GEOMETRY_ORIENTATION_MODE::VECTOR_CHANNEL:
        return frost::geometry_orientation_mode::vector_channel;
    default:
        throw std::runtime_error( "get_geometry_orientation_mode Error: unknown geometry orientation mode: " +
                                  boost::lexical_cast<std::string>( orientationMethod ) );
    }
}
frantic::graphics::vector3f max3d_geometry_meshing_parameters::get_geometry_orientation() {
    const float specifiedOrientationX =
        frantic::math::degrees_to_radians( (float)m_node->m_geometryOrientationX.at_time( m_t ) ); // initial rotation x
    const float specifiedOrientationY =
        frantic::math::degrees_to_radians( (float)m_node->m_geometryOrientationY.at_time( m_t ) ); // initial rotation y
    const float specifiedOrientationZ =
        frantic::math::degrees_to_radians( (float)m_node->m_geometryOrientationZ.at_time( m_t ) ); // initial rotation z
    // NOTE!! negative because from_euler_xyz_rotation_degrees()
    // seems to produce a backward rotation for z
    const frantic::graphics::vector3f specifiedOrientation( specifiedOrientationX, specifiedOrientationY,
                                                            -specifiedOrientationZ );
    return specifiedOrientation;
}
frantic::graphics::vector3f max3d_geometry_meshing_parameters::get_geometry_look_at_position() {
    INode* targetNode = m_node->m_geometryOrientationLookAtNode.at_time( m_t );
    if( targetNode ) {
        m_cachedLookAtTransform = targetNode->GetObjTMAfterWSM( m_t );
    }
    return m_node->get_orientation_look_at_pos( m_t );
}
frantic::graphics::vector3f max3d_geometry_meshing_parameters::get_geometry_look_at_orientation() {
    frantic::graphics::vector3f result;
    INode* targetNode = m_node->m_geometryOrientationLookAtNode.at_time( m_t );
    if( targetNode ) {
        Matrix3 tm = targetNode->GetObjTMAfterWSM( m_t );
        m_cachedLookAtTransform = tm;
        // m_cachedTargetTm = tm;
        result = extract_euler_xyz( tm );
        // NOTE!! negative because from_euler_xyz_rotation_degrees()
        // seems to produce a backward rotation for z
        result.z = -result.z;
    }
    return result;
}
frantic::tstring max3d_geometry_meshing_parameters::get_geometry_orientation_vector_channel() {
    return m_node->m_geometryOrientationVectorChannel.at_time( m_t );
}
float max3d_geometry_meshing_parameters::get_geometry_orientation_divergence() {
    return m_node->m_geometryOrientationDivergence.at_time( m_t );
}

bool max3d_geometry_meshing_parameters::get_orientation_restrict_divergence_axis() {
    return m_node->m_geometryOrientationRestrictDivergenceAxis.at_time( m_t );
}
frantic::graphics::vector3f max3d_geometry_meshing_parameters::get_orientation_divergence_axis() {
    const float divergenceAxisX = m_node->m_geometryOrientationDivergenceAxisX.at_time( m_t );
    const float divergenceAxisY = m_node->m_geometryOrientationDivergenceAxisY.at_time( m_t );
    const float divergenceAxisZ = m_node->m_geometryOrientationDivergenceAxisZ.at_time( m_t );

    const frantic::graphics::vector3f inputDivergenceAxis( divergenceAxisX, divergenceAxisY, divergenceAxisZ );
    const float inputDivergenceAxisMagnitude = inputDivergenceAxis.get_magnitude();
    frantic::graphics::vector3f divergenceAxis = frantic::graphics::vector3f::from_zaxis();
    if( frantic::math::is_finite( inputDivergenceAxisMagnitude ) &&
        inputDivergenceAxisMagnitude > std::numeric_limits<float>::epsilon() ) {
        divergenceAxis = inputDivergenceAxis / inputDivergenceAxisMagnitude;
    }
    return divergenceAxis;
}
geometry_orientation_divergence_axis_space::option
max3d_geometry_meshing_parameters::get_geometry_orientation_divergence_axis_space() {
    const int divergenceAxisSpaceVal = m_node->m_geometryOrientationDivergenceAxisSpace.at_time( m_t );
    switch( divergenceAxisSpaceVal ) {
    case AXIS_SPACE::WORLD:
        return frost::geometry_orientation_divergence_axis_space::world;
    case AXIS_SPACE::LOCAL:
        return frost::geometry_orientation_divergence_axis_space::local;
    default:
        throw std::runtime_error(
            "get_geometry_orientation_divergence_axis_space Error: unknown geometry orientation axis space: " +
            boost::lexical_cast<std::string>( divergenceAxisSpaceVal ) );
    }
}

Matrix3 max3d_geometry_meshing_parameters::get_cached_look_at_transform() { return m_cachedLookAtTransform; }
