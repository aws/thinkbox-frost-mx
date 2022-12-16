// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/timer.hpp>

#include <frantic/channels/channel_propagation_policy.hpp>
#include <frantic/channels/named_channel_data.hpp>

#include <frantic/files/filename_sequence.hpp>

#include <frantic/logging/progress_logger.hpp>

#include <frantic/math/utils.hpp>

#include <frantic/particles/particle_grid_tree.hpp>
#include <frantic/particles/streams/apply_function_particle_istream.hpp>
#include <frantic/particles/streams/concatenated_particle_istream.hpp>
#include <frantic/particles/streams/empty_particle_istream.hpp>
#include <frantic/particles/streams/particle_array_particle_istream.hpp>
#include <frantic/particles/streams/particle_istream.hpp>
#include <frantic/particles/streams/set_channel_particle_istream.hpp>
#include <frantic/particles/streams/shared_particle_container_particle_istream.hpp>
#include <frantic/particles/streams/transformed_particle_istream.hpp>

#include <frantic/volumetrics/implicitsurface/calculate_particle_anisotropic_params.hpp>
#include <frantic/volumetrics/implicitsurface/implicit_surface_to_trimesh3.hpp>
#include <frantic/volumetrics/voxel_coord_system.hpp>

#include <frantic/max3d/geometry/mesh.hpp>
#include <frantic/max3d/max_utility.hpp>

#include <frantic/max3d/logging/status_panel_progress_logger.hpp>

#include <frantic/max3d/particles/streams/max_geometry_vert_particle_istream.hpp>
#include <frantic/max3d/particles/streams/max_iparticleobjext_particle_istream.hpp>
#include <frantic/max3d/particles/streams/max_legacy_particle_istream.hpp>
#include <frantic/max3d/particles/streams/max_pflow_particle_istream.hpp>
#include <frantic/max3d/particles/streams/ticks_to_seconds_particle_istream.hpp>
#include <frantic/max3d/particles/tp_interface.hpp>

#include <frantic/max3d/rendering/renderplugin_utils.hpp>

#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>

#pragma warning( push, 3 )
#pragma warning( disable : 4100 ) // warning C4100: unreferenced formal parameter
#include <gizmo.h>
#pragma warning( pop )

#pragma warning( push, 3 )
#pragma warning( disable : 4239 ) // warning C4239: nonstandard extension used : 'default argument' : conversion from
                                  // 'DefaultRemapDir' to 'RemapDir &'
#include <gizmoimp.h>
#pragma warning( pop )

#if MAX_VERSION_MAJOR >= 14
#include <Graphics/IDisplayManager.h>
#endif

#include "resource.h"

#include "Frost.hpp"

#include "utility.hpp"

#include "FrostClassDesc.hpp"
#include "frost_param_block_desc.hpp"

#include "frost_pb_accessor.hpp"

#include "frost_create_mouse_callback.hpp"
#include "frost_status_panel_progress_logger.hpp"

#include "anisotropic_dlg_proc.hpp"
#include "frost_help_dlg_proc.hpp"
#include "frost_main_dlg_proc.hpp"
#include "geometry_dlg_proc.hpp"
#include "material_dlg_proc.hpp"
#include "meshing_dlg_proc.hpp"
#include "meshing_quality_dlg_proc.hpp"
#include "metaballs_dlg_proc.hpp"
#include "particle_files_dlg_proc.hpp"
#include "particle_flow_events_dlg_proc.hpp"
#include "particle_objects_dlg_proc.hpp"
#include "presets_dlg_proc.hpp"
#include "roi_dlg_proc.hpp"
#include "tp_groups_dlg_proc.hpp"
#include "zhu_bridson_dlg_proc.hpp"

#include "geometry_list_validator.hpp"
#include "node_list_validator.hpp"
#include "orientation_look_at_validator.hpp"

#include "check_for_render_abort_progress_logger.hpp"

#include "apply_function_particle_istream_nothreads.hpp"
#include "box_culling_particle_istream.hpp"
#include "get_legacy_thinking_particles_radius_particle_istream.hpp"
#include "gizmo_particle_istream.hpp"
#include "thinking_particles_particle_istream.hpp"
#include <frost/create_id_channel_from_index_particle_istream.hpp>
#include <frost/positive_radius_culled_particle_istream.hpp>
// exclude for now; I was using this to mesh Krakatoa Kinect directly
//#include "krakatoa_prt_source_particle_istream.hpp"

#include "Phoenix/MaxKrakatoaPhoenixParticleIstream.hpp"

#include "max3d_geometry_meshing_parameters.hpp"
#include "max3d_geometry_source.hpp"
#include <frost/geometry_meshing.hpp>

#include "trimesh3_culling.hpp"

#include "FrostIconMaterial.hpp"

#include <frost/utility.hpp>

// Interface for native V-Ray geometry
// Copied from vraygeom.h in the V-Ray SDK
#define I_VRAYGEOMETRY_V100 ( I_USERINTERFACE + 0x20112003 )

// From the V-Ray SDK: "GeomObject's that support native V-Ray geometry must
// support this interface in their GetInterface() method, and return a pointer
// to an instance of class VRenderObject".
#define I_VRAYGEOMETRY ( I_VRAYGEOMETRY_V100 )

using namespace boost::assign;

using std::string;
using std::vector;

using namespace frantic::channels;
using frantic::files::filename_sequence;
using frantic::geometry::trimesh3;
using frantic::geometry::trimesh3_face_channel_accessor;
using frantic::geometry::trimesh3_face_channel_general_accessor;
using frantic::geometry::trimesh3_vertex_channel_accessor;
using frantic::geometry::trimesh3_vertex_channel_general_accessor;
using frantic::graphics::color3f;
using frantic::graphics::transform4f;
using frantic::graphics::vector3f;
using frantic::math::radians_to_degrees;
using frantic::max3d::from_max_t;
using frantic::max3d::get_base_object;
using frantic::max3d::geometry::clear_mesh;
using frantic::max3d::geometry::mesh_copy;
using frantic::max3d::geometry::mesh_copy_time_offset;
using frantic::max3d::particles::IMaxKrakatoaPRTObject;
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
using frantic::max3d::particles::tp_interface;
#endif
using frantic::particles::streams::particle_istream;
using namespace frantic::particles;
using namespace frantic::particles::streams;
using namespace frantic::volumetrics::implicitsurface;
using namespace frantic::volumetrics;

Frost* GetFrost( Object* obj ) {
    if( obj->ClassID() == Frost_CLASS_ID )
        return reinterpret_cast<Frost*>( obj );
    else
        return NULL;
}

void BuildMesh_FrostLogoMesh( Mesh& outMesh );
boost::shared_ptr<Mesh> BuildIconMesh() {
    boost::shared_ptr<Mesh> result( new Mesh() );
    BuildMesh_FrostLogoMesh( *result );
    return result;
}
static boost::shared_ptr<Mesh> g_pIconMesh = BuildIconMesh();

void BuildMesh_FrostDemoLogoMesh( Mesh& outMesh );
boost::shared_ptr<Mesh> BuildDemoIconMesh() {
    boost::shared_ptr<Mesh> result( new Mesh() );
    BuildMesh_FrostLogoMesh( *result );
    return result;
}
static boost::shared_ptr<Mesh> g_pDemoIconMesh = BuildDemoIconMesh();

static frost_create_mouse_callback gCreateMouseCB;

IObjParam* Frost::ip = NULL;

std::vector<int> Frost::m_beforeRangeBehaviorDisplayCodes =
    list_of( FRAME_RANGE_CLAMP_MODE::HOLD )( FRAME_RANGE_CLAMP_MODE::BLANK );
std::map<int, frantic::tstring> Frost::m_beforeRangeBehaviorNames;

std::vector<int> Frost::m_afterRangeBehaviorDisplayCodes =
    list_of( FRAME_RANGE_CLAMP_MODE::HOLD )( FRAME_RANGE_CLAMP_MODE::BLANK );
std::map<int, frantic::tstring> Frost::m_afterRangeBehaviorNames;

std::vector<int> Frost::m_fileLengthUnitDisplayCodes =
    list_of( FILE_LENGTH_UNIT::GENERIC )( FILE_LENGTH_UNIT::INCHES )( FILE_LENGTH_UNIT::FEET )(
        FILE_LENGTH_UNIT::MILES )( FILE_LENGTH_UNIT::MILLIMETERS )( FILE_LENGTH_UNIT::CENTIMETERS )(
        FILE_LENGTH_UNIT::METERS )( FILE_LENGTH_UNIT::KILOMETERS )( FILE_LENGTH_UNIT::CUSTOM );
std::map<int, frantic::tstring> Frost::m_fileLengthUnitNames;

std::vector<int> Frost::m_materialModeDisplayCodes =
    list_of( MATERIAL_MODE::SINGLE )( MATERIAL_MODE::MTLINDEX_CHANNEL )( MATERIAL_MODE::SHAPE_NUMBER )(
        MATERIAL_MODE::MATERIAL_ID_FROM_GEOMETRY )( MATERIAL_MODE::MATERIAL_FROM_GEOMETRY );
std::map<int, frantic::tstring> Frost::m_materialModeNames;

std::vector<int> Frost::m_radiusAnimationModeCodes = list_of( RADIUS_ANIMATION_MODE::ABSOLUTE_TIME )(
    RADIUS_ANIMATION_MODE::AGE )( RADIUS_ANIMATION_MODE::LIFE_PERCENT );
std::map<int, frantic::tstring> Frost::m_radiusAnimationModeNames;

std::vector<int> Frost::m_meshingMethodDisplayCodes =
    list_of( MESHING_METHOD::GEOMETRY )( MESHING_METHOD::UNION_OF_SPHERES )( MESHING_METHOD::METABALLS )(
        MESHING_METHOD::ZHU_BRIDSON )( MESHING_METHOD::ANISOTROPIC )( MESHING_METHOD::VERTEX_CLOUD );
std::map<int, frantic::tstring> Frost::m_meshingMethodNames;

std::vector<int> Frost::m_viewportLoadModeCodes =
    list_of( VIEWPORT_LOAD_MODE::HEAD )( VIEWPORT_LOAD_MODE::STRIDE )( VIEWPORT_LOAD_MODE::ID );
std::map<int, frantic::tstring> Frost::m_viewportLoadModeNames;

std::vector<int> Frost::m_motionBlurDisplayCodes = list_of( MOTION_BLUR_MODE::FRAME_VELOCITY_OFFSET )(
    MOTION_BLUR_MODE::SUBFRAME_PARTICLE_OFFSET )( MOTION_BLUR_MODE::NEAREST_FRAME_VELOCITY_OFFSET );
std::map<int, frantic::tstring> Frost::m_motionBlurMethodNames;

std::vector<int> Frost::m_geometryTypeDisplayCodes = list_of( GEOMETRY_TYPE::PLANE )( GEOMETRY_TYPE::SPRITE )(
    GEOMETRY_TYPE::TETRAHEDRON )( GEOMETRY_TYPE::BOX )( GEOMETRY_TYPE::SPHERE20 )( GEOMETRY_TYPE::CUSTOM_GEOMETRY );
std::map<int, frantic::tstring> Frost::m_geometryTypeNames;

std::vector<int> Frost::m_geometrySelectionModeDisplayCodes = list_of( GEOMETRY_SELECTION_MODE::CYCLE )(
    GEOMETRY_SELECTION_MODE::RANDOM_BY_ID )( GEOMETRY_SELECTION_MODE::SHAPEINDEX_CHANNEL );
std::map<int, frantic::tstring> Frost::m_geometrySelectionModeNames;

std::vector<int> Frost::m_geometryTimingBaseModeDisplayCodes =
    list_of( GEOMETRY_TIMING_BASE_MODE::TIME_0 )( GEOMETRY_TIMING_BASE_MODE::CURRENT_TIME );
std::map<int, frantic::tstring> Frost::m_geometryTimingBaseModeNames;

std::vector<int> Frost::m_geometryTimingOffsetModeDisplayCodes = list_of( GEOMETRY_TIMING_OFFSET_MODE::NO_OFFSET )(
    GEOMETRY_TIMING_OFFSET_MODE::
        RANDOM_BY_ID ) /*(GEOMETRY_TIMING_OFFSET_MODE::ABSTIME_CHANNEL)(GEOMETRY_TIMING_OFFSET_MODE::TIMEOFFSET_CHANNEL)*/
    ( GEOMETRY_TIMING_OFFSET_MODE::GEOMTIME_CHANNEL );
std::map<int, frantic::tstring> Frost::m_geometryTimingOffsetModeNames;

std::vector<int> Frost::m_geometryOrientationModeDisplayCodes = list_of( GEOMETRY_ORIENTATION_MODE::LOOK_AT )(
    GEOMETRY_ORIENTATION_MODE::MATCH_OBJECT )( GEOMETRY_ORIENTATION_MODE::ORIENTATION_CHANNEL )(
    GEOMETRY_ORIENTATION_MODE::VECTOR_CHANNEL )( GEOMETRY_ORIENTATION_MODE::SPECIFY );
std::map<int, frantic::tstring> Frost::m_geometryOrientationModeNames;

std::vector<int> Frost::m_orientationDivergenceAxisSpaceDisplayCodes =
    list_of( AXIS_SPACE::WORLD )( AXIS_SPACE::LOCAL );
std::map<int, frantic::tstring> Frost::m_orientationDivergenceAxisSpaceNames;

Frost* Frost::editObj = 0;

IParamMap2* Frost::m_helpParamMap = 0;
bool Frost::m_rsHelp = true;
IParamMap2* Frost::m_mainParamMap = 0;
bool Frost::m_rsMain = false;
IParamMap2* Frost::m_presetsParamMap = 0;
bool Frost::m_rsPresets = true;
IParamMap2* Frost::m_particleObjectsParamMap = 0;
bool Frost::m_rsParticleObjects = false;
IParamMap2* Frost::m_particleFlowParamMap = 0;
bool Frost::m_rsParticleFlow = true;
IParamMap2* Frost::m_tpGroupsParamMap = 0;
bool Frost::m_rsTPGroups = true;
IParamMap2* Frost::m_particleFilesParamMap = 0;
bool Frost::m_rsParticleFiles = true;
IParamMap2* Frost::m_meshingParamMap = 0;
bool Frost::m_rsMeshing = false;
IParamMap2* Frost::m_materialParamMap = 0;
bool Frost::m_rsMaterial = true;
IParamMap2* Frost::m_roiParamMap = 0;
bool Frost::m_rsROI = true;

std::map<int, param_map_info> Frost::m_mesherParamMapInfo;
IParamMap2* Frost::m_mesherParamMap = 0;
bool Frost::m_rsMesher = false;

std::map<int, param_map_info> Frost::m_qualityParamMapInfo;
IParamMap2* Frost::m_qualityParamMap = 0;
bool Frost::m_rsQuality = false;

std::map<frost_status_code_t, frost_status_code_info> Frost::m_statusCodeInfo;

frantic::channels::channel_map Frost::m_defaultChannelMap;
frantic::channels::channel_map Frost::m_defaultKrakatoaChannelMap;
frantic::channels::channel_map Frost::m_emptyChannelMap;

frost_help_dlg_proc helpDlgProc;
frost_main_dlg_proc mainDlgProc;
presets_dlg_proc presetsDlgProc;
particle_objects_dlg_proc particleObjectsDlgProc;
particle_flow_events_dlg_proc particleFlowDlgProc;
tp_groups_dlg_proc tpGroupsDlgProc;
particle_files_dlg_proc particleFilesDlgProc;
meshing_dlg_proc meshingDlgProc;
geometry_dlg_proc geometryDlgProc;
metaballs_dlg_proc metaballsDlgProc;
zhu_bridson_dlg_proc zhuBridsonDlgProc;
anisotropic_dlg_proc anisotropicDlgProc;
material_dlg_proc materialDlgProc;
meshing_quality_dlg_proc meshingQualityDlgProc;
roi_dlg_proc roiDlgProc;

Frost::StaticInitializer::StaticInitializer() {
    m_mesherParamMapInfo.clear();
    m_mesherParamMapInfo[MESHING_METHOD::GEOMETRY] =
        param_map_info( frost_geometry_param_map, IDD_GEOMETRY, IDS_GEOMETRY_DIALOG_TITLE, &geometryDlgProc );
    m_mesherParamMapInfo[MESHING_METHOD::UNION_OF_SPHERES] = param_map_info( 0, 0, 0, 0 );
    m_mesherParamMapInfo[MESHING_METHOD::METABALLS] =
        param_map_info( frost_metaballs_param_map, IDD_METABALLS, IDS_METABALLS_DIALOG_TITLE, &metaballsDlgProc );
    m_mesherParamMapInfo[MESHING_METHOD::ZHU_BRIDSON] = param_map_info(
        frost_zhu_bridson_param_map, IDD_ZHU_BRIDSON, IDS_ZHU_BRIDSON_DIALOG_TITLE, &zhuBridsonDlgProc );
    m_mesherParamMapInfo[MESHING_METHOD::ANISOTROPIC] = param_map_info(
        frost_anisotropic_param_map, IDD_ANISOTROPIC, IDS_ANISOTROPIC_DIALOG_TITLE, &anisotropicDlgProc );

    m_qualityParamMapInfo.clear();
    m_qualityParamMapInfo[MESHING_METHOD::UNION_OF_SPHERES] =
        param_map_info( frost_meshing_quality_param_map, IDD_MESHING_QUALITY, IDS_MESHING_QUALITY_DIALOG_TITLE,
                        &meshingQualityDlgProc );
    m_qualityParamMapInfo[MESHING_METHOD::METABALLS] =
        param_map_info( frost_meshing_quality_param_map, IDD_MESHING_QUALITY, IDS_MESHING_QUALITY_DIALOG_TITLE,
                        &meshingQualityDlgProc );
    m_qualityParamMapInfo[MESHING_METHOD::ZHU_BRIDSON] =
        param_map_info( frost_meshing_quality_param_map, IDD_MESHING_QUALITY, IDS_MESHING_QUALITY_DIALOG_TITLE,
                        &meshingQualityDlgProc );
    m_qualityParamMapInfo[MESHING_METHOD::ANISOTROPIC] =
        param_map_info( frost_meshing_quality_param_map, IDD_MESHING_QUALITY, IDS_MESHING_QUALITY_DIALOG_TITLE,
                        &meshingQualityDlgProc );

    m_beforeRangeBehaviorNames.clear();
    m_beforeRangeBehaviorNames[FRAME_RANGE_CLAMP_MODE::HOLD] = FRAME_RANGE_CLAMP_MODE_HOLD_BEFORE;
    m_beforeRangeBehaviorNames[FRAME_RANGE_CLAMP_MODE::BLANK] = FRAME_RANGE_CLAMP_MODE_BLANK;

    m_afterRangeBehaviorNames.clear();
    m_afterRangeBehaviorNames[FRAME_RANGE_CLAMP_MODE::HOLD] = FRAME_RANGE_CLAMP_MODE_HOLD_AFTER;
    m_afterRangeBehaviorNames[FRAME_RANGE_CLAMP_MODE::BLANK] = FRAME_RANGE_CLAMP_MODE_BLANK;

    m_fileLengthUnitNames.clear();
    m_fileLengthUnitNames[FILE_LENGTH_UNIT::GENERIC] = FILE_LENGTH_UNIT_GENERIC;
    m_fileLengthUnitNames[FILE_LENGTH_UNIT::INCHES] = FILE_LENGTH_UNIT_INCHES;
    m_fileLengthUnitNames[FILE_LENGTH_UNIT::FEET] = FILE_LENGTH_UNIT_FEET;
    m_fileLengthUnitNames[FILE_LENGTH_UNIT::MILES] = FILE_LENGTH_UNIT_MILES;
    m_fileLengthUnitNames[FILE_LENGTH_UNIT::MILLIMETERS] = FILE_LENGTH_UNIT_MILLIMETERS;
    m_fileLengthUnitNames[FILE_LENGTH_UNIT::CENTIMETERS] = FILE_LENGTH_UNIT_CENTIMETERS;
    m_fileLengthUnitNames[FILE_LENGTH_UNIT::METERS] = FILE_LENGTH_UNIT_METERS;
    m_fileLengthUnitNames[FILE_LENGTH_UNIT::KILOMETERS] = FILE_LENGTH_UNIT_KILOMETERS;
    m_fileLengthUnitNames[FILE_LENGTH_UNIT::CUSTOM] = FILE_LENGTH_UNIT_CUSTOM;

    m_materialModeNames.clear();
    m_materialModeNames[MATERIAL_MODE::SINGLE] = MATERIAL_MODE_SINGLE;
    m_materialModeNames[MATERIAL_MODE::MTLINDEX_CHANNEL] = MATERIAL_MODE_MTLINDEX_CHANNEL;
    m_materialModeNames[MATERIAL_MODE::SHAPE_NUMBER] = MATERIAL_MODE_SHAPE_NUMBER;
    m_materialModeNames[MATERIAL_MODE::MATERIAL_ID_FROM_GEOMETRY] = MATERIAL_MODE_MATERIAL_ID_FROM_GEOMETRY;
    m_materialModeNames[MATERIAL_MODE::MATERIAL_FROM_GEOMETRY] = MATERIAL_MODE_MATERIAL_FROM_GEOMETRY;

    m_radiusAnimationModeNames.clear();
    m_radiusAnimationModeNames[RADIUS_ANIMATION_MODE::ABSOLUTE_TIME] = RADIUS_ANIMATION_MODE_ABSOLUTE_TIME;
    m_radiusAnimationModeNames[RADIUS_ANIMATION_MODE::AGE] = RADIUS_ANIMATION_MODE_AGE;
    m_radiusAnimationModeNames[RADIUS_ANIMATION_MODE::LIFE_PERCENT] = RADIUS_ANIMATION_MODE_LIFE_PERCENT;

    m_meshingMethodNames.clear();
    m_meshingMethodNames[MESHING_METHOD::GEOMETRY] = MESHING_METHOD_GEOMETRY;
    m_meshingMethodNames[MESHING_METHOD::UNION_OF_SPHERES] = MESHING_METHOD_UNION_OF_SPHERES;
    m_meshingMethodNames[MESHING_METHOD::METABALLS] = MESHING_METHOD_METABALLS;
    m_meshingMethodNames[MESHING_METHOD::ZHU_BRIDSON] = MESHING_METHOD_ZHU_BRIDSON;
    m_meshingMethodNames[MESHING_METHOD::ANISOTROPIC] = MESHING_METHOD_ANISOTROPIC;
    m_meshingMethodNames[MESHING_METHOD::VERTEX_CLOUD] = MESHING_METHOD_VERTEX_CLOUD;
    m_meshingMethodNames[MESHING_METHOD::TETRAHEDRON] = MESHING_METHOD_TETRAHEDRA;

    m_viewportLoadModeNames.clear();
    m_viewportLoadModeNames[VIEWPORT_LOAD_MODE::HEAD] = VIEWPORT_LOAD_MODE_HEAD;
    m_viewportLoadModeNames[VIEWPORT_LOAD_MODE::STRIDE] = VIEWPORT_LOAD_MODE_STRIDE;
    m_viewportLoadModeNames[VIEWPORT_LOAD_MODE::ID] = VIEWPORT_LOAD_MODE_ID;

    m_motionBlurMethodNames.clear();
    m_motionBlurMethodNames[MOTION_BLUR_MODE::FRAME_VELOCITY_OFFSET] = MOTION_BLUR_FRAME_VELOCITY_OFFSET;
    m_motionBlurMethodNames[MOTION_BLUR_MODE::SUBFRAME_PARTICLE_OFFSET] = MOTION_BLUR_SUBFRAME_POSITION;
    m_motionBlurMethodNames[MOTION_BLUR_MODE::NEAREST_FRAME_VELOCITY_OFFSET] =
        MOTION_BLUR_NEAREST_FRAME_VELOCITY_OFFSET;

    m_geometryTypeNames.clear();
    m_geometryTypeNames[GEOMETRY_TYPE::PLANE] = GEOMETRY_TYPE_PLANE;
    m_geometryTypeNames[GEOMETRY_TYPE::SPRITE] = GEOMETRY_TYPE_SPRITE;
    m_geometryTypeNames[GEOMETRY_TYPE::TETRAHEDRON] = GEOMETRY_TYPE_TETRAHEDRON;
    m_geometryTypeNames[GEOMETRY_TYPE::BOX] = GEOMETRY_TYPE_BOX;
    m_geometryTypeNames[GEOMETRY_TYPE::SPHERE20] = GEOMETRY_TYPE_SPHERE20;
    m_geometryTypeNames[GEOMETRY_TYPE::CUSTOM_GEOMETRY] = GEOMETRY_TYPE_CUSTOM;

    m_geometrySelectionModeNames.clear();
    m_geometrySelectionModeNames[GEOMETRY_SELECTION_MODE::CYCLE] = GEOMETRY_SELECTION_CYCLE;
    m_geometrySelectionModeNames[GEOMETRY_SELECTION_MODE::RANDOM_BY_ID] = GEOMETRY_SELECTION_RANDOM;
    m_geometrySelectionModeNames[GEOMETRY_SELECTION_MODE::SHAPEINDEX_CHANNEL] = GEOMETRY_SELECTION_SHAPEINDEX;

    m_geometryTimingBaseModeNames.clear();
    m_geometryTimingBaseModeNames[GEOMETRY_TIMING_BASE_MODE::TIME_0] = GEOMETRY_TIMING_BASE_TIME_0;
    m_geometryTimingBaseModeNames[GEOMETRY_TIMING_BASE_MODE::CURRENT_TIME] = GEOMETRY_TIMING_BASE_CURRENT_TIME;

    m_geometryTimingOffsetModeNames.clear();
    m_geometryTimingOffsetModeNames[GEOMETRY_TIMING_OFFSET_MODE::NO_OFFSET] = GEOMETRY_TIMING_OFFSET_NO_OFFSET;
    m_geometryTimingOffsetModeNames[GEOMETRY_TIMING_OFFSET_MODE::RANDOM_BY_ID] = GEOMETRY_TIMING_OFFSET_RANDOM;
    m_geometryTimingOffsetModeNames[GEOMETRY_TIMING_OFFSET_MODE::ABSTIME_CHANNEL] = GEOMETRY_TIMING_OFFSET_ABSTIME;
    m_geometryTimingOffsetModeNames[GEOMETRY_TIMING_OFFSET_MODE::TIMEOFFSET_CHANNEL] =
        GEOMETRY_TIMING_OFFSET_TIMEOFFSET;
    m_geometryTimingOffsetModeNames[GEOMETRY_TIMING_OFFSET_MODE::GEOMTIME_CHANNEL] = GEOMETRY_TIMING_OFFSET_GEOMTIME;

    m_geometryOrientationModeNames.clear();
    m_geometryOrientationModeNames[GEOMETRY_ORIENTATION_MODE::LOOK_AT] = GEOMETRY_ORIENTATION_LOOK_AT;
    m_geometryOrientationModeNames[GEOMETRY_ORIENTATION_MODE::MATCH_OBJECT] = GEOMETRY_ORIENTATION_MATCH_OBJECT;
    m_geometryOrientationModeNames[GEOMETRY_ORIENTATION_MODE::ORIENTATION_CHANNEL] =
        GEOMETRY_ORIENTATION_ORIENTATION_CHANNEL;
    m_geometryOrientationModeNames[GEOMETRY_ORIENTATION_MODE::VECTOR_CHANNEL] = GEOMETRY_ORIENTATION_VECTOR_CHANNEL;
    m_geometryOrientationModeNames[GEOMETRY_ORIENTATION_MODE::SPECIFY] = GEOMETRY_ORIENTATION_SPECIFY;

    m_orientationDivergenceAxisSpaceNames.clear();
    m_orientationDivergenceAxisSpaceNames[AXIS_SPACE::WORLD] = AXIS_SPACE_WORLD;
    m_orientationDivergenceAxisSpaceNames[AXIS_SPACE::LOCAL] = AXIS_SPACE_LOCAL;

    m_statusCodeInfo.clear();
    m_statusCodeInfo[frost_status_code::access_failed] = frost_status_code_info( _T("Access Failed."), 2, 2 );
    m_statusCodeInfo[frost_status_code::no_particles_found] = frost_status_code_info( _T("No Particles Found."), 1, 0 );
    m_statusCodeInfo[frost_status_code::ok] = frost_status_code_info( _T("Ok."), 0, 0 );
    m_statusCodeInfo[frost_status_code::warning] = frost_status_code_info( _T("Warning."), 1, 1 );
    m_statusCodeInfo[frost_status_code::error] = frost_status_code_info( _T("Error."), 2, 2 );

    m_statusCodeInfo[frost_status_code::no_radius_channel] = frost_status_code_info( _T("No Radius Channel."), 2, 2 );
    m_statusCodeInfo[frost_status_code::radius_channel_ok] =
        frost_status_code_info( _T("Radius Channel Found."), 0, 0 );
    m_statusCodeInfo[frost_status_code::partial_radius_channel] =
        frost_status_code_info( _T("Partial Radius Channel."), 1, 1 );

    m_statusCodeInfo[frost_status_code::no_id_channel] = frost_status_code_info( _T("No ID; Using Index."), 1, 1 );
    m_statusCodeInfo[frost_status_code::id_channel_ok] = frost_status_code_info( _T("ID Channel Found."), 0, 0 );
    m_statusCodeInfo[frost_status_code::partial_id_channel] = frost_status_code_info( _T("Partial ID Channel."), 1, 1 );

    m_statusCodeInfo[frost_status_code::no_age_channel] = frost_status_code_info( _T("No Age; Using 0."), 1, 1 );
    m_statusCodeInfo[frost_status_code::partial_age_channel] =
        frost_status_code_info( _T("Partial Age Channel."), 1, 1 );
    m_statusCodeInfo[frost_status_code::no_lifespan_channel] =
        frost_status_code_info( _T("No LifeSpan; Using 1s."), 1, 1 );
    m_statusCodeInfo[frost_status_code::partial_lifespan_channel] =
        frost_status_code_info( _T("Partial LifeSpan Channel."), 1, 1 );

    m_statusCodeInfo[frost_status_code::no_mtlindex_channel] =
        frost_status_code_info( _T("No MtlIndex; Using 1."), 1, 1 );
    m_statusCodeInfo[frost_status_code::mtlindex_channel_ok] =
        frost_status_code_info( _T("MtlIndex Channel Found."), 0, 0 );
    m_statusCodeInfo[frost_status_code::partial_mtlindex_channel] =
        frost_status_code_info( _T("Partial MtlIndex Channel."), 1, 1 );
    m_statusCodeInfo[frost_status_code::unsupported_material_mode] =
        frost_status_code_info( _T("Unsupported; Using 1."), 1, 1 );

    m_defaultChannelMap.define_channel<frantic::graphics::vector3f>( _T("Position") );
    m_defaultChannelMap.end_channel_definition();

    // add Velocity to the default channel map for Krakatoa sources
    // this is required to get the correct Velocity from a PRT Loader with a playback graph
    m_defaultKrakatoaChannelMap.define_channel<frantic::graphics::vector3f>( _T("Position") );
    m_defaultKrakatoaChannelMap.define_channel<frantic::graphics::vector3f>( _T("Velocity") );
    m_defaultKrakatoaChannelMap.end_channel_definition();

    m_emptyChannelMap.end_channel_definition();
}

Frost::StaticInitializer Frost::m_staticInitializer;

FrostVRayLibrary::ptr_type Frost::m_frostVRayLibrary;

static void notify_render_preeval( void* param, NotifyInfo* info ) {
    Frost* pFrost = (Frost*)param;
    TimeValue* pTime = (TimeValue*)info->callParam;
    if( pFrost && pTime ) {
        // mprintf( "preeval: %g\n", ( * pTime ) / (float)GetTicksPerFrame() );
        // std::ofstream out( "c:/temp/frost.log", std::ios::out | std::ios::app );
        // out << "preeval: " << boost::lexical_cast<std::string>( (*pTime) / (float)GetTicksPerFrame() ) << std::endl;
        pFrost->SetRenderTime( *pTime );
        pFrost->set_empty_validity_and_notify_dependents();
    }
}

static void notify_post_renderframe( void* param, NotifyInfo* info ) {
    Frost* pFrost = (Frost*)param;
    RenderGlobalContext* pContext = (RenderGlobalContext*)info->callParam;
    if( pFrost && pContext ) {
        // mprintf( "post_renderframe: %g\n", ( pContext->time ) / (float)GetTicksPerFrame() );
        pFrost->ClearRenderTime();
    }
}

static void notify_manipulate_mode_changed( void* param, NotifyInfo* /*info*/ ) {
    Frost* pFrost = (Frost*)param;
    if( pFrost && pFrost->editObj == pFrost ) {
        roiDlgProc.UpdateManipulateMode();
    }
}

#pragma warning( push, 3 )
#pragma warning( disable : 4355 ) // warning C4355: 'this' : used in base member initializer list
                                  // TODO : I should probably change this rather than hide the warnings

Frost::Frost( bool loading, bool allowPresets )
    : m_inRenderingMode( false )
    , m_inRenderingInstanceMode( false )
    , TIMESTEP( GetTicksPerFrame() / 4 )
    , m_renderTime( TIME_NegInfinity )
    ,

    m_cachedMeshTime( TIME_NegInfinity )
    , m_cachedMeshInRenderingMode( false )
    , m_cachedMeshIsRenderQuality( false )
    ,

    m_displayFaceLimit( std::numeric_limits<int>::max() )
    ,

    m_statusIndicatorTime( TIME_NegInfinity )
    ,

    m_isFrostDirty( true )
    , m_isParticlesDirty( true )
    , m_isCustomGeometryDirty( true )
    , m_isTargetPosDirty( true )
    , m_isTargetRotDirty( true )
    ,

    m_isVectorChannelNameCacheDirty( true )
    ,

    m_cachedNativeChannelMapsTime( TIME_NegInfinity )
    ,

    m_isLoadingPreset( false )
    ,

    m_isSourceParticlesDirty( true )
    ,

    m_buildTrimesh3Count( 0 )
    ,

    m_thisNode( 0 )
    ,

    m_showIcon( this, pb_showIcon )
    , m_iconSize( this, pb_iconSize )
    , m_iconMode( this, pb_iconMode )
    ,

    m_updateOnFrostChange( this, pb_updateOnFrostChange )
    , m_updateOnParticleChange( this, pb_updateOnParticleChange )
    ,

    m_enableViewportMesh( this, pb_enableViewportMesh )
    , m_enableRenderMesh( this, pb_enableRenderMesh )
    ,

    m_nodeList( this, pb_nodeList )
    ,

    m_fileList( this, pb_fileList )
    , m_loadSingleFrame( this, pb_loadSingleFrame )
    , m_frameOffset( this, pb_frameOffset )
    , m_limitToRange( this, pb_limitToRange )
    , m_rangeFirstFrame( this, pb_rangeFirstFrame )
    , m_rangeLastFrame( this, pb_rangeLastFrame )
    , m_enablePlaybackGraph( this, pb_enablePlaybackGraph )
    , m_playbackGraphTime( this, pb_playbackGraphTime )
    , m_beforeRangeBehavior( this, pb_beforeRangeBehavior )
    , m_afterRangeBehavior( this, pb_afterRangeBehavior )
    , m_fileLengthUnit( this, pb_fileLengthUnit )
    , m_fileCustomScale( this, pb_fileCustomScale )
    ,

    m_pfEventFilterMode( this, pb_pfEventFilterMode )
    , m_pfEventList( this, pb_pfEventList )
    ,

    m_tpGroupFilterMode( this, pb_tpGroupFilterMode )
    , m_tpGroupList( this, pb_tpGroupList )
    ,

    m_meshingMethod( this, pb_meshingMethod )
    , m_viewRenderParticles( this, pb_viewRenderParticles )
    , m_radius( this, pb_radius )
    , m_useRadiusChannel( this, pb_useRadiusChannel )
    , m_randomizeRadius( this, pb_randomizeRadius )
    , m_radiusRandomVariation( this, pb_radiusRandomVariation )
    , m_radiusRandomSeed( this, pb_radiusSeed )
    , m_enableRadiusScale( this, pb_enableRadiusScale )
    , m_radiusScale( this, pb_radiusScale )
    , m_radiusAnimationMode( this, pb_radiusAnimationMode )
    , m_motionBlurMode( this, pb_motionBlurMode )
    ,

    m_renderUsingViewportSettings( this, pb_renderUsingViewportSettings )
    , m_renderMeshingResolution( this, pb_renderMeshingResolution )
    , m_renderVertRefinementIterations( this, pb_renderVertRefinementIterations )
    , m_previewAsGeometry( false )
    , m_viewportMeshingResolution( this, pb_viewportMeshingResolution )
    , m_viewportVertRefinementIterations( this, pb_viewportVertRefinementIterations )
    , m_meshingResolutionMode( this, pb_meshingResolutionMode )
    , m_renderVoxelLength( this, pb_renderVoxelLength )
    , m_viewportVoxelLength( this, pb_viewportVoxelLength )
    ,

    m_metaballRadiusScale( this, pb_metaballRadiusScale )
    , m_metaballIsosurfaceLevel( this, pb_metaballIsosurfaceLevel )
    ,

    m_zhuBridsonBlendRadiusScale( this, pb_zhuBridsonBlendRadiusScale )
    , m_zhuBridsonEnableLowDensityTrimming( this, pb_zhuBridsonEnableLowDensityTrimming )
    , m_zhuBridsonLowDensityTrimmingThreshold( this, pb_zhuBridsonLowDensityTrimmingThreshold )
    , m_zhuBridsonLowDensityTrimmingStrength( this, pb_zhuBridsonLowDensityTrimmingStrength )
    ,

    m_geometryType( this, pb_geometryType )
    , m_geometryList( this, pb_geometryList )
    , m_geometrySelectionMode( this, pb_geometrySelectionMode )
    , m_geometrySelectionSeed( this, pb_geometrySelectionSeed )
    ,

    m_geometrySampleTimeBaseMode( this, pb_geometrySampleTimeBaseMode )
    , m_geometrySampleTimeOffsetMode( this, pb_geometrySampleTimeOffsetMode )
    , m_geometrySampleTimeMaxRandomOffset( this, pb_geometrySampleTimeMaxRandomOffset )
    , m_geometrySampleTimeSeed( this, pb_geometrySampleTimeSeed )
    ,

    m_geometryOrientationMode( this, pb_geometryOrientationMode )
    , m_geometryOrientationX( this, pb_geometryOrientationX )
    , m_geometryOrientationY( this, pb_geometryOrientationY )
    , m_geometryOrientationZ( this, pb_geometryOrientationZ )
    , m_geometryOrientationLookAtNode( this, pb_geometryOrientationLookAtNode )
    , m_geometryOrientationVectorChannel( this, pb_geometryOrientationVectorChannel )
    , m_geometryOrientationDivergence( this, pb_geometryOrientationDivergence )
    ,

    m_geometryOrientationRestrictDivergenceAxis( this, pb_geometryOrientationRestrictDivergenceAxis )
    , m_geometryOrientationDivergenceAxisSpace( this, pb_geometryOrientationDivergenceAxisSpace )
    , m_geometryOrientationDivergenceAxisX( this, pb_geometryOrientationDivergenceAxisX )
    , m_geometryOrientationDivergenceAxisY( this, pb_geometryOrientationDivergenceAxisY )
    , m_geometryOrientationDivergenceAxisZ( this, pb_geometryOrientationDivergenceAxisZ )
    ,

    m_useRenderInstancing( this, pb_useRenderInstancing )
    ,

    m_viewportLoadMode( this, pb_viewportLoadMode )
    , m_viewportLoadPercent( this, pb_viewportLoadPercent )
    ,

    m_writeVelocityMapChannel( this, pb_writeVelocityMapChannel )
    , m_velocityMapChannel( this, pb_velocityMapChannel )
    ,

    m_anisotropicRadiusScale( this, pb_anisotropicRadiusScale )
    , m_anisotropicWindowScale( 2.f /*, pb_anisotropicWindowScale*/ )
    , m_anisotropicIsosurfaceLevel( this, pb_anisotropicIsosurfaceLevel )
    , m_anisotropicMaxAnisotropy( this, pb_anisotropicMaxAnisotropy )
    , m_anisotropicMinNeighborCount( this, pb_anisotropicMinNeighborCount )
    , m_anisotropicEnablePositionSmoothing( true /*this, pb_anisotropicEnablePositionSmoothing*/ )
    , m_anisotropicPositionSmoothingWindowScale( 2.f /*this, pb_anisotropicPositionSmoothingWindowScale*/ )
    , m_anisotropicPositionSmoothingWeight( this, pb_anisotropicPositionSmoothingWeight )
    ,

    m_materialMode( this, pb_materialMode )
    , m_undefinedMaterialID( this, pb_undefinedMaterialId )
    , m_geometryMaterialIDNodeList( this, pb_geometryMaterialIDNodeList )
    , m_geometryMaterialIDInList( this, pb_geometryMaterialIDInList )
    , m_geometryMaterialIDOutList( this, pb_geometryMaterialIDOutList )
    ,

    m_enableRenderROI( this, pb_enableRenderROI )
    , m_enableViewportROI( this, pb_enableViewportROI )
    , m_roiCenterX( this, pb_roiCenterX )
    , m_roiCenterY( this, pb_roiCenterY )
    , m_roiCenterZ( this, pb_roiCenterZ )
    , m_roiSizeX( this, pb_roiSizeX )
    , m_roiSizeY( this, pb_roiSizeY )
    , m_roiSizeZ( this, pb_roiSizeZ ) {
    InitializeFPDescriptor();
    GetFrostClassDesc()->MakeAutoParamBlocks( this );

    ClearRenderTime();

    m_presetName = _T("");
    if( !loading && allowPresets ) {
        try {
            bool usePresets =
                frantic::max3d::mxs::expression(
                    _T("if (FrostUi!=undefined) then ( try( FrostUi.getUsePresets() )catch( false ) ) else false") )
                    .evaluate<bool>();
            if( usePresets ) {
                frantic::tstring presetName =
                    frantic::max3d::mxs::expression(
                        _T("if (FrostUi!=undefined) then ( try( FrostUi.getPresetName() )catch( \"\" ) ) else \"\"") )
                        .evaluate<frantic::tstring>();
                const frantic::tstring presetFilename = get_preset_filename( presetName );
                if( boost::filesystem::exists( boost::filesystem::path( presetFilename ) ) ) {
                    load_preset_from_filename( presetFilename );
                    m_presetName = presetName;
                }
            }
        } catch( const std::exception& e ) {
            frantic::tstring errmsg = _T("Frost::Frost: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            report_error( errmsg );
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
            if( is_network_render_server() ) {
                throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
            }
        }
    }

    RegisterNotification( notify_render_preeval, (void*)this, NOTIFY_RENDER_PREEVAL );
    RegisterNotification( notify_post_renderframe, (void*)this, NOTIFY_POST_RENDERFRAME );
    RegisterNotification( notify_manipulate_mode_changed, (void*)this, NOTIFY_MANIPULATE_MODE_OFF );
    RegisterNotification( notify_manipulate_mode_changed, (void*)this, NOTIFY_MANIPULATE_MODE_ON );
}

#pragma warning( pop )

Frost::~Frost() {
    UnRegisterNotification( notify_manipulate_mode_changed, (void*)this, NOTIFY_MANIPULATE_MODE_ON );
    UnRegisterNotification( notify_manipulate_mode_changed, (void*)this, NOTIFY_MANIPULATE_MODE_OFF );
    UnRegisterNotification( notify_post_renderframe, (void*)this, NOTIFY_POST_RENDERFRAME );
    UnRegisterNotification( notify_render_preeval, (void*)this, NOTIFY_RENDER_PREEVAL );
}

// To support the Frantic Function Publishing interface
void Frost::InitializeFPDescriptor() {
    FFCreateDescriptor c( this, Frost_INTERFACE_ID, Frost_INTERFACE_NAME, GetFrostClassDesc() );

    c.add_function( &Frost::GetMaxParticleRadius, _T("GetMaxParticleRadius") );

    c.add_function( &Frost::GetNodeListSelection, _T("GetNodeListSelection") );
    c.add_function( &Frost::SetNodeListSelection, _T("SetNodeListSelection") );

    // c.add_function( & Frost::GetPFEventListSelection, _T("GetPFEventListSelection") );
    // c.add_function( & Frost::SetPFEventListSelection, _T("SetPFEventListSelection") );
    // c.add_function( & Frost::RefreshPFEventList, _T("RefreshPFEventList") );
    // c.add_function( & Frost::RefreshTPGroupList, _T("RefreshTPGroupList") );

    c.add_function( &Frost::GetFileListSelection, _T("GetFileListSelection") );
    c.add_function( &Frost::SetFileListSelection, _T("SetFileListSelection") );

    c.add_function( &Frost::GetGeometryListSelection, _T("GetGeometryListSelection") );
    c.add_function( &Frost::SetGeometryListSelection, _T("SetGeometryListSelection") );

#ifdef ENABLE_FROST_DISK_CACHE
    c.add_function( &Frost::SetCacheName, _T("SetCacheName") );
    c.add_function( &Frost::StripSequenceNumber, _T("StripSequenceNumber") );
    c.add_function( &Frost::SaveMeshToSequence, _T("SaveMeshToSequence") );
    c.add_function( &Frost::SaveLevelSetToSequence, _T("SaveLevelSetToSequence") );
    c.add_function( &Frost::UpdateSourceSequences, _T("UpdateSourceSequences") );
    c.add_function( &Frost::GetTempFileNameWithPrefix, _T("GetTempFileNameWithPrefix") );
#endif
}

// From BaseObject
void Frost::BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev ) {
    this->editObj = this;
    this->ip = ip;

    SimpleObject2::BeginEditParams( ip, flags, prev );
    // GetFrostClassDesc()->BeginEditParams( ip, this, flags, prev );

    mainDlgProc.SetFrost( this );
    helpDlgProc.SetFrost( this );
    presetsDlgProc.SetFrost( this );
    particleObjectsDlgProc.SetFrost( this );
    particleFlowDlgProc.SetFrost( this );
    tpGroupsDlgProc.SetFrost( this );
    particleFilesDlgProc.SetFrost( this );
    materialDlgProc.SetFrost( this );
    meshingDlgProc.SetFrost( this );
    roiDlgProc.SetFrost( this );

    geometryDlgProc.SetFrost( this );
    meshingQualityDlgProc.SetFrost( this );
    metaballsDlgProc.SetFrost( this );
    zhuBridsonDlgProc.SetFrost( this );
    anisotropicDlgProc.SetFrost( this );

    GetNodeListValidator()->SetFrost( this );
    GetGeometryListValidator()->SetFrost( this );
    GetOrientationLookAtValidator()->SetFrost( this );

    m_mainParamMap =
        CreateCPParamMap2( frost_main_param_map, pblock2, ip, ghInstance, MAKEINTRESOURCE( IDD_FROST ),
                           GetString( IDS_FROST_DIALOG_TITLE ), m_rsMain ? APPENDROLL_CLOSED : 0, &mainDlgProc );
    m_helpParamMap =
        CreateCPParamMap2( frost_help_param_map, pblock2, ip, ghInstance, MAKEINTRESOURCE( IDD_HELP ),
                           GetString( IDS_HELP_DIALOG_TITLE ), m_rsHelp ? APPENDROLL_CLOSED : 0, &helpDlgProc );

    m_presetsParamMap = CreateCPParamMap2( frost_presets_param_map, pblock2, ip, ghInstance,
                                           MAKEINTRESOURCE( IDD_PRESETS ), GetString( IDS_PRESETS_DIALOG_TITLE ),
                                           m_rsPresets ? APPENDROLL_CLOSED : 0, &presetsDlgProc );
    m_particleObjectsParamMap =
        CreateCPParamMap2( frost_particle_objects_param_map, pblock2, ip, ghInstance,
                           MAKEINTRESOURCE( IDD_PARTICLE_OBJECTS ), GetString( IDS_PARTICLE_OBJECTS_DIALOG_TITLE ),
                           m_rsParticleObjects ? APPENDROLL_CLOSED : 0, &particleObjectsDlgProc );
    m_particleFlowParamMap = CreateCPParamMap2(
        frost_particle_flow_events_param_map, pblock2, ip, ghInstance, MAKEINTRESOURCE( IDD_PARTICLE_GROUPS ),
        GetString( IDS_PARTICLE_FLOW_DIALOG_TITLE ), m_rsParticleFlow ? APPENDROLL_CLOSED : 0, &particleFlowDlgProc );
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    if( tp_interface::get_instance().is_available() ) {
        m_tpGroupsParamMap = CreateCPParamMap2( frost_tp_groups_param_map, pblock2, ip, ghInstance,
                                                MAKEINTRESOURCE( IDD_PARTICLE_GROUPS ),
                                                GetString( IDS_THINKING_PARTICLES_GROUPS_DIALOG_TITLE ),
                                                m_rsTPGroups ? APPENDROLL_CLOSED : 0, &tpGroupsDlgProc );
    }
#endif
    m_particleFilesParamMap =
        CreateCPParamMap2( frost_particle_files_param_map, pblock2, ip, ghInstance,
                           MAKEINTRESOURCE( IDD_PARTICLE_FILES ), GetString( IDS_PARTICLE_FILES_DIALOG_TITLE ),
                           m_rsParticleFiles ? APPENDROLL_CLOSED : 0, &particleFilesDlgProc );
    m_roiParamMap = CreateCPParamMap2( frost_roi_param_map, pblock2, ip, ghInstance, MAKEINTRESOURCE( IDD_ROI ),
                                       GetString( IDS_ROI_TITLE ), m_rsROI ? APPENDROLL_CLOSED : 0, &roiDlgProc );
    m_materialParamMap = CreateCPParamMap2( frost_material_param_map, pblock2, ip, ghInstance,
                                            MAKEINTRESOURCE( IDD_MATERIAL ), GetString( IDS_MATERIAL_DIALOG_TITLE ),
                                            m_rsMaterial ? APPENDROLL_CLOSED : 0, &materialDlgProc );
    m_meshingParamMap = CreateCPParamMap2( frost_meshing_param_map, pblock2, ip, ghInstance,
                                           MAKEINTRESOURCE( IDD_MESHING ), GetString( IDS_MESHING_DIALOG_TITLE ),
                                           m_rsMeshing ? APPENDROLL_CLOSED : 0, &meshingDlgProc );

    param_map_info qualityPMI;
    get_quality_param_map_info( qualityPMI );
    if( qualityPMI.dlgProc ) {
        m_qualityParamMap =
            CreateCPParamMap2( qualityPMI.paramMap, pblock2, ip, ghInstance, MAKEINTRESOURCE( qualityPMI.dialogId ),
                               GetString( qualityPMI.dialogStringId ), m_rsQuality ? APPENDROLL_CLOSED : 0,
                               qualityPMI.dlgProc, 0 /*, ROLLOUT_CAT_CUSTOMATTRIB*/ );
    } else {
        m_qualityParamMap = 0;
    }
    param_map_info mesherPMI;
    get_mesher_param_map_info( mesherPMI );
    if( mesherPMI.dlgProc ) {
        m_mesherParamMap =
            CreateCPParamMap2( mesherPMI.paramMap, pblock2, ip, ghInstance, MAKEINTRESOURCE( mesherPMI.dialogId ),
                               GetString( mesherPMI.dialogStringId ), m_rsMesher ? APPENDROLL_CLOSED : 0,
                               mesherPMI.dlgProc, 0 /*, ROLLOUT_CAT_CUSTOMATTRIB*/ );
    } else {
        m_mesherParamMap = 0;
    }
}

void Frost::EndEditParams( IObjParam* ip, ULONG flags, Animatable* next ) {
    SimpleObject2::EndEditParams( ip, flags, next );
    // GetFrostClassDesc()->EndEditParams( ip, this, flags, next );

    if( m_mainParamMap ) {
        m_rsMain = IsRollupPanelOpen( m_mainParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_mainParamMap );
        m_mainParamMap = 0;
    }
    if( m_helpParamMap ) {
        m_rsHelp = IsRollupPanelOpen( m_helpParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_helpParamMap );
        m_helpParamMap = 0;
    }
    if( m_presetsParamMap ) {
        m_rsPresets = IsRollupPanelOpen( m_presetsParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_presetsParamMap );
        m_presetsParamMap = 0;
    }
    if( m_particleObjectsParamMap ) {
        m_rsParticleObjects = IsRollupPanelOpen( m_particleObjectsParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_particleObjectsParamMap );
        m_particleObjectsParamMap = 0;
    }
    if( m_particleFilesParamMap ) {
        m_rsParticleFiles = IsRollupPanelOpen( m_particleFilesParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_particleFilesParamMap );
        m_particleFilesParamMap = 0;
    }
    if( m_particleFlowParamMap ) {
        m_rsParticleFlow = IsRollupPanelOpen( m_particleFlowParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_particleFlowParamMap );
        m_particleFlowParamMap = 0;
    }
    if( m_tpGroupsParamMap ) {
        m_rsTPGroups = IsRollupPanelOpen( m_tpGroupsParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_tpGroupsParamMap );
        m_tpGroupsParamMap = 0;
    }
    if( m_materialParamMap ) {
        m_rsMaterial = IsRollupPanelOpen( m_materialParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_materialParamMap );
        m_materialParamMap = 0;
    }
    if( m_meshingParamMap ) {
        m_rsMeshing = IsRollupPanelOpen( m_meshingParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_meshingParamMap );
        m_meshingParamMap = 0;
    }
    if( m_qualityParamMap ) {
        m_rsQuality = IsRollupPanelOpen( m_qualityParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_qualityParamMap );
        m_qualityParamMap = 0;
    }
    if( m_mesherParamMap ) {
        m_rsMesher = IsRollupPanelOpen( m_mesherParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_mesherParamMap );
        m_mesherParamMap = 0;
    }

    if( m_roiParamMap ) {
        m_rsROI = IsRollupPanelOpen( m_roiParamMap->GetHWnd() ) ? false : true;
        DestroyCPParamMap2( m_roiParamMap );
        m_roiParamMap = 0;
    }

    this->ip = 0;
    this->editObj = 0;
}

int hit_test_box( const Box3& box, GraphicsWindow* gw, HitRegion& hitRegion, int /*abortOnHit*/ ) {
    if( gw == 0 ) {
        return 0;
    }
    if( box.IsEmpty() ) {
        return 0;
    }

    bool hitAll = true;
    bool hitAny = false;
    DWORD distance = std::numeric_limits<DWORD>::max();

    const Point3 faces[6][4] = { { box[0], box[1], box[3], box[2] }, { box[0], box[1], box[5], box[4] },
                                 { box[0], box[2], box[6], box[4] }, { box[1], box[3], box[7], box[5] },
                                 { box[2], box[3], box[7], box[6] }, { box[4], box[5], box[7], box[6] } };

    gw->clearHitCode();

    BOOST_FOREACH( const Point3* face, faces ) {
        gw->clearHitCode();
        gw->polyline( 4, const_cast<Point3*>( face ), NULL, NULL, 1, NULL );
        if( gw->checkHitCode() ) {
            hitAny = true;
            distance = std::min<DWORD>( distance, gw->getHitDistance() );
        } else {
            hitAll = false;
        }
    }

    const bool requireAllInside = ( hitRegion.type != POINT_RGN ) && ( hitRegion.crossing == 0 );
    if( requireAllInside ) {
        gw->setHitCode( hitAll );
        gw->setHitDistance( distance );
    } else {
        gw->setHitCode( hitAny );
        gw->setHitDistance( distance );
    }

    return gw->checkHitCode();
}

int Frost::HitTest( TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2* p, ViewExp* vpt ) {
    try {
        GraphicsWindow* gw = vpt->getGW();

        if( ( ( flags & HIT_SELONLY ) && !inode->Selected() ) || gw == NULL || inode == NULL ) {
            return 0;
        }

        if( gw->getRndMode() & GW_BOX_MODE ) {
            const DWORD rndLimits = gw->getRndLimits();
            gw->setRndLimits( ( rndLimits | GW_PICK ) & ~GW_ILLUM );

            Matrix3 tm = inode->GetNodeTM( t );
            gw->setTransform( tm );

            HitRegion hitRegion;
            MakeHitRegion( hitRegion, type, crossing, 4, p );
            gw->setHitRegion( &hitRegion );
            gw->clearHitCode();

            Box3 box;
            GetLocalBoundBox( t, inode, vpt, box );
            const int hit = hit_test_box( box, gw, hitRegion, flags & HIT_ABORTONHIT );
            gw->setRndLimits( rndLimits );
            return hit;
        }

        // Let the SimpleObject2 hit test code do its thing
        if( SimpleObject2::HitTest( t, inode, type, crossing, flags, p, vpt ) ) {
            return 1;
        }

        HitRegion hitRegion;
        MakeHitRegion( hitRegion, type, crossing, 4, p );

        Matrix3 tm = inode->GetNodeTM( t );
        gw->setTransform( tm );

        gw->setHitRegion( &hitRegion );
        gw->clearHitCode();

        // Hit test against the ROI box
        if( inode->Selected() ) {
            boost::optional<boundbox3f> roiBox = get_display_roi_box( t );
            if( roiBox ) {
                const DWORD rndLimits = gw->getRndLimits();
                gw->setRndLimits( ( rndLimits | GW_PICK ) & ~GW_ILLUM );

                Matrix3 worldTM( TRUE );
                gw->setTransform( worldTM );

                const int hit =
                    hit_test_box( frantic::max3d::to_max_t( *roiBox ), gw, hitRegion, flags & HIT_ABORTONHIT );

                gw->setRndLimits( rndLimits );

                if( hit ) {
                    return hit;
                }
            }
        }

        // Hit test against the icon if necessary
        Mesh* iconMesh = get_icon_mesh( t );
        if( iconMesh ) {
            const DWORD rndLimits = gw->getRndLimits();

            if( !use_filled_icon( t ) ) {
                gw->setRndLimits( rndLimits & ~( DWORD( GW_ILLUM | GW_FLAT | GW_TEXTURE | GW_CONSTANT ) ) |
                                  GW_WIREFRAME );
            }

            FrostIconMaterial frostIconMaterial;

            float f = m_iconSize.at_time( t );
            tm.Scale( Point3( f, f, f ) );
            gw->setTransform( tm );

            if( iconMesh->select( gw, &frostIconMaterial, &hitRegion, flags & HIT_ABORTONHIT ) ) {
                gw->setRndLimits( rndLimits );
                return true;
            }

            gw->setRndLimits( rndLimits );
        }

        return false;
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("HitTest: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return false;
}

DWORD get_icon_mesh_rnd_limits( DWORD rndLimits ) {
#if MAX_VERSION_MAJOR >= 14
    if( MaxSDK::Graphics::IsRetainedModeEnabled() ) {
        return rndLimits & ~( DWORD( GW_VERT_TICKS | GW_ILLUM | GW_TEXTURE ) );
    }
#endif
    return rndLimits & ~( DWORD( GW_VERT_TICKS ) );
}

int Frost::Display( TimeValue t, INode* inode, ViewExp* pView, int flags ) {
    try {
        if( !inode || !pView ) {
            return 0;
        }

        if( inode->IsNodeHidden() ) {
            return TRUE;
        }

        BuildMesh( t );

        SimpleObject2::Display( t, inode, pView, flags );

        // draw ROI Box
        if( inode->Selected() ) {
            boost::optional<boundbox3f> roiBox = get_display_roi_box( t );
            if( roiBox ) {
                GraphicsWindow* gw = pView->getGW();

                Matrix3 worldTM( TRUE );
                gw->setTransform( worldTM );

                color3f lineColor = color3f::from_RGBA( inode->GetWireColor() );
                if( inode->Selected() )
                    lineColor = color3f::white();

                gw->setColor( LINE_COLOR, lineColor.r, lineColor.g, lineColor.b );
                frantic::max3d::DrawBox( gw, frantic::max3d::to_max_t( *roiBox ) );
            }
        }

        // draw icon
        Mesh* iconMesh = get_icon_mesh( t );
        if( iconMesh ) {
            GraphicsWindow* gw = pView->getGW();
            const DWORD rndLimits = gw->getRndLimits();

            gw->setTransform( inode->GetNodeTM( t ) );

            Matrix3 iconMatrix = inode->GetNodeTM( t );
            const float f = m_iconSize.at_time( t );
            iconMatrix.Scale( Point3( f, f, f ) );
            gw->setTransform( iconMatrix );

            const color3f fillColor = color3f::from_RGBA( inode->GetWireColor() );
            const color3f lineColor = inode->Selected() ? color3f( &from_max_t( GetSelColor() )[0] ) : fillColor;

            if( use_filled_icon( t ) ) {
                gw->setRndLimits( get_icon_mesh_rnd_limits( rndLimits ) );

                FrostIconMaterial iconMaterial( fillColor );
                gw->setMaterial( iconMaterial );
                gw->setColor( LINE_COLOR, lineColor.r, lineColor.g, lineColor.b );
                gw->setColor( FILL_COLOR, fillColor.r, fillColor.g, fillColor.b );

                Rect damageRect = pView->GetDammageRect();
                iconMesh->render( gw, &iconMaterial, ( flags & USE_DAMAGE_RECT ) ? &damageRect : NULL, COMP_ALL );
            } else {
                // This wireframe drawing function is easier to use, because we don't have to mess with the material and
                // stuff.
                gw->setRndLimits( get_icon_mesh_rnd_limits( GW_WIREFRAME | ( GW_Z_BUFFER & rndLimits ) ) );
                frantic::max3d::draw_mesh_wireframe( gw, iconMesh, lineColor );
                /*
                FrostIconMaterial iconMaterial( fillColor );
                gw->setMaterial( iconMaterial );
                gw->setColor( LINE_COLOR, lineColor.r, lineColor.g, lineColor.b );
                gw->setColor( FILL_COLOR, fillColor.r, fillColor.g, fillColor.b );

                Rect damageRect = pView->GetDammageRect();
                //frantic::max3d::draw_mesh_wireframe( gw, iconMesh, lineColor );
                iconMesh->render( gw, & iconMaterial, ( flags & USE_DAMAGE_RECT ) ? & damageRect : NULL, COMP_ALL );
                */
            }

            gw->setRndLimits( rndLimits );
        }

        return TRUE;
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("Display: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return 0;
}

CreateMouseCallBack* Frost::GetCreateMouseCallBack() {
    gCreateMouseCB.SetObj( this );
    return &gCreateMouseCB;
}

#if MAX_VERSION_MAJOR >= 24
const MCHAR* Frost::GetObjectName( bool localized ) {
#elif MAX_VERSION_MAJOR >= 15
const MCHAR* Frost::GetObjectName() {
#else
MCHAR* Frost::GetObjectName() {
#endif
    return Frost_DISPLAY_NAME;
}

BOOL Frost::HasViewDependentBoundingBox() { return TRUE; }

#if MAX_VERSION_MAJOR >= 17

unsigned long Frost::GetObjectDisplayRequirement() const {
    return MaxSDK::Graphics::ObjectDisplayRequireLegacyDisplayMode;
}

#elif MAX_VERSION_MAJOR >= 14

bool Frost::RequiresSupportForLegacyDisplayMode() const { return true; }

#endif

#if MAX_VERSION_MAJOR >= 17

bool Frost::PrepareDisplay( const MaxSDK::Graphics::UpdateDisplayContext& prepareDisplayContext ) {
    try {
        BuildMesh( prepareDisplayContext.GetDisplayTime() );

        if( get_active_mesh().getNumVerts() > 0 ) {
            mRenderItemHandles.ClearAllRenderItems();

            MaxSDK::Graphics::IMeshDisplay2* pMeshDisplay = static_cast<MaxSDK::Graphics::IMeshDisplay2*>(
                get_active_mesh().GetInterface( IMesh_DISPLAY2_INTERFACE_ID ) );
            if( !pMeshDisplay ) {
                return false;
            }

            MaxSDK::Graphics::GenerateMeshRenderItemsContext generateRenderItemsContext;
            generateRenderItemsContext.GenerateDefaultContext( prepareDisplayContext );
            pMeshDisplay->PrepareDisplay( generateRenderItemsContext );

            return true;
        } else {
            mRenderItemHandles.ClearAllRenderItems();
            return true;
        }
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("PrepareDisplay: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return false;
}

bool Frost::UpdatePerNodeItems( const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
                                MaxSDK::Graphics::UpdateNodeContext& nodeContext,
                                MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer ) {
    try {
        if( get_active_mesh().getNumVerts() > 0 ) {
            MaxSDK::Graphics::IMeshDisplay2* pMeshDisplay = static_cast<MaxSDK::Graphics::IMeshDisplay2*>(
                get_active_mesh().GetInterface( IMesh_DISPLAY2_INTERFACE_ID ) );
            if( !pMeshDisplay ) {
                return false;
            }

            MaxSDK::Graphics::GenerateMeshRenderItemsContext generateRenderItemsContext;
            generateRenderItemsContext.GenerateDefaultContext( updateDisplayContext );
            generateRenderItemsContext.RemoveInvisibleMeshElementDescriptions( nodeContext.GetRenderNode() );

            pMeshDisplay->GetRenderItems( generateRenderItemsContext, nodeContext, targetRenderItemContainer );

            return true;
        }
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg =
            _T("UpdatePerNodeItems: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return false;
}

#elif MAX_VERSION_MAJOR >= 15

bool Frost::UpdateDisplay( const MaxSDK::Graphics::MaxContext& /*maxContext*/,
                           const MaxSDK::Graphics::UpdateDisplayContext& displayContext ) {
    try {
        BuildMesh( displayContext.GetDisplayTime() );

        if( get_active_mesh().getNumVerts() > 0 ) {
            MaxSDK::Graphics::GenerateMeshRenderItemsContext generateRenderItemsContext;
            generateRenderItemsContext.GenerateDefaultContext( displayContext );
            get_active_mesh().GenerateRenderItems( mRenderItemHandles, generateRenderItemsContext );
            return true;
        } else {
            mRenderItemHandles.ClearAllRenderItems();
            return true;
        }

    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("UpdateDisplay: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return false;
}

#elif MAX_VERSION_MAJOR == 14

bool Frost::UpdateDisplay( unsigned long renderItemCategories,
                           const MaxSDK::Graphics::MaterialRequiredStreams& materialRequiredStreams, TimeValue t ) {
    try {
        BuildMesh( t );

        MaxSDK::Graphics::GenerateRenderItems( mRenderItemHandles, &get_active_mesh(), renderItemCategories,
                                               materialRequiredStreams );

        return true;
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("UpdateDisplay: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return false;
}

#endif

// from Object

ObjectState Frost::Eval( TimeValue /*t*/ ) { return ObjectState( this ); }

void Frost::InitNodeName( MSTR& s ) { s = Frost_DISPLAY_NAME; }

Interval Frost::ObjectValidity( TimeValue t ) { return Interval( t, t ); }

BOOL Frost::PolygonCount( TimeValue t, int& numFaces, int& numVerts ) {
    try {
        BuildMesh( t );
        Mesh& activeMesh = get_active_mesh();
        numFaces = activeMesh.numFaces;
        numVerts = activeMesh.numVerts;
        return TRUE;
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("PolygonCount: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    numFaces = 0;
    numVerts = 0;
    return FALSE;
}

BOOL Frost::UseSelectionBrackets() { return TRUE; }

void Frost::GetWorldBoundBox( TimeValue t, INode* inode, ViewExp* vpt, Box3& box ) {
    try {
        GetLocalBoundBox( t, inode, vpt, box );
        box = box * inode->GetNodeTM( t );
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("GetWorldBoundBox: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

Box3 Frost::get_mesh_bound_box( TimeValue t ) {
    BuildMesh( t );
    Mesh& activeMesh = get_active_mesh();
    return activeMesh.getBoundingBox();
}

void Frost::GetLocalBoundBox( TimeValue t, INode* inode, ViewExp* /*vpt*/, Box3& box ) {
    try {
        box.Init();

#if MAX_VERSION_MAJOR == 14 || MAX_VERSION_MAJOR == 15 || MAX_VERSION_MAJOR == 16 || MAX_VERSION_MAJOR == 17
        // Attempted workaround for "evaluate while hidden" issue.
        // This issue appears in 3ds Max 2012 - 2013 when Nitrous is off
        // and the time is changed by dragging the time slider.
        // Also observed in 3ds Max 2014 when Nitrous is off, the time
        // slider is dragged, and the mouse is released over a viewport.
        const bool getMeshBoundBox =
            m_inRenderingMode || MaxSDK::Graphics::IsRetainedModeEnabled() || !inode->IsNodeHidden();
#else
        const bool getMeshBoundBox = true;
#endif

        if( getMeshBoundBox ) {
            box = get_mesh_bound_box( t );
        }

        Mesh* iconMesh = get_icon_mesh( t );
        if( iconMesh ) {
            // Compute the world-space scaled bounding box
            float scale = m_iconSize.at_time( t );
            Matrix3 iconMatrix( 1 );
            iconMatrix.Scale( Point3( scale, scale, scale ) );
            box += iconMesh->getBoundingBox( &iconMatrix );
        }

        boost::optional<boundbox3f> roiBox = get_roi_box( t );
        if( roiBox ) {
            Matrix3 tm = inode->GetNodeTM( t );
            box += frantic::max3d::to_max_t( *roiBox ) * Inverse( tm );
        }
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("GetLocalBoundBox: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

int Frost::NumberOfRenderMeshes() { return SimpleObject2::NumberOfRenderMeshes(); }

// from Animatable

void Frost::DeleteThis() { delete this; }

Class_ID Frost::ClassID() { return Frost_CLASS_ID; }

void Frost::FreeCaches() {
    SimpleObject2::FreeCaches();
    clear_mesh( mesh );
    m_cachedMeshTime = TIME_NegInfinity;
    m_cachedTrimesh3.clear_and_deallocate();
    clear_particle_cache();
    ivalid.SetEmpty();
}

#if MAX_VERSION_MAJOR >= 24
void Frost::GetClassName( MSTR& s, bool localized )
#else
void Frost::GetClassName( MSTR& s )
#endif
{
    s = Frost_CLASS_NAME;
}

void* Frost::GetInterface( ULONG id ) {
    if( id == I_VRAYGEOMETRY ) {
        try {
            if( use_vray_instancing() ) {
                m_inRenderingInstanceMode = true;

                if( !m_frostVRayLibrary ) {
                    m_frostVRayLibrary = LoadFrostVRayLibrary();
                }

                if( m_frostVRayLibrary ) {
                    VUtils::VRenderObject* vrenderObject = m_frostVRayLibrary->CreateVRenderObject( this );

                    if( vrenderObject ) {
                        FF_LOG( debug ) << "Using V-Ray Render Object" << std::endl;
                        return vrenderObject;
                    } else {
                        FF_LOG( error ) << "Internal Error: V-Ray Render Object is NULL" << std::endl;
                    }
                } else {
                    FF_LOG( error )
                        << "Internal Error: Got request for V-Ray Geometry, but Frost V-Ray library was not loaded."
                        << std::endl;
                }
            }
        } catch( std::exception& e ) {
            const frantic::tstring errmsg = _T("GetInterface: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
            report_error( errmsg );
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        }

        return 0;
    } else {
        return SimpleObject2::GetInterface( id );
    }
}

void Frost::ReleaseInterface( ULONG id, void* i ) {
    if( id == I_VRAYGEOMETRY ) {
        if( m_frostVRayLibrary ) {
            m_frostVRayLibrary->DestroyVRenderObject( (VUtils::VRenderObject*)i );
        }
    } else {
        SimpleObject2::ReleaseInterface( id, i );
    }
}

BaseInterface* Frost::GetInterface( Interface_ID id ) {
    if( id == Frost_INTERFACE_ID ) {
        return static_cast<frantic::max3d::fpwrapper::FFMixinInterface<Frost>*>( this );
    } else {
        return SimpleObject2::GetInterface( id );
    }
}

int Frost::RenderBegin( TimeValue /*t*/, ULONG flags ) {
    try {
        // Only switch to rendering mode if it's not in the material editor
        const bool inMEdit = ( flags & RENDERBEGIN_IN_MEDIT ) != 0;
        if( !inMEdit ) {
            // This is necessary if a modifier is on top of this geometry object, because otherwise it
            // just reuses its cached viewport copy.
            m_inRenderingMode = true;
            m_inRenderingInstanceMode = false;

            // This was required to update render particles between renders when
            // using a V-Ray VRenderObject.  Previously I was checking
            // is_particles_invalid() to decide whether to invalidate the particles,
            // but this missed some particle changes.
            if( is_renderer( RENDERER::VRAY ) && use_vray_instancing() ) {
                invalidate_particle_node_cache();
            }

            // When using V-Ray, delay notification until NOTIFY_RENDER_PREEVAL
            //
            // It seems that V-Ray does not like getting calls to
            // GetCOREInterface()->CheckForRenderAbort() before NOTIFY_RENDER_PREEVAL --
            // the first Frost object will be fine, but subsequent Frost objects
            // will disappear from the render if they are selected in the viewport.
            // Really weird...
            if( is_network_render_server() || !is_render_active() || !is_renderer( RENDERER::VRAY ) ) {
                ivalid = NEVER;
                NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
            }
        }
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("RenderBegin: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return 1;
}

int Frost::RenderEnd( TimeValue /*t*/ ) {
    try {
        if( m_inRenderingMode ) {
            m_inRenderingMode = false;
            m_inRenderingInstanceMode = false;
            ivalid = NEVER;
            ClearRenderTime();
            // This is necessary if a modifier is on top of this geometry object, because otherwise it
            // just reuses its cached render copy.
            NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
        }
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg = _T("RenderEnd: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return 1;
}

int Frost::NumParamBlocks() { return frost_param_block_count; }

IParamBlock2* Frost::GetParamBlock( int i ) {
    switch( i ) {
    case 0:
        return pblock2;
    default:
        return 0;
    }
}

IParamBlock2* Frost::GetParamBlockByID( short id ) {
    if( id == pblock2->ID() ) {
        return pblock2;
    } else {
        return 0;
    }
}

// from ref

#if MAX_VERSION_MAJOR >= 17
RefResult Frost::NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                   RefMessage message, BOOL propagate ) {
#else
RefResult Frost::NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message ) {
#endif
    RefResult res = REF_SUCCEED;
    // Avoid default implementation for REFMSG_CHANGE, because it always sets
    // ivalid to empty, which leads to an update even if we'd skip it due to
    // disabling Update When Frost Changes / When Particles Change.  (Such an
    // update is relatively fast -- it uses the cached mesh -- but a user
    // complained about it.)
    if( message != REFMSG_CHANGE ) {
#if MAX_VERSION_MAJOR >= 17
        res = SimpleObject2::NotifyRefChanged( changeInt, hTarget, partID, message, propagate );
#else
        res = SimpleObject2::NotifyRefChanged( changeInt, hTarget, partID, message );
#endif
    }

    try {
        switch( message ) {
        case REFMSG_TARGET_DELETED:
            if( hTarget == pblock2 ) {
                ParamID changingParam = pblock2->LastNotifyParamID();
                if( changingParam == pb_nodeList ) {
                    if( partID & ALL_CHANNELS ) {
                        invalidate_particles();
                        maybe_invalidate( 0 );
                    }
                } else if( changingParam == pb_geometryList ) {
                    if( partID & ALL_CHANNELS ) {
                        invalidate_custom_geometry();
                        maybe_invalidate( 0 );
                    }
                }
                GetFrostParamBlockDesc()->InvalidateUI();
            }
            break;
        case REFMSG_CHANGE:
            if( hTarget == pblock2 ) {
                int tabIndex = -1;
                ParamID changingParam = pblock2->LastNotifyParamID( tabIndex );
                if( changingParam == pb_nodeList ) {
                    if( partID & ALL_CHANNELS ) {
                        invalidate_particles();
                        maybe_invalidate( 0 );
                    }
                } else if( changingParam == pb_geometryList ) {
                    if( partID & ALL_CHANNELS ) {
                        invalidate_custom_geometry();
                        maybe_invalidate( 0 );
                    }
                } else if( changingParam == pb_geometryOrientationLookAtNode ) {
                    if( partID & PART_ALL ) {
                        const TimeValue t = GetCOREInterface()->GetTime();
                        Matrix3 tm = get_target_tm( t );
                        Matrix3 cachedTm = get_cached_target_tm();
                        bool doInvalidate = false;
                        if( tm.GetTrans() != cachedTm.GetTrans() ) {
                            invalidate_target_pos();
                            doInvalidate = true;
                        }
                        tm.NoTrans();
                        cachedTm.NoTrans();
                        if( !( tm == cachedTm ) ) {
                            invalidate_target_rot();
                            doInvalidate = true;
                        }
                        if( doInvalidate ) {
                            maybe_invalidate( 0 );
                        }
                    }
                }
                GetFrostParamBlockDesc()->InvalidateUI();
            }
            break;
        case REFMSG_NODE_NAMECHANGE:
            // look for the node in
            // nodeList
            // pfEventList
            // tpGroupList
            // geometryList
            // refresh where you find it ?
            // need to add new refresh events which attempt to keep the old selection ?
            if( editObj == this && hTarget == pblock2 ) {
                int tabIndex = -1;
                ParamID changingParam = pblock2->LastNotifyParamID( tabIndex );

                const TimeValue t = GetCOREInterface()->GetTime();

                bool updateNodeList = false;
                bool updatePFEventList = false;
                bool updateTPGroupList = false;
                bool updateGeometryList = false;

                if( changingParam == pb_nodeList ) {
                    updateNodeList = true;
                    // check if node[tabIndex] is Particle Flow or thinkingParticles
                    // if so, then update the corresponding list
                    if( tabIndex >= 0 && static_cast<std::size_t>( tabIndex ) < m_nodeList.size() ) {
                        INode* node = m_nodeList[tabIndex].at_time( t );
                        if( is_node_particle_flow( node ) ) {
                            updatePFEventList = true;
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
                        } else if( tp_interface::get_instance().is_node_thinking_particles( node ) ) {
                            updateTPGroupList = true;
#endif
                        }
                    }
                } else if( changingParam == pb_pfEventList ) {
                    updatePFEventList = true;
                } else if( changingParam == pb_tpGroupList ) {
                    updateTPGroupList = true;
                } else if( changingParam == pb_geometryList ) {
                    updateGeometryList = true;
                }

                if( updateNodeList ) {
                    particleObjectsDlgProc.invalidate_node_list_labels();
                }
                if( updatePFEventList ) {
                    particleFlowDlgProc.invalidate_group_list_labels();
                }
                if( updateTPGroupList ) {
                    tpGroupsDlgProc.invalidate_group_list_labels();
                }
                if( updateGeometryList ) {
                    geometryDlgProc.invalidate_geometry_list_labels();
                }
            }
            break;
        // TODO: improve how we capture events from Particle Flow.
        // Maybe I'm missing something, but it seems like a lot of
        // changes that people expect to see updates from only send
        // GetNumParticles* and GetBoundBoxRequest?
        case kPFMSG_GetNumParticles:
        case kPFMSG_GetNumParticlesGenerated:
        case kPFMSG_InvalidateParticles:
        case kPFMSG_InvalidateViewportParticles:
        case kPFMSG_InvalidateRenderParticles:
            if( hTarget == pblock2 ) {
                int tabIndex = -1;
                ParamID changingParam = pblock2->LastNotifyParamID( tabIndex );
                if( changingParam == pb_nodeList ) {
                    invalidate_particles();
                    maybe_invalidate( 0 );
                }
            }
            break;
        }
    } catch( std::exception& e ) {
        const frantic::tstring errmsg = _T("NotifyRefChanged: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        clear_mesh( mesh );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return res;
}

IOResult Frost::Save( ISave* isave ) {
    // This must be left in for backwards compatibility
    // We were using it to indicate that files were saved
    // with a licensed version of Frost as an anti-piracy measure
    // We're continuting to add this marker to files now
    // in-case someone needs to open a scene using an
    // old version of Frost.
    isave->BeginChunk( CHUNK_ID::LICENSE );
    isave->EndChunk();

    return IO_OK;
}

// From ReferenceTarget

ReferenceTarget* Frost::Clone( RemapDir& remap ) {
    Frost* newObj = new Frost;
    for( int i = 0, iEnd = newObj->NumRefs(); i < iEnd; ++i )
        newObj->ReplaceReference( i, remap.CloneRef( this->GetReference( i ) ) );
    this->BaseClone( this, newObj, remap );
    return newObj;
}

// From SimpleObject

void Frost::BuildMesh( TimeValue t ) { BuildMesh( t, get_default_camera() ); }

void Frost::InvalidateUI() { GetFrostParamBlockDesc()->InvalidateUI( pblock2->LastNotifyParamID() ); }

// Frost

void Frost::BuildMesh( TimeValue t, const frantic::graphics::camera<float>& camera ) {
    if( ivalid.InInterval( t ) )
        return;

    ivalid.SetInstant( t );

    try {
        clear_mesh( get_inactive_mesh() );
        if( !is_saving_to_file() || get_build_during_save() || t != 0 || t == m_cachedMeshTime || m_inRenderingMode ||
            is_network_render_server() ) {
            build_mesh( t, camera, get_active_mesh() );
        } else {
            // just leave the mesh alone for now -- seems the mem free calls can be very slow..?
            // clear_mesh( get_active_mesh() );
        }
    } catch( frantic::logging::progress_cancel_exception& ) {
        clear_mesh( mesh );
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg =
            _T("BuildMesh (") + get_node_name() + _T("): ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        clear_mesh( mesh );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    // this is here intentionally, because ivalid can be reset during build_mesh()
    ivalid.SetInstant( t );
}

void Frost::SetRenderTime( TimeValue t ) { m_renderTime = t; }

void Frost::ClearRenderTime() { m_renderTime = TIME_NegInfinity; }

TimeValue Frost::GetRenderTime() { return m_renderTime; }

bool Frost::HasValidRenderTime() { return m_renderTime != TIME_NegInfinity; }

void Frost::set_empty_validity_and_notify_dependents() {
    ivalid.SetEmpty();
    NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
}

void Frost::force_invalidate() {
    m_cachedMeshTime = TIME_NegInfinity;
    m_cachedNativeChannelMapsTime = TIME_NegInfinity;
    m_statusIndicatorTime = TIME_NegInfinity;
    m_isVectorChannelNameCacheDirty = true;
    ivalid.SetEmpty();
    NotifyDependents( FOREVER, (PartID)PART_ALL, REFMSG_CHANGE );
}

void Frost::force_viewport_update() {
    invalidate_particle_cache();
    force_invalidate();
}

void Frost::maybe_invalidate( TimeValue t ) {
    const bool updateOnFrostChange = m_updateOnFrostChange.at_time( t );
    const bool updateOnParticleChange = m_updateOnParticleChange.at_time( t );
    const int meshingMethod = m_meshingMethod.at_time( t );

    if( updateOnFrostChange && is_frost_invalid() || updateOnParticleChange && is_particles_invalid() ||
        m_inRenderingMode || is_network_render_server() ||
        updateOnFrostChange && meshingMethod == MESHING_METHOD::GEOMETRY &&
            m_geometryType.at_time( t ) == GEOMETRY_TYPE::CUSTOM_GEOMETRY && is_custom_geometry_invalid() ||
        updateOnFrostChange && meshingMethod == MESHING_METHOD::GEOMETRY &&
            m_geometryOrientationMode.at_time( t ) == GEOMETRY_ORIENTATION_MODE::LOOK_AT && is_target_pos_invalid() ||
        updateOnFrostChange && meshingMethod == MESHING_METHOD::GEOMETRY &&
            m_geometryOrientationMode.at_time( t ) == GEOMETRY_ORIENTATION_MODE::MATCH_OBJECT &&
            is_target_rot_invalid() ) {
        if( updateOnParticleChange && is_particles_invalid() ) {
            invalidate_particle_node_cache();
        }
        force_invalidate();
    }
}

void Frost::invalidate_tab_param( ParamID id, TimeValue t ) {
    switch( id ) {
    case pb_nodeList:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_particle_objects_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                particleObjectsDlgProc.InvalidateUI( hwnd, id );
                invalidate_status_indicators();
            }
        }
        invalidate_frost();
        invalidate_vector_channel_name_cache();
        maybe_invalidate( t );
        break;
    case pb_pfEventList:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_particle_flow_events_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                particleFlowDlgProc.InvalidateUI( hwnd, id );
                invalidate_status_indicators();
            }
        }
        if( m_pfEventFilterMode.at_time( t ) == PARTICLE_GROUP_FILTER_MODE::SELECTED ) {
            invalidate_frost();
            invalidate_particles();
            invalidate_particle_node_cache();
            maybe_invalidate( t );
        }
        break;
    case pb_tpGroupList:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_tp_groups_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                tpGroupsDlgProc.InvalidateUI( hwnd, id );
                invalidate_status_indicators();
            }
        }
        if( m_tpGroupFilterMode.at_time( t ) == PARTICLE_GROUP_FILTER_MODE::SELECTED ) {
            invalidate_frost();
            invalidate_particles();
            invalidate_particle_node_cache();
            maybe_invalidate( t );
        }
        break;
    case pb_fileList:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_particle_files_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                particleFilesDlgProc.InvalidateUI( hwnd, id );
                invalidate_status_indicators();
            }
        }
        invalidate_frost();
        invalidate_particles();
        invalidate_vector_channel_name_cache();
        invalidate_source_sequences();
        // UpdateSourceSequences( t );
        maybe_invalidate( t );
        break;
    case pb_geometryList:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_geometry_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                geometryDlgProc.InvalidateUI( hwnd, id );
            }
        }
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryMaterialIDNodeList:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryMaterialIDInList:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryMaterialIDOutList:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    }
}

void Frost::on_param_set( PB2Value& v, ParamID id, int /*tabIndex*/, TimeValue t ) {
    switch( id ) {
    // frost
    case pb_showIcon:
        invalidate_preset();
        break;
    case pb_iconSize:
        invalidate_preset();
        break;

    case pb_updateOnFrostChange:
        invalidate_preset();
        if( m_updateOnFrostChange.at_time( t ) && is_frost_invalid() ) {
            maybe_invalidate( t );
        }
        break;
    case pb_updateOnParticleChange:
        invalidate_preset();
        if( m_updateOnParticleChange.at_time( t ) && is_particles_invalid() ) {
            maybe_invalidate( t );
        }
        break;
    case pb_writeVelocityMapChannel:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_velocityMapChannel:
        invalidate_preset();
        invalidate_frost();
        if( pblock2 && m_writeVelocityMapChannel.at_time( t ) ) {
            maybe_invalidate( t );
        }
        break;

    // nodes
    case pb_nodeList:
        invalidate_tab_param( id, t );
        break;

    // pflow
    case pb_pfEventFilterMode:
    case pb_useAllPFEvents:
        invalidate_preset();
        invalidate_particles();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_pfEventList:
        invalidate_tab_param( id, t );
        break;

    // thinkingParticles
    case pb_tpGroupFilterMode:
        invalidate_preset();
        invalidate_particles();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_tpGroupList:
        invalidate_tab_param( id, t );
        break;

    // files
    case pb_fileList:
        invalidate_tab_param( id, t );
        break;
    case pb_loadSingleFrame:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_frameOffset:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_limitToRange:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_rangeFirstFrame:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_rangeLastFrame:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_enablePlaybackGraph:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_playbackGraphTime:
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_beforeRangeBehavior:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_particle_files_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                particleFilesDlgProc.InvalidateUI( hwnd, pb_beforeRangeBehavior );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_afterRangeBehavior:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_particle_files_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                particleFilesDlgProc.InvalidateUI( hwnd, pb_afterRangeBehavior );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_fileLengthUnit:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_particle_files_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                particleFilesDlgProc.InvalidateUI( hwnd, pb_fileLengthUnit );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_fileCustomScale:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;

    // material
    case pb_materialMode:
        if( editObj == this && pblock2 ) {
            invalidate_status_indicators();
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_undefinedMaterialId:
        if( v.i < 1 ) {
            v.i = 1;
        }
        if( v.i > 65536 ) {
            v.i = 65536;
        }
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryMaterialIDNodeList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    case pb_geometryMaterialIDInList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    case pb_geometryMaterialIDOutList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;

    // meshing
    case pb_meshingMethod:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_meshing_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                meshingDlgProc.InvalidateUI( hwnd, pb_meshingMethod );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_enableViewportMesh:
        invalidate_preset();
        if( m_enableViewportMesh.at_time( t ) ) {
            invalidate_frost();
            invalidate_particles();
        }
        maybe_invalidate( t );
        break;
    case pb_enableRenderMesh:
        invalidate_preset();
        break;
    case pb_viewRenderParticles:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_viewportLoadMode:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_meshing_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                meshingDlgProc.InvalidateUI( hwnd, pb_viewportLoadMode );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_viewportLoadPercent:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_radius:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_useRadiusChannel:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_randomizeRadius:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_radiusRandomVariation:
        invalidate_preset();
        if( m_randomizeRadius.at_time( t ) ) {
            invalidate_frost();
            maybe_invalidate( t );
        }
        break;
    case pb_radiusSeed:
        invalidate_preset();
        if( m_randomizeRadius.at_time( t ) ) {
            invalidate_frost();
            maybe_invalidate( t );
        }
        break;
    case pb_enableRadiusScale:
    case pb_radiusScale:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_radiusAnimationMode:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_meshing_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                meshingDlgProc.InvalidateUI( hwnd, pb_radiusAnimationMode );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_motionBlurMode:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_meshing_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                meshingDlgProc.InvalidateUI( hwnd, pb_motionBlurMode );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;

    // meshing res.
    case pb_renderUsingViewportSettings:
        invalidate_preset();
        break;
    case pb_renderMeshingResolution:
        invalidate_preset();
        break;
    case pb_renderVertRefinementIterations:
        invalidate_preset();
        break;
    case pb_previewAsGeometry:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_viewportMeshingResolution:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_viewportVertRefinementIterations:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_meshingResolutionMode:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_meshing_quality_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                meshingQualityDlgProc.InvalidateUI( hwnd, id );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_renderVoxelLength:
        invalidate_preset();
        break;
    case pb_viewportVoxelLength:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;

    // metaballs
    case pb_metaballRadiusScale:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_metaballIsosurfaceLevel:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;

    // zhu-bridson
    case pb_zhuBridsonBlendRadiusScale:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_zhuBridsonEnableLowDensityTrimming:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_zhuBridsonLowDensityTrimmingThreshold:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_zhuBridsonLowDensityTrimmingStrength:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;

    // geometry
    case pb_geometryType:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_geometry_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                geometryDlgProc.InvalidateUI( hwnd, pb_geometryType );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryList:
        invalidate_tab_param( id, t );
        break;
    case pb_geometrySelectionMode:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_geometry_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                geometryDlgProc.InvalidateUI( hwnd, pb_geometrySelectionMode );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometrySelectionSeed:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometrySampleTimeBaseMode:
    case pb_geometrySampleTimeOffsetMode:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_geometry_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                geometryDlgProc.InvalidateUI( hwnd, id );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometrySampleTimeMaxRandomOffset:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometrySampleTimeSeed:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationMode:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_geometry_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                geometryDlgProc.InvalidateUI( hwnd, id );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationX:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationY:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationZ:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationLookAtNode:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_geometry_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                geometryDlgProc.InvalidateUI( hwnd, pb_geometryOrientationLookAtNode );
            }
        }
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationVectorChannel:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_geometry_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                geometryDlgProc.InvalidateUI( hwnd, pb_geometryOrientationVectorChannel );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationDivergence:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationRestrictDivergenceAxis:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationDivergenceAxisSpace:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_geometry_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                geometryDlgProc.InvalidateUI( hwnd, pb_geometryOrientationDivergenceAxisSpace );
            }
        }
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationDivergenceAxisX:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationDivergenceAxisY:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;
    case pb_geometryOrientationDivergenceAxisZ:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;

    // anisotropic
    case pb_anisotropicRadiusScale:
    case pb_anisotropicWindowScale:
    case pb_anisotropicIsosurfaceLevel:
    case pb_anisotropicMaxAnisotropy:
    case pb_anisotropicMinNeighborCount:
    // case pb_anisotropicEnablePositionSmoothing:
    case pb_anisotropicPositionSmoothingWindowScale:
    case pb_anisotropicPositionSmoothingWeight:
        invalidate_preset();
        invalidate_frost();
        maybe_invalidate( t );
        break;

    // roi
    case pb_roiCenterX:
    case pb_roiCenterY:
    case pb_roiCenterZ:
    case pb_roiSizeX:
    case pb_roiSizeY:
    case pb_roiSizeZ:
        invalidate_particles();
        maybe_invalidate( t );
        break;
    case pb_enableRenderROI:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_roi_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                roiDlgProc.InvalidateUI( hwnd, pb_enableRenderROI );
            }
        }
        break;
    case pb_enableViewportROI:
        if( editObj == this && pblock2 ) {
            IParamMap2* pm = pblock2->GetMap( frost_roi_param_map );
            if( pm ) {
                HWND hwnd = pm->GetHWnd();
                roiDlgProc.InvalidateUI( hwnd, pb_enableViewportROI );
            }
        }
        invalidate_frost();
        invalidate_particles();
        maybe_invalidate( t );
        break;
    }
}

void Frost::on_tab_changed( PBAccessor::tab_changes /*changeCode*/, Tab<PB2Value>* /*tab*/, ParamID id,
                            int /*tabIndex*/, int /*count*/ ) {
    switch( id ) {
    case pb_nodeList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    case pb_pfEventList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    case pb_tpGroupList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    case pb_fileList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    case pb_geometryList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    case pb_geometryMaterialIDNodeList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    case pb_geometryMaterialIDInList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    case pb_geometryMaterialIDOutList:
        invalidate_tab_param( id, GetCOREInterface()->GetTime() );
        break;
    }
}

void Frost::invalidate_status_indicators() { m_statusIndicatorTime = TIME_NegInfinity; }

void Frost::update_status_indicators( TimeValue t ) {
    if( editObj == this && m_statusIndicatorTime != t ) {
        IParamMap2* pm = pblock2->GetMap( frost_meshing_param_map );
        if( pm ) {
            pm->Invalidate();
        }
        pm = pblock2->GetMap( frost_material_param_map );
        if( pm ) {
            pm->Invalidate();
        }
        m_statusIndicatorTime = t;
    }
}

bool Frost::is_saving_to_file() {
    Interface8* pInterface = GetCOREInterface8();
    if( pInterface ) {
        return pInterface->IsSavingToFile();
    } else {
        return false;
    }
}

bool Frost::is_frost_invalid() { return m_isFrostDirty; }
void Frost::invalidate_frost() { m_isFrostDirty = true; }
bool Frost::is_particles_invalid() { return m_isParticlesDirty; }
void Frost::invalidate_particles() { m_isParticlesDirty = true; }
bool Frost::is_custom_geometry_invalid() { return m_isCustomGeometryDirty; }
void Frost::invalidate_custom_geometry() { m_isCustomGeometryDirty = true; }
bool Frost::is_target_pos_invalid() { return m_isTargetPosDirty; }
void Frost::invalidate_target_pos() { m_isTargetPosDirty = true; }
bool Frost::is_target_rot_invalid() { return m_isTargetRotDirty; }
void Frost::invalidate_target_rot() { m_isTargetRotDirty = true; }
void Frost::set_valid() {
    m_isFrostDirty = false;
    m_isParticlesDirty = false;
    m_isCustomGeometryDirty = false;
    m_isTargetPosDirty = false;
    m_isTargetRotDirty = false;
}

void Frost::invalidate_preset() {
    if( !m_isLoadingPreset && !get_preset_name().empty() ) {
        set_preset_name( _T("") );
        if( m_presetsParamMap ) {
            HWND hwnd = m_presetsParamMap->GetHWnd();
            if( hwnd ) {
                presetsDlgProc.invalidate_preset( hwnd );
            }
        }
    }
}

frantic::tstring Frost::get_preset_name() { return m_presetName; }
void Frost::set_preset_name( const frantic::tstring& presetName ) { m_presetName = presetName; }
void Frost::invalidate_preset_name_list() {}
void Frost::get_preset_name_list( std::vector<frantic::tstring>& outPresetNames ) {
    outPresetNames.clear();

    std::vector<frantic::tstring> presetNames;
    try {
        presetNames =
            frantic::max3d::mxs::expression(
                _T("if (FrostUi!=undefined) then ( try( FrostUi.getPresetNameList() )catch( #() ) ) else #()") )
                .evaluate<std::vector<frantic::tstring>>();
    } catch( std::exception& e ) {
        report_warning( _T("Could not get presets list:") + frantic::strings::to_tstring( e.what() ) );
        presetNames.clear();
    }

    outPresetNames.reserve( presetNames.size() );
    BOOST_FOREACH( const frantic::tstring& s, presetNames ) {
        outPresetNames.push_back( s );
    }

    // std::swap( outPresetNames, presetNames );
}

frantic::tstring Frost::get_preset_filename( const frantic::tstring& presetName ) {
    frantic::tstring tOut =
        frantic::max3d::mxs::expression( _T("if (FrostUi!=undefined) then ( try( FrostUi.getPresetFilename \"") +
                                         presetName + _T("\" )catch( \"\" ) ) else \"\"") )
            .evaluate<frantic::tstring>();
    return tOut;
}

void Frost::load_preset_from_filename( const frantic::tstring& filename ) {
    m_isLoadingPreset = true;
    try {
        const frantic::tstring escapedFilename = frantic::strings::get_escaped_string( filename );
        frantic::max3d::mxs::expression( _T("if (FrostUi!=undefined) do ( try ( FrostUi.loadPreset frostNode \"") +
                                         escapedFilename + _T("\" )catch() )") )
            .bind( _T("frostNode"), this )
            .evaluate<void>();
    } catch( std::exception& e ) {
        report_warning( _T("Could not load preset from file: ") + filename + _T(".  Error: ") +
                        frantic::strings::to_tstring( e.what() ) );
    }
    m_isLoadingPreset = false;
}

void Frost::invalidate_source_sequences() { m_isSourceParticlesDirty = true; }

void Frost::validate_source_sequences( TimeValue t ) {
    if( m_isSourceParticlesDirty || ( m_fileList.size() != m_fileSequences.size() ) ) {
        UpdateSourceSequences( t );
        m_isSourceParticlesDirty = false;
    }
}

/**
 *	Updates the plugins current list of file sequences it maintains for loading level set data.
 *	This is to avoid having to create and sync file sequences during every buildmesh request.
 *
 *	@param	t	time to rebuild the list at
 */
void Frost::UpdateSourceSequences( TimeValue t ) {
    try {
        // To keep everything in order, I just rebuild the list.
        m_fileSequences.clear();
        if( m_fileList.size() == 0 )
            return;
        for( int i = 0; i < (int)m_fileList.size(); i++ ) {
            frantic::tstring filename = m_fileList[i].at_time( t );
            m_fileSequences.push_back( filename_sequence( filename ) );
            m_fileSequences[i].sync_frame_set();
        }

    } catch( const std::exception& e ) {
        const frantic::tstring errmsg =
            _T("UpdateSourceSequences: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        clear_mesh( mesh );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

void Frost::GetParticleFilenames( TimeValue t, double frame, std::vector<frantic::tstring>& outFilenames,
                                  std::vector<float>& outTimeOffsets ) {
    bool loadSingleFrame = m_loadSingleFrame.at_time( t );

    validate_source_sequences( t );

    // find the nearest frame to the requested one in each sequence
    for( size_t i = 0; i < m_fileSequences.size(); ++i ) {
        if( loadSingleFrame ) {
            outFilenames.push_back( m_fileList[static_cast<int>( i )].at_time( t ).operator const frantic::strings::tstring() );
            outTimeOffsets.push_back( 0.f );
        } else if( m_fileSequences[i].get_frame_set().frame_exists( frame ) ) {
            outFilenames.push_back( m_fileSequences[i][frame] );
            outTimeOffsets.push_back( 0.f );
        } else {
            std::pair<double, double> bracket;
            double actualFrame;
            float alpha;

            if( m_fileSequences[i].get_frame_set().empty() &&
                frantic::files::file_exists( m_fileList[static_cast<int>( i )].at_time( t ) ) ) {
                outFilenames.push_back( m_fileList[static_cast<int>( i )].at_time( t ).operator const frantic::strings::tstring() );
                outTimeOffsets.push_back( 0.f );
            } else {
                if( !m_fileSequences[i].get_nearest_subframe_interval( frame, bracket, alpha ) ) {
                    throw std::runtime_error(
                        "GetParticleFilenames: Could not find a file nearest to frame " +
                        boost::lexical_cast<std::string>( frame ) + " in sequence " +
                        frantic::strings::to_string( m_fileSequences[i].get_filename_pattern().get_pattern() ) );
                }
                if( alpha < 0.5 ) {
                    actualFrame = bracket.first;
                } else {
                    actualFrame = bracket.second;
                }
                outFilenames.push_back( m_fileSequences[i][actualFrame] );
                outTimeOffsets.push_back( (float)( ( frame - actualFrame ) / GetFrameRate() ) );
            }
        }
    }
}

#ifdef ENABLE_FROST_DISK_CACHE

// Saves the mesh at time t to the xmesh sequence being maintained by the plugin
void Frost::SaveMeshToSequence( TimeValue t, bool renderMode ) {
    if( m_cacheName.get_filename_pattern().get_pattern() == _T("") ) {
        FF_LOG( error ) << "SaveMeshToSequence: No cache name set." << std::endl;
        throw std::runtime_error( "Frost: SaveMeshToSequence: No cache name set" );
    }

    trimesh3 outMesh;
    BuildTrimesh3( t * GetTicksPerFrame(), renderMode, outMesh );
    m_xss.write_xmesh( outMesh, m_cacheName[t] );
}

/**
 *	Saves a level set generated from the particles set at time t to the level set
 *	sequence being maintained by the plugin, applying the transformation to the particles first.
 *
 *	@param	t	TimeValue at which to make the particles into a level set and save it
 *	@param	tm	The transformation matrix to apply to the particles.
 */
void Frost::SaveLevelSetToSequence( TimeValue t, Matrix3 tm ) {
    if( m_cacheName.get_filename_pattern().get_pattern() == _T("") ) {
        FF_LOG( error ) << "SaveLevelSetToSequence: No save pathname set." << std::endl;
        throw std::runtime_error( "Frost: SaveLevelSetToSequence: No save pathname set" );
    }

    frantic::volumetrics::levelset::rle_level_set outRLS;
    BuildLevelSet( t * GetTicksPerFrame(), tm, outRLS );
    frantic::volumetrics::levelset::write_rls_rle_level_set_file( m_cacheName[t], outRLS );
}

// Sets the name of the xmesh cache
frantic::tstring Frost::SetCacheName( const frantic::tstring& filename ) {

    if( m_cacheName.get_filename_pattern().matches_pattern( filename ) )
        return _T("");
    m_cacheName.get_filename_pattern().set( filename );
    m_cacheName.sync_frame_set();
    m_xss.clear();
    return m_cacheName.get_filename_pattern().get_directory( true ) + m_cacheName.get_filename_pattern().get_prefix() +
           m_cacheName.get_filename_pattern().get_extension();
}

// remove the sequence number from a filename
// should be consistent with the return value of SetCacheName above
frantic::tstring Frost::StripSequenceNumber( const frantic::tstring& filename ) {
    frantic::files::filename_pattern p( filename );
    return p.get_directory( true ) + p.get_prefix() + p.get_extension();
}
/*
 * Create a temporary file in the system's temporary directory, with the first
 * three characters of prefix used as the temporary file's prefix.  The
 * temporary file is created and closed, and its filename is returned.  The
 * caller is reponsible for deleting the temporary file.
 *
 * If the temporary file could not be created, a std::runtime_error is thrown.
 */
frantic::tstring Frost::GetTempFileNameWithPrefix( const frantic::tstring& prefix ) {
    const int buffer_len = MAX_PATH;
    TCHAR buffer_dir[MAX_PATH + 1];  // hold temporary directory
    TCHAR buffer_file[MAX_PATH + 1]; // hold filename in temp directory
    unsigned int result;
    frantic::tstring temp_file_name;

    buffer_dir[0] = 0;
    result = GetTempPath( buffer_len, buffer_dir );
    if( result == 0 ) {
        throw std::runtime_error( "Frost: GetTempFileNameWithPrefix: Unable to get temporary directory." );
    }

    buffer_file[0] = 0;
    result = GetTempFileName( buffer_dir, prefix.c_str(), 0, buffer_file );
    if( result == 0 ) {
        throw std::runtime_error(
            "Frost: GetTempFileNameWithPrefix: Unable to create temporary file in the temporary directory \'" +
            frantic::strings::to_string( buffer_dir ) + "\'." );
    }

    return buffer_file;
}
#endif // #ifdef ENABLE_FROST_DISK_CACHE

frantic::tstring Frost::get_ini_filename() {
    try {
        const frantic::tstring iniFilename = GetFrostInterface()->GetSettingsFilename();
        return iniFilename;
    } catch( std::exception& ) {
    }
    return _T("");
}

void Frost::save_preferences() { const frantic::tstring iniFilename = get_ini_filename(); }

void Frost::load_preferences() { const frantic::tstring iniFilename = get_ini_filename(); }

bool Frost::get_build_during_save() { return false; }

bool Frost::is_particle_object( INode* node ) {
    if( !node ) {
        return false;
    }

    Object* obj = node->GetObjectRef();
    if( !obj ) {
        return false;
    }
    Object* baseObj = get_base_object( node ); // prtObject->FindBaseObject(); fails?
    if( !baseObj ) {
        throw std::runtime_error( "Could not get base object from node: " +
                                  std::string( frantic::strings::to_string( node->GetName() ) ) );
    }
    ObjectState os = node->EvalWorldState( GetCOREInterface()->GetTime() );

    node->BeginDependencyTest();
    NotifyDependents( FOREVER, 0, REFMSG_TEST_DEPENDENCY );
    if( node->EndDependencyTest() ) {
        return false;
    }

    if( frantic::max3d::particles::GetIMaxKrakatoaPRTObject( baseObj ) ) {
        return true;
#if defined( PHOENIX_SDK_AVAILABLE )
    } else if( IsPhoenixObject( node, os, GetCOREInterface()->GetTime() ).first ) {
        return true;
#endif
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    } else if( tp_interface::get_instance().is_node_thinking_particles( node ) ) {
        return true;
#endif
    } else if( ParticleGroupInterface( obj ) ) {
        return true;
    } else if( GetParticleObjectExtInterface( baseObj ) ) {
        return true;
    } else if( baseObj->GetInterface( I_SIMPLEPARTICLEOBJ ) ) {
        return true;
        //} else if( krakatoa_prt_source_particle_istream::can_get_from_node( node, GetCOREInterface()->GetTime() ) ) {
        //	return true;
    } else if( os.obj->CanConvertToType( polyObjectClassID ) ) {
        return true;
    } else if( baseObj->ClassID() == SPHEREGIZMO_CLASSID || baseObj->ClassID() == CYLGIZMO_CLASSID ||
               baseObj->ClassID() == BOXGIZMO_CLASSID ) {
        return true;
    } /*else if( os.obj->CanConvertToType( triObjectClassID ) ) {
            return true;
    } */

    return false;
}

bool Frost::has_particle_object( INode* node ) {
    if( !node ) {
        return false;
    }
    if( !pblock2 ) {
        return false;
    }
    INode* listNode = 0;
    Interval listNodeValid;
    for( int i = 0, ie = pblock2->Count( pb_nodeList ); i != ie; ++i ) {
        BOOL success = pblock2->GetValue( pb_nodeList, GetCOREInterface()->GetTime(), listNode, listNodeValid, i );
        if( success && node == listNode ) {
            return true;
        }
    }
    return false;
}

bool Frost::accept_particle_object( INode* node ) {
    if( is_particle_object( node ) && !has_particle_object( node ) ) {
        node->BeginDependencyTest();
        NotifyDependents( FOREVER, 0, REFMSG_TEST_DEPENDENCY );
        if( node->EndDependencyTest() ) {
            return false;
        } else {
            return true;
        }
    } else {
        return false;
    }
}

void Frost::add_particle_object( INode* node ) {
    if( pblock2 && accept_particle_object( node ) ) {
        pblock2->Append( pb_nodeList, 1, &node );
    }
}

void Frost::remove_particle_object( const int i ) {
    if( !pblock2 ) {
        return;
    }

    if( i >= 0 && i < pblock2->Count( pb_nodeList ) ) {
        Interval valid;
        INode* node;
        BOOL success = pblock2->GetValue( pb_nodeList, 0, node, valid, i );
        if( success ) {
            if( is_node_particle_flow( node ) ) {
                // remove associated particle flow groups
                std::set<INode*> particleGroupNodes;
                extract_particle_groups( node, particleGroupNodes );
                for( int groupIndex = pblock2->Count( pb_pfEventList ) - 1; groupIndex >= 0; --groupIndex ) {
                    INode* groupNode = 0;
                    success = pblock2->GetValue( pb_pfEventList, 0, groupNode, valid, groupIndex );
                    if( success && groupNode && particleGroupNodes.find( groupNode ) != particleGroupNodes.end() ) {
                        pblock2->Delete( pb_pfEventList, groupIndex, 1 );
                    }
                }
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
            } else if( tp_interface::get_instance().is_node_thinking_particles( node ) ) {
                // remove associated thinkingparticles groups
                std::set<ReferenceTarget*> particleGroups;
                extract_tp_groups( node, particleGroups );
                for( int groupIndex = pblock2->Count( pb_tpGroupList ) - 1; groupIndex >= 0; --groupIndex ) {
                    ReferenceTarget* groupRef = 0;
                    success = pblock2->GetValue( pb_tpGroupList, 0, groupRef, valid, groupIndex );
                    if( success && groupRef && particleGroups.find( groupRef ) != particleGroups.end() ) {
                        pblock2->Delete( pb_tpGroupList, groupIndex, 1 );
                    }
                }
#endif
            }
        }

        pblock2->Delete( pb_nodeList, i, 1 );
    }
}

INode* Frost::get_particle_object( int i ) {
    if( !pblock2 ) {
        return 0;
    }
    if( i < 0 || i >= pblock2->Count( pb_nodeList ) ) {
        return 0;
    }
    INode* node = 0;
    Interval valid;
    BOOL success = pblock2->GetValue( pb_nodeList, GetCOREInterface()->GetTime(), node, valid, i );
    if( success ) {
        return node;
    } else {
        return 0;
    }
}

void Frost::get_particle_object_names( std::vector<frantic::tstring>& names ) {
    names.clear();
    if( !pblock2 ) {
        return;
    }
    for( std::size_t i = 0, ie = pblock2->Count( pb_nodeList ); i < ie; ++i ) {
        INode* node = 0;
        Interval valid;
        BOOL success = pblock2->GetValue( pb_nodeList, GetCOREInterface()->GetTime(), node, valid, (int)i );
        if( success ) {
            if( node ) {
                names.push_back( node->GetName() );
            } else {
                names.push_back( _T("<deleted>") );
            }
        } else {
            names.push_back( _T("<error>") );
        }
    }
}

void Frost::SetNodeListSelection( const std::vector<int> selection ) {
    try {
        if( editObj == this ) {
            particleObjectsDlgProc.set_node_list_selection( selection );
        }
    } catch( std::exception& e ) {
        const frantic::tstring errmsg =
            _T("SetNodeListSelection: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

std::vector<int> Frost::GetNodeListSelection( void ) {
    std::vector<int> result;
    try {
        if( editObj == this ) {
            particleObjectsDlgProc.get_node_list_selection( result );
        }
    } catch( std::exception& e ) {
        const frantic::tstring errmsg =
            _T("GetNodeListSelection: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return result;
}

/*
void Frost::SetPFEventListSelection( const std::vector<int> selection ) {
        try {
                if( editObj == this ) {
                        particleFlowDlgProc.set_pf_event_list_selection( selection );
                }
        } catch( std::exception & e ) {
                const frantic::tstring errmsg = _T("SetPFEventListSelection: ") + frantic::strings::to_tstring(e.what())
+ _T("\n"); report_error( errmsg ); LogSys* log = GetCOREInterface()->Log(); log->LogEntry( SYSLOG_ERROR, NO_DIALOG,
_T("Frost Error"), _T("%s"), errmsg.c_str() ); if( is_network_render_server() ) { throw
MAXException(const_cast<TCHAR*>(errmsg.c_str()));
                }
        }
}

std::vector<int> Frost::GetPFEventListSelection( void ) {
        std::vector<int> result;
        try {
                if( editObj == this ) {
                        particleFlowDlgProc.get_pf_event_list_selection( result );
                }
        } catch( std::exception & e ) {
                const frantic::tstring errmsg = _T("GetPFEventListSelection: ") + frantic::strings::to_tstring(e.what())
+ _T("\n"); report_error( errmsg ); LogSys* log = GetCOREInterface()->Log(); log->LogEntry( SYSLOG_ERROR, NO_DIALOG,
_T("Frost Error"), _T("%s"), errmsg.c_str() ); if( is_network_render_server() ) { throw
MAXException(const_cast<TCHAR*>(errmsg.c_str()));
                }
        }
        return result;
}
*/
bool Frost::has_particle_file( const frantic::tstring& filename ) {
    if( !pblock2 ) {
        return false;
    }

    Interval valid;

    for( int i = 0, ie = pblock2->Count( pb_fileList ); i != ie; ++i ) {
#if MAX_VERSION_MAJOR < 12
        TCHAR* listFilename = 0;
#else
        const MCHAR* listFilename = 0;
#endif
        BOOL success = pblock2->GetValue( pb_fileList, GetCOREInterface()->GetTime(), listFilename, valid, i );
        if( success && listFilename && _tcscmp( listFilename, filename.c_str() ) == 0 ) {
            return true;
        }
    }

    return false;
}

bool Frost::accept_particle_file( const frantic::tstring& filename ) {
    if( !filename.empty() && !has_particle_file( filename ) ) {
        return true;
    } else {
        return false;
    }
}

void Frost::add_particle_file( const frantic::tstring& filename ) {
    if( pblock2 && accept_particle_file( filename ) ) {
        MSTR str( filename.c_str() );
        const MCHAR* el = str.data();
        pblock2->Append( pb_fileList, 1, const_cast<MCHAR**>( &el ) );
    }
}

void Frost::remove_particle_file( int i ) {
    if( !pblock2 ) {
        return;
    }

    if( i >= 0 && i < pblock2->Count( pb_fileList ) ) {
        pblock2->Delete( pb_fileList, i, 1 );
    }
}

int Frost::get_particle_file_count() {
    if( pblock2 ) {
        return pblock2->Count( pb_fileList );
    } else {
        return 0;
    }
}

frantic::tstring Frost::get_particle_file( int i ) {
    if( !pblock2 ) {
        return _T("");
    }
    if( i < 0 || i >= pblock2->Count( pb_fileList ) ) {
        return _T("");
    }
#if MAX_VERSION_MAJOR < 12
    TCHAR* filename = 0;
#else
    const MCHAR* filename = 0;
#endif
    Interval valid;
    BOOL success = pblock2->GetValue( pb_fileList, GetCOREInterface()->GetTime(), filename, valid, i );
    if( success && filename ) {
        return filename;
    } else {
        return _T("");
    }
}

frantic::tstring Frost::get_particle_file_display_name( int i ) {
    frantic::tstring filename = get_particle_file( i );
    if( filename.empty() ) {
        return filename;
    }
    return frantic::files::to_tstring( boost::filesystem::path( filename ).filename() );
}

void Frost::get_particle_file_display_names( std::vector<frantic::tstring>& displayNames ) {
    displayNames.clear();

    if( !pblock2 ) {
        return;
    }

    for( int i = 0; i < pblock2->Count( pb_fileList ); ++i ) {
#if MAX_VERSION_MAJOR < 12
        TCHAR* filename = 0;
#else
        const MCHAR* filename = 0;
#endif
        Interval valid;
        BOOL success = pblock2->GetValue( pb_fileList, GetCOREInterface()->GetTime(), filename, valid, i );
        if( success ) {
            if( filename ) {
                displayNames.push_back(
                    frantic::files::to_tstring( boost::filesystem::path( frantic::tstring( filename ) ).filename() ) );
            } else {
                displayNames.push_back( _T("<deleted>") );
            }
        } else {
            displayNames.push_back( _T("<error>") );
        }
    }
}

void Frost::begin_pick_particle_object( HWND hwnd ) {
    HWND hwndButton = GetDlgItem( hwnd, BTN_PARTICLE_OBJECTS_PICK );
    if( hwndButton ) {
        ICustButton* iBut = GetICustButton( hwndButton );
        if( iBut ) {
            iBut->SetCheck( TRUE );
            ReleaseICustButton( iBut );
        }
    }
}

void Frost::end_pick_particle_object( HWND hwnd ) {
    HWND hwndButton = GetDlgItem( hwnd, BTN_PARTICLE_OBJECTS_PICK );
    if( hwndButton ) {
        ICustButton* iBut = GetICustButton( hwndButton );
        if( iBut ) {
            iBut->SetCheck( FALSE );
            ReleaseICustButton( iBut );
        }
    }
}

void Frost::invalidate_particle_cache() {
    invalidate_particle_node_cache();
    invalidate_particle_file_cache();
}

void Frost::invalidate_particle_node_cache() {
    BOOST_FOREACH( node_particle_cache_entry& cache, m_nodeParticleCache ) {
        cache.invalidate();
    }
}

void Frost::reload_particle_files( TimeValue t ) {
    invalidate_particle_file_cache();
    UpdateSourceSequences( t );
    invalidate_particles();
}

void Frost::invalidate_particle_file_cache() {
    BOOST_FOREACH( file_particle_cache_entry& cache, m_fileParticleCache ) {
        cache.invalidate();
    }
}

std::pair<float, float> Frost::get_particle_file_frame_range() {
    float startFrame = 0;
    bool gotStartFrame = get_start_frame( startFrame );

    float endFrame = 0;
    bool gotEndFrame = get_end_frame( endFrame );

    if( gotStartFrame && gotEndFrame ) {
        return std::pair<float, float>( startFrame, endFrame );
    } else {
        return std::pair<float, float>( 1.f, -1.f );
    }
}

void Frost::set_particle_file_first_frame( int frameNumber ) {
    if( pblock2 ) {
        pblock2->SetValue( pb_rangeFirstFrame, GetCOREInterface()->GetTime(), frameNumber );
    }
}

void Frost::set_particle_file_last_frame( int frameNumber ) {
    if( pblock2 ) {
        pblock2->SetValue( pb_rangeLastFrame, GetCOREInterface()->GetTime(), frameNumber );
    }
}

const std::vector<int>& Frost::get_before_range_behavior_codes() { return m_beforeRangeBehaviorDisplayCodes; }

frantic::tstring Frost::get_before_range_behavior_name( int code ) {
    std::map<int, frantic::tstring>::iterator i = m_beforeRangeBehaviorNames.find( code );
    if( i == m_beforeRangeBehaviorNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return i->second;
    }
}

int Frost::get_before_range_behavior() { return m_beforeRangeBehavior.at_time( 0 ); }

void Frost::set_before_range_behavior( int behavior ) {
    if( pblock2 ) {
        pblock2->SetValue( pb_beforeRangeBehavior, 0, behavior );
    }
}

const std::vector<int>& Frost::get_after_range_behavior_codes() { return m_afterRangeBehaviorDisplayCodes; }

frantic::tstring Frost::get_after_range_behavior_name( int code ) {
    std::map<int, frantic::tstring>::iterator i = m_afterRangeBehaviorNames.find( code );
    if( i == m_afterRangeBehaviorNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return i->second;
    }
}

int Frost::get_after_range_behavior() { return m_afterRangeBehavior.at_time( 0 ); }

void Frost::set_after_range_behavior( int behavior ) {
    if( pblock2 ) {
        pblock2->SetValue( pb_afterRangeBehavior, 0, behavior );
    }
}

const std::vector<int>& Frost::get_file_particle_scale_factor_codes() { return m_fileLengthUnitDisplayCodes; }

int Frost::get_file_length_unit() {
    if( !pblock2 ) {
        return -1;
    }
    int unit = 0;
    Interval valid;
    BOOL success = pblock2->GetValue( pb_fileLengthUnit, GetCOREInterface()->GetTime(), unit, valid );
    if( success ) {
        return unit;
    } else {
        return -1;
    }
}

void Frost::set_file_length_unit( const int unit ) {
    if( pblock2 ) {
        pblock2->SetValue( pb_fileLengthUnit, GetCOREInterface()->GetTime(), unit );
    }
}

frantic::tstring Frost::get_file_length_unit_name( int code ) {
    std::map<int, frantic::tstring>::iterator i = m_fileLengthUnitNames.find( code );
    if( i == m_fileLengthUnitNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return i->second;
    }
}

bool Frost::get_load_single_frame() { return m_loadSingleFrame.at_time( 0 ); }

void Frost::set_load_single_frame( bool loadSingleFrame ) { m_loadSingleFrame.at_time( 0 ) = loadSingleFrame; }

bool Frost::get_enable_playback_graph() { return m_enablePlaybackGraph.at_time( 0 ); }

bool Frost::get_limit_to_range() { return m_limitToRange.at_time( 0 ); }

void Frost::set_limit_to_range( bool limitToRange ) { m_limitToRange.at_time( 0 ) = limitToRange; }

void Frost::SetToValidFrameRange( TimeValue t ) {
    try {
        validate_source_sequences( t );

        if( m_fileList.size() != m_fileSequences.size() ) {
            throw std::runtime_error( "Internal Error: mismatch between file list size and file sequence size." );
        }

        double minFrame = std::numeric_limits<double>::min();
        double maxFrame = std::numeric_limits<double>::max();
        bool gotFile = false;

        for( std::size_t i = 0; i < m_fileSequences.size(); ++i ) {
            const frantic::files::frame_set& fset = m_fileSequences[i].get_frame_set();

            if( fset.empty() ) {
                const frantic::tstring filename = m_fileList[(int)i].at_time( t );
                const boost::filesystem::path filepath( filename );
                if( boost::filesystem::exists( filepath ) && boost::filesystem::is_regular_file( filepath ) ) {
                    // require single frame which is currently MEOW
                    const bool loadSingleFrame = m_loadSingleFrame.at_time( t );
                    const frantic::tstring msg = _T("Cannot determine a safe range.  The sequence\n") + filename +
                                                 _T("\ndoes not have a frame number.\n\nIt will load correctly only ")
                                                 _T("in 'Load Single Frame Only' mode which is currently ") +
                                                 ( loadSingleFrame ? _T("ON") : _T("OFF") ) + _T(".");
                    UINT icon = loadSingleFrame ? MB_ICONINFORMATION : MB_ICONWARNING;
                    MessageBox( GetCOREInterface()->GetMAXHWnd(), msg.c_str(), _T("Frost Range"), MB_OK | icon );
                } else {
                    // error: no files in sequence
                    const frantic::tstring msg =
                        _T("Cannot determine a safe range.  No files were found in the sequence:\n") + filename;
                    MessageBox( GetCOREInterface()->GetMAXHWnd(), msg.c_str(), _T("Frost Range"),
                                MB_OK | MB_ICONWARNING );
                }
            } else {
                std::pair<double, double> frameRange = m_fileSequences[i].get_frame_set().allframe_range();
                if( !gotFile ) {
                    minFrame = frameRange.first;
                    maxFrame = frameRange.second;
                    gotFile = true;
                } else {
                    minFrame = std::max( frameRange.first, minFrame );
                    maxFrame = std::min( frameRange.second, maxFrame );
                }
            }
        }

        if( gotFile ) {
            if( minFrame <= maxFrame ) {
                // wooo set range
                if( !theHold.Holding() ) {
                    theHold.Begin();
                }
                m_limitToRange.at_time( t ) = true;
                m_rangeFirstFrame.at_time( t ) = static_cast<int>( floor( minFrame ) );
                m_rangeLastFrame.at_time( t ) = static_cast<int>( ceil( maxFrame ) );
                theHold.Accept( ::GetString( IDS_PARAMETER_CHANGE ) );
            } else {
                // no range
                const frantic::tstring msg = _T("Cannot determine a safe range.  There are no intersecting frame ")
                                             _T("numbers in the file sequences.");
                MessageBox( GetCOREInterface()->GetMAXHWnd(), msg.c_str(), _T("Frost Range"), MB_OK | MB_ICONWARNING );
            }
        }
    } catch( std::exception& e ) {
        const frantic::tstring errmsg =
            _T("SetToValidFrameRange: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

bool Frost::get_use_custom_file_scale() {
    const int fileLengthUnit = m_fileLengthUnit.at_time( 0 );
    return ( fileLengthUnit == FILE_LENGTH_UNIT::CUSTOM );
}

std::vector<int> Frost::GetFileListSelection() {
    std::vector<int> result;
    IParamBlock2* pb = GetParamBlockByID( frost_param_block );
    if( !pb ) {
        return result;
    }
    IParamMap2* pm = pb->GetMap( frost_particle_files_param_map );
    if( !pm ) {
        return result;
    }
    const HWND hwnd = pm->GetHWnd();
    if( !hwnd ) {
        return result;
    }
    if( editObj != this ) {
        return result;
    }
    const HWND hwndFileList = GetDlgItem( hwnd, IDC_PARTICLE_FILES );
    LRESULT selCount = SendMessage( hwndFileList, LB_GETSELCOUNT, 0, 0 );
    if( selCount != LB_ERR ) {
        std::vector<int> sel( selCount + 1 );
        LRESULT lRet = SendMessage( hwndFileList, LB_GETSELITEMS, selCount, LPARAM( &sel[0] ) );
        if( lRet != LB_ERR ) {
            sel.resize( lRet );
            for( std::size_t i = 0; i < sel.size(); ++i ) {
                ++sel[i];
            }
            sel.swap( result );
        }
    }
    return result;
}

void Frost::SetFileListSelection( const std::vector<int> selection ) {
    try {
        if( editObj == this ) {
            particleFilesDlgProc.set_file_list_selection( selection );
        }
    } catch( std::exception& e ) {
        const frantic::tstring errmsg =
            _T("SetFileListSelection: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

const std::vector<int>& Frost::get_meshing_method_codes() { return m_meshingMethodDisplayCodes; }

frantic::tstring Frost::get_meshing_method_name( int code ) {
    std::map<int, frantic::tstring>::iterator iter = m_meshingMethodNames.find( code );
    if( iter == m_meshingMethodNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return iter->second;
    }
}

int Frost::get_meshing_method() { return m_meshingMethod.at_time( 0 ); }

void Frost::set_meshing_method( const int meshingMethod ) { m_meshingMethod.at_time( 0 ) = meshingMethod; }

const std::vector<int>& Frost::get_material_mode_codes() { return m_materialModeDisplayCodes; }

frantic::tstring Frost::get_material_mode_name( int code ) {
    std::map<int, frantic::tstring>::iterator iter = m_materialModeNames.find( code );
    if( iter == m_materialModeNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return iter->second;
    }
}

int Frost::get_material_mode() { return m_materialMode.at_time( 0 ); }

void Frost::set_material_mode( const int materialMode ) { m_materialMode.at_time( 0 ) = materialMode; }

MtlID Frost::get_undefined_material_id() {
    return static_cast<MtlID>(
        frantic::math::clamp<int>( m_undefinedMaterialID.at_time( 0 ) - 1, 0, std::numeric_limits<MtlID>::max() ) );
}

const std::vector<int>& Frost::get_viewport_load_mode_codes() { return m_viewportLoadModeCodes; }

frantic::tstring Frost::get_viewport_load_mode_name( int code ) {
    std::map<int, frantic::tstring>::iterator iter = m_viewportLoadModeNames.find( code );
    if( iter == m_viewportLoadModeNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return iter->second;
    }
}

int Frost::get_viewport_load_mode() { return m_viewportLoadMode.at_time( 0 ); }

void Frost::set_viewport_load_mode( int mode ) {
    if( pblock2 ) {
        m_viewportLoadMode.at_time( 0 ) = mode;
    }
}

const std::vector<int>& Frost::get_radius_animation_mode_codes() { return m_radiusAnimationModeCodes; }
frantic::tstring Frost::get_radius_animation_mode_name( int code ) {
    std::map<int, frantic::tstring>::iterator iter = m_radiusAnimationModeNames.find( code );
    if( iter == m_radiusAnimationModeNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return iter->second;
    }
}
int Frost::get_radius_animation_mode() { return m_radiusAnimationMode.at_time( 0 ); }
void Frost::set_radius_animation_mode( int mode ) {
    if( pblock2 ) {
        m_radiusAnimationMode.at_time( 0 ) = mode;
    }
}

const std::vector<int>& Frost::get_motion_blur_codes() { return m_motionBlurDisplayCodes; }

frantic::tstring Frost::get_motion_blur_name( int code ) {
    std::map<int, frantic::tstring>::iterator iter = m_motionBlurMethodNames.find( code );
    if( iter == m_motionBlurMethodNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return iter->second;
    }
}

int Frost::get_motion_blur_mode() {
    if( !pblock2 ) {
        return -1;
    }
    int motionBlurMode = 0;
    Interval valid;
    BOOL success = pblock2->GetValue( pb_motionBlurMode, 0, motionBlurMode, valid );
    if( success ) {
        return motionBlurMode;
    } else {
        return -1;
    }
}

void Frost::set_motion_blur_mode( const int motionBlurMode ) { m_motionBlurMode.at_time( 0 ) = motionBlurMode; }

std::vector<int> Frost::get_pf_event_list_selection() {
    std::vector<int> result;
    IParamBlock2* pb = GetParamBlockByID( frost_param_block );
    if( !pb ) {
        return result;
    }
    IParamMap2* pm = pb->GetMap( frost_particle_flow_events_param_map );
    if( !pm ) {
        return result;
    }
    const HWND hwnd = pm->GetHWnd();
    if( !hwnd ) {
        return result;
    }
    const HWND hwndEventList = GetDlgItem( hwnd, IDC_PARTICLE_FLOW_EVENTS );
    LRESULT selCount = SendMessage( hwndEventList, LB_GETSELCOUNT, 0, 0 );
    if( selCount != LB_ERR ) {
        std::vector<int> sel( selCount + 1 );
        LRESULT lRet = SendMessage( hwndEventList, LB_GETSELITEMS, selCount, LPARAM( &sel[0] ) );
        if( lRet != LB_ERR ) {
            sel.resize( lRet );
            for( std::size_t i = 0; i < sel.size(); ++i ) {
                ++sel[i];
            }
            sel.swap( result );
        }
    }
    return result;
}

bool Frost::has_particle_flow_event( INode* node ) {
    if( node ) {
        Object* obj = node->GetObjectRef();
        if( obj ) {
            if( IParticleGroup* pGroup = ParticleGroupInterface( obj ) ) {
                for( std::size_t i = 0; i < m_pfEventList.size(); ++i ) {
                    if( node == m_pfEventList[(int)i].at_time( 0 ) ) {
                        return true;
                    }
                }
                return false;
            }
        }
    }
    return false;
}

bool Frost::accept_particle_flow_event( INode* node ) {
    if( node ) {
        Object* obj = node->GetObjectRef();
        if( obj ) {
            if( IParticleGroup* pGroup = ParticleGroupInterface( obj ) ) {
                for( std::size_t i = 0; i < m_pfEventList.size(); ++i ) {
                    if( node == m_pfEventList[(int)i].at_time( 0 ) ) {
                        return false;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

void Frost::get_selected_particle_flow_events( std::vector<INode*>& outEvents ) {
    outEvents.clear();
    if( pblock2 ) {
        const int count = pblock2->Count( pb_pfEventList );
        if( count > 0 ) {
            outEvents.reserve( count );
            for( int i = 0; i < count; ++i ) {
                INode* node = 0;
                Interval valid;
                BOOL success = pblock2->GetValue( pb_pfEventList, GetCOREInterface()->GetTime(), node, valid, i );
                if( success ) {
                    outEvents.push_back( node );
                }
            }
        }
    }
}

void Frost::get_all_particle_flow_events( std::vector<INode*>& out ) {
    out.clear();
    std::set<INode*> particleGroupSet;

    for( size_t i = 0; i < m_nodeList.size(); ++i ) {
        extract_connected_particle_groups( m_nodeList[(int)i].at_time( 0 ), particleGroupSet );
    }

    out.reserve( particleGroupSet.size() );
    BOOST_FOREACH( INode* particleGroup, particleGroupSet ) {
        out.push_back( particleGroup );
    }
}

void Frost::get_acceptable_particle_flow_events( std::vector<INode*>& out ) {
    out.clear();

    std::vector<INode*> allEvents;
    get_all_particle_flow_events( allEvents );

    BOOST_FOREACH( INode* particleGroup, allEvents ) {
        if( particleGroup && accept_particle_flow_event( particleGroup ) ) {
            out.push_back( particleGroup );
        }
    }
}

void Frost::add_particle_flow_event( INode* node ) {
    if( pblock2 && accept_particle_flow_event( node ) ) {
        pblock2->Append( pb_pfEventList, 1, &node );
    }
}

void Frost::add_particle_flow_events( const std::vector<INode*>& nodeList ) {
    BOOST_FOREACH( INode* node, nodeList ) {
        add_particle_flow_event( node );
    }
}

std::size_t Frost::get_particle_flow_event_count() { return m_pfEventList.size(); }

INode* Frost::get_particle_flow_event( int i ) {
    if( i >= 0 && static_cast<std::size_t>( i ) < m_pfEventList.size() ) {
        return m_pfEventList[i].at_time( 0 );
    }
    return 0;
}

void Frost::get_particle_flow_event_names( std::vector<frantic::tstring>& eventNames ) {
    eventNames.clear();
    if( !pblock2 ) {
        return;
    }
    for( std::size_t i = 0, ie = pblock2->Count( pb_pfEventList ); i < ie; ++i ) {
        INode* node = 0;
        Interval valid;
        BOOL success = pblock2->GetValue( pb_pfEventList, GetCOREInterface()->GetTime(), node, valid, (int)i );
        if( success ) {
            if( node ) {
                eventNames.push_back( node->GetName() );
            } else {
                eventNames.push_back( _T("<deleted>") );
            }
        } else {
            eventNames.push_back( _T("<error>") );
        }
    }
}

void Frost::clear_particle_flow_events() {
    if( !pblock2 ) {
        return;
    }
    pblock2->ZeroCount( pb_pfEventList );
}

void Frost::remove_particle_flow_events( const std::vector<int>& indicesIn ) {
    if( !pblock2 ) {
        return;
    }
    std::vector<int> indexList( indicesIn.begin(), indicesIn.end() );
    std::sort( indexList.begin(), indexList.end(), std::greater<int>() );
    BOOST_FOREACH( int index, indexList ) {
        if( index >= 0 && static_cast<std::size_t>( index ) < m_pfEventList.size() ) {
            pblock2->Delete( pb_pfEventList, index, 1 );
        }
    }
}
/*
void Frost::RefreshPFEventList() {
        try {
                if( editObj == this ) {
                        IParamBlock2 * pb = GetParamBlockByID( frost_param_block );
                        if( ! pb ) {
                                return;
                        }
                        IParamMap2 * pm = pb->GetMap( frost_particle_flow_events_param_map );
                        if( ! pm ) {
                                return;
                        }
                        const HWND hwnd = pm->GetHWnd();
                        if( ! hwnd ) {
                                return;
                        }

                        particleFlowDlgProc.InvalidateUI( hwnd, pb_pfEventList );
                }
        } catch( std::exception & e ) {
                const frantic::tstring errmsg = _T("RefreshPFEventList: ") + frantic::strings::to_tstring(e.what()) +
_T("\n"); report_error( errmsg ); LogSys* log = GetCOREInterface()->Log(); log->LogEntry( SYSLOG_ERROR, NO_DIALOG,
_T("Frost Error"), _T("%s"), errmsg.c_str() ); if( is_network_render_server() ) { throw
MAXException(const_cast<TCHAR*>(errmsg.c_str()));
                }
        }
}
*/
bool Frost::has_tp_group( INode* node, ReferenceTarget* group ) {
    if( !node || !group ) {
        return false;
    }
    // TODO : check group's class
    for( std::size_t i = 0; i < m_tpGroupList.size(); ++i ) {
        if( group == m_tpGroupList[(int)i].at_time( 0 ) ) {
            return true;
        }
    }
    return false;
}

bool Frost::is_acceptable_tp_group( INode* node, ReferenceTarget* group ) {
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    if( !node || !group ) {
        return false;
    }
    if( !tp_interface::get_instance().is_node_thinking_particles( node ) ) {
        return false;
    }
    // TODO : check group's class
    for( std::size_t i = 0; i < m_tpGroupList.size(); ++i ) {
        if( group == m_tpGroupList[(int)i].at_time( 0 ) ) {
            return false;
        }
    }
    return true;
#else
    return false;
#endif
}

void Frost::get_selected_tp_groups( std::vector<tp_group_info>& outGroups ) {
    outGroups.clear();
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    if( pblock2 && tp_interface::get_instance().is_available() ) {
        TimeValue t = GetCOREInterface()->GetTime();
        std::map<ReferenceTarget*, INode*> groupToNode;
        const int nodeCount = pblock2->Count( pb_nodeList );
        const int groupCount = pblock2->Count( pb_tpGroupList );
        if( nodeCount > 0 && groupCount > 0 ) {
            std::vector<ReferenceTarget*> nodeGroups;
            for( int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex ) {
                nodeGroups.clear();
                INode* node = 0;
                Interval valid;
                BOOL success = pblock2->GetValue( pb_nodeList, t, node, valid, nodeIndex );
                if( success && node && tp_interface::get_instance().is_node_thinking_particles( node ) ) {
                    tp_interface::get_instance().get_groups( node, nodeGroups );
                    BOOST_FOREACH( ReferenceTarget* group, nodeGroups ) {
                        groupToNode[group] = node;
                    }
                }
            }
        }

        if( groupCount > 0 ) {
            outGroups.reserve( groupCount );
            for( int groupIndex = 0; groupIndex < groupCount; ++groupIndex ) {
                ReferenceTarget* group = 0;
                Interval valid;
                BOOL success = pblock2->GetValue( pb_tpGroupList, t, group, valid, groupIndex );
                if( success ) {
                    if( group ) {
                        std::map<ReferenceTarget*, INode*>::iterator i = groupToNode.find( group );
                        INode* node = 0;
                        frantic::tstring s;
                        if( i == groupToNode.end() ) {
                            s += _T("<missing>");
                        } else {
                            node = i->second;
                            if( node ) {
                                if( node->GetName() ) {
                                    s += frantic::tstring( node->GetName() );
                                } else {
                                    s += _T("<null>");
                                }
                            } else {
                                s += _T("<deleted>");
                            }
                        }

                        const frantic::tstring groupName = tp_interface::get_instance().get_group_name( group );
                        if( groupName.empty() ) {
                            s += _T("-><empty>");
                        } else {
                            s += _T("->") + groupName;
                        }

                        outGroups.push_back( tp_group_info( node, group, s ) );
                    } else {
                        outGroups.push_back( tp_group_info( 0, 0, _T("<deleted>") ) );
                    }
                } else {
                    outGroups.push_back( tp_group_info( 0, 0, _T("<error>") ) );
                }
            }
        }
    }
#endif
}

void Frost::get_all_tp_groups( std::vector<tp_group_info>& outGroups ) {
    outGroups.clear();
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    if( tp_interface::get_instance().is_available() ) {
        const int nodeCount = pblock2->Count( pb_nodeList );
        typedef std::pair<frantic::tstring, ReferenceTarget*> named_group_t;
        struct node_and_name {
            INode* node;
            frantic::tstring name;
            node_and_name( INode* node = 0, const frantic::tstring& name = _T("<none>") )
                : node( node )
                , name( name ) {}
        };
        std::vector<ReferenceTarget*> nodeGroups;
        typedef std::map<ReferenceTarget*, INode*> group_info_t;
        group_info_t groupInfo;
        for( int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex ) {
            INode* node;
            Interval valid;
            BOOL success = pblock2->GetValue( pb_nodeList, GetCOREInterface()->GetTime(), node, valid, nodeIndex );
            if( success ) {
                if( tp_interface::get_instance().is_node_thinking_particles( node ) ) {
                    nodeGroups.clear();
                    tp_interface::get_instance().get_groups( node, nodeGroups );
                    BOOST_FOREACH( ReferenceTarget* group, nodeGroups ) {
                        groupInfo[group] = node;
                    }
                }
            }
        }
        outGroups.reserve( groupInfo.size() );
        for( group_info_t::iterator i = groupInfo.begin(); i != groupInfo.end(); ++i ) {
            outGroups.push_back(
                tp_group_info( i->second, i->first, tp_interface::get_instance().get_group_name( i->first ) ) );
        }
    }
#endif
}

void Frost::get_acceptable_tp_groups( std::vector<tp_group_info>& outGroups ) {
    outGroups.clear();
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    std::vector<tp_group_info> allGroups;
    get_all_tp_groups( allGroups );

    outGroups.reserve( allGroups.size() );
    for( std::vector<tp_group_info>::iterator i = allGroups.begin(); i != allGroups.end(); ++i ) {
        if( is_acceptable_tp_group( i->node, i->group ) ) {
            outGroups.push_back( tp_group_info( i->node, i->group, i->name ) );
        }
    }
#endif
}

void Frost::add_tp_group( INode* node, ReferenceTarget* group ) {
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    if( pblock2 && is_acceptable_tp_group( node, group ) ) {
        pblock2->Append( pb_tpGroupList, 1, &group );
    }
#endif
}

void Frost::clear_tp_groups() {
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    if( !pblock2 ) {
        return;
    }
    pblock2->ZeroCount( pb_tpGroupList );
#endif
}

void Frost::remove_tp_groups( const std::vector<int>& indicesIn ) {
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    if( !pblock2 ) {
        return;
    }
    std::vector<int> indexList( indicesIn.begin(), indicesIn.end() );
    std::sort( indexList.begin(), indexList.end(), std::greater<int>() );
    BOOST_FOREACH( int index, indexList ) {
        if( index >= 0 && static_cast<std::size_t>( index ) < m_tpGroupList.size() ) {
            pblock2->Delete( pb_tpGroupList, index, 1 );
        }
    }
#endif
}

void Frost::get_tp_group_names( std::vector<frantic::tstring>& outGroupNames ) {
    // need a mapping from tpGroupList entry -> string

    outGroupNames.clear();
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )

    std::vector<tp_group_info> groupInfoList;

    get_selected_tp_groups( groupInfoList );

    outGroupNames.reserve( groupInfoList.size() );

    BOOST_FOREACH( tp_group_info& groupInfo, groupInfoList ) {
        outGroupNames.push_back( groupInfo.name );
    }
    /*
    typedef std::map<ReferenceTarget*,std::string> group_to_name_map_t;

    group_to_name_map_t groupToNameMap;

    std::vector<tp_group_info> tpGroupInfo;

    // find all tp nodes in object list
    // for each node:
    //	get groups
    const int groupCount = pblock2->Count( pb_tpGroupList );
    if( groupCount > 0 ) {
            const int particleObjectCount = pblock2->Count( pb_nodeList );
            if( particleObjectCount > 0 ) {
                    typedef std::pair<std::string,ReferenceTarget*> name_and_group_t;
                    std::vector<ReferenceTarget*> nodeParticleGroups;

                    for( int i = 0; i < particleObjectCount; ++i ) {
                            INode * node;
                            Interval valid;

                            BOOL success = pblock2->GetValue( pb_nodeList, GetCOREInterface()->GetTime(), node, valid, i
    );

                            if( success && node ) {
                                    if( tp_interface::get_instance().is_node_thinking_particles( node ) ) {
                                            nodeParticleGroups.clear();
                                            tp_interface::get_instance().get_groups( node, nodeParticleGroups );
                                            BOOST_FOREACH( ReferenceTarget * group, nodeParticleGroups ) {
                                                    std::string groupName = tp_interface::get_instance().get_group_name(
    group ); if( groupName.empty() ) { groupName = "<none>";
                                                    }
                                                    //tpGroupInfo.push_back( tp_group_info( node, group, groupName ) );
                                                    const std::string nodeName = node->GetName() ? node->GetName() :
    "<unknown>"; groupToNameMap[group] = nodeName + "->" + groupName;
                                            }
                                    }
                            }
                    }
            }

            std::vector<std::string> names( groupCount );
            for( std::size_t groupIndex = 0; groupIndex < groupCount; ++groupIndex ) {
                    ReferenceTarget * group;
                    Interval valid;

                    BOOL success = pblock2->GetValue( pb_tpGroupList, GetCOREInterface()->GetTime(), group, valid,
    groupIndex );

                    if( success ) {
                            group_to_name_map_t::iterator i = groupToNameMap.find( group );
                            if( i == groupToNameMap.end() ) {
                                    names[groupIndex] = "<missing>";
                            } else {
                                    names[groupIndex] = i->second;
                            }
                    } else {
                            names[groupIndex] = "<error>";
                    }
            }

            outGroupNames.reserve( outGroupNames.size() + names.size() );

            BOOST_FOREACH( const std::string & name, names ) {
                    outGroupNames.push_back( name );
            }
    }
    */
#endif
}
/*
void Frost::RefreshTPGroupList() {
        try {
                if( editObj == this ) {

                        IParamBlock2 * pb = GetParamBlockByID( frost_param_block );
                        if( ! pb ) {
                                return;
                        }
                        IParamMap2 * pm = pb->GetMap( frost_tp_groups_param_map );
                        if( ! pm ) {
                                return;
                        }
                        const HWND hwnd = pm->GetHWnd();
                        if( ! hwnd ) {
                                return;
                        }

                        particleFlowDlgProc.InvalidateUI( hwnd, pb_tpGroupList );
                }
        } catch( std::exception & e ) {
                const frantic::tstring errmsg = _T("RefreshTPGroupList: ") + frantic::strings::to_tstring(e.what()) +
_T("\n"); report_error( errmsg ); LogSys* log = GetCOREInterface()->Log(); log->LogEntry( SYSLOG_ERROR, NO_DIALOG,
_T("Frost Error"), _T("%s"), errmsg.c_str() ); if( is_network_render_server() ) { throw
MAXException(const_cast<TCHAR*>(errmsg.c_str()));
                }
        }
}
*/
int Frost::get_meshing_resolution_mode() {
    if( !pblock2 ) {
        return -1;
    }
    int meshingResolutionMode = 0;
    Interval valid;
    BOOL success = pblock2->GetValue( pb_meshingResolutionMode, 0, meshingResolutionMode, valid );
    if( success ) {
        return meshingResolutionMode;
    } else {
        return -1;
    }
}

void Frost::set_meshing_resolution_mode( const int meshingResolutionMode ) {
    m_meshingResolutionMode.at_time( 0 ) = meshingResolutionMode;
}

int Frost::get_geometry_type() { return m_geometryType.at_time( 0 ); }

void Frost::set_geometry_type( const int geometryType ) { m_geometryType.at_time( 0 ) = geometryType; }

bool Frost::accept_geometry( TimeValue t, INode* node ) {
    if( !node ) {
        return false;
    }
    if( !can_get_mesh_from_inode( t, node ) ) {
        return false;
    }
    node->BeginDependencyTest();
    NotifyDependents( FOREVER, 0, REFMSG_TEST_DEPENDENCY );
    if( node->EndDependencyTest() ) {
        return false;
    }
    for( std::size_t i = 0; i < m_geometryList.size(); ++i ) {
        if( node == m_geometryList[(int)i].at_time( t ) ) {
            return false;
        }
    }
    return true;
}

void Frost::add_geometry( TimeValue t, INode* node ) {
    if( pblock2 && accept_geometry( t, node ) ) {
        pblock2->Append( pb_geometryList, 1, &node );
    }
}

void Frost::remove_geometry( const std::vector<int>& indicesIn ) {
    if( !pblock2 ) {
        return;
    }
    std::vector<int> indexList( indicesIn.begin(), indicesIn.end() );
    std::sort( indexList.begin(), indexList.end(), std::greater<int>() );
    BOOST_FOREACH( int index, indexList ) {
        if( index >= 0 && index < pblock2->Count( pb_geometryList ) ) {
            pblock2->Delete( pb_geometryList, index, 1 );
        }
    }
}

void Frost::get_geometry_names( std::vector<frantic::tstring>& nodeNames ) {
    nodeNames.clear();
    if( !pblock2 ) {
        return;
    }
    for( int i = 0; i < pblock2->Count( pb_geometryList ); ++i ) {
        INode* node = 0;
        Interval valid;
        BOOL success = pblock2->GetValue( pb_geometryList, GetCOREInterface()->GetTime(), node, valid, i );
        if( success ) {
            if( node ) {
                nodeNames.push_back( node->GetName() );
            } else {
                nodeNames.push_back( _T("<deleted>") );
            }
        } else {
            nodeNames.push_back( _T("<error>") );
        }
    }
}

std::size_t Frost::get_custom_geometry_count() { return static_cast<std::size_t>( m_geometryList.size() ); }

INode* Frost::get_custom_geometry( std::size_t i ) {
    if( i < m_geometryList.size() ) {
        return m_geometryList[static_cast<int>( i )].at_time( 0 );
    } else {
        return 0;
    }
}

std::size_t Frost::get_geometry_material_id_node_count() { return m_geometryMaterialIDNodeList.size(); }

INode* Frost::get_geometry_material_id_node( std::size_t i ) {
    if( i < m_geometryMaterialIDNodeList.size() ) {
        return m_geometryMaterialIDNodeList[static_cast<int>( i )].at_time( 0 );
    } else {
        return 0;
    }
}

std::size_t Frost::get_geometry_material_id_in_count() { return m_geometryMaterialIDInList.size(); }

int Frost::get_geometry_material_id_in( std::size_t i ) {
    if( i < m_geometryMaterialIDInList.size() ) {
        return m_geometryMaterialIDInList[static_cast<int>( i )].at_time( 0 );
    } else {
        return 0;
    }
}

std::size_t Frost::get_geometry_material_id_out_count() { return m_geometryMaterialIDOutList.size(); }

int Frost::get_geometry_material_id_out( std::size_t i ) {
    if( i < m_geometryMaterialIDOutList.size() ) {
        return m_geometryMaterialIDOutList[static_cast<int>( i )].at_time( 0 );
    } else {
        return 0;
    }
}

void Frost::SetGeometryListSelection( const std::vector<int> selection ) {
    try {
        if( editObj == this ) {
            geometryDlgProc.set_geometry_list_selection( selection );
        }
    } catch( std::exception& e ) {
        const frantic::tstring errmsg =
            _T("SetGeometryListSelection: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

std::vector<int> Frost::GetGeometryListSelection( void ) {
    std::vector<int> result;
    try {
        if( editObj == this ) {
            geometryDlgProc.get_geometry_list_selection( result );
        }
    } catch( std::exception& e ) {
        const frantic::tstring errmsg =
            _T("GetGeometryListSelection: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return result;
}

const std::vector<int>& Frost::get_geometry_type_codes() { return m_geometryTypeDisplayCodes; }

frantic::tstring Frost::get_geometry_type_name( int code ) {
    std::map<int, frantic::tstring>::iterator i = m_geometryTypeNames.find( code );
    if( i == m_geometryTypeNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return i->second;
    }
}

int Frost::get_geometry_selection_mode() { return m_geometrySelectionMode.at_time( 0 ); }

void Frost::set_geometry_selection_mode( const int s ) { m_geometrySelectionMode.at_time( 0 ) = s; }

const std::vector<int>& Frost::get_geometry_selection_mode_codes() { return m_geometrySelectionModeDisplayCodes; }

frantic::tstring Frost::get_geometry_selection_mode_name( int code ) {
    std::map<int, frantic::tstring>::iterator i = m_geometrySelectionModeNames.find( code );
    if( i == m_geometrySelectionModeNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return i->second;
    }
}

int Frost::get_geometry_timing_base_mode() { return m_geometrySampleTimeBaseMode.at_time( 0 ); }

void Frost::set_geometry_timing_base_mode( const int s ) { m_geometrySampleTimeBaseMode.at_time( 0 ) = s; }

const std::vector<int>& Frost::get_geometry_timing_base_mode_codes() { return m_geometryTimingBaseModeDisplayCodes; }

frantic::tstring Frost::get_geometry_timing_base_mode_name( int code ) {
    std::map<int, frantic::tstring>::iterator i = m_geometryTimingBaseModeNames.find( code );
    if( i == m_geometryTimingBaseModeNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return i->second;
    }
}

int Frost::get_geometry_timing_offset_mode() { return m_geometrySampleTimeOffsetMode.at_time( 0 ); }

void Frost::set_geometry_timing_offset_mode( const int s ) { m_geometrySampleTimeOffsetMode.at_time( 0 ) = s; }

const std::vector<int>& Frost::get_geometry_timing_offset_mode_codes() {
    return m_geometryTimingOffsetModeDisplayCodes;
}

frantic::tstring Frost::get_geometry_timing_offset_mode_name( int code ) {
    std::map<int, frantic::tstring>::iterator i = m_geometryTimingOffsetModeNames.find( code );
    if( i == m_geometryTimingOffsetModeNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return i->second;
    }
}

int Frost::get_geometry_orientation_mode() { return m_geometryOrientationMode.at_time( 0 ); }

void Frost::set_geometry_orientation_mode( const int s ) { m_geometryOrientationMode.at_time( 0 ) = s; }

const std::vector<int>& Frost::get_geometry_orientation_mode_codes() { return m_geometryOrientationModeDisplayCodes; }

frantic::tstring Frost::get_geometry_orientation_mode_name( int code ) {
    std::map<int, frantic::tstring>::iterator i = m_geometryOrientationModeNames.find( code );
    if( i == m_geometryOrientationModeNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return i->second;
    }
}

void Frost::invalidate_vector_channel_name_cache() { m_isVectorChannelNameCacheDirty = true; }

void Frost::get_vector_channel_names( std::vector<frantic::tstring>& outChannelNames, TimeValue t ) {
    outChannelNames.clear();

    try {
        if( m_isVectorChannelNameCacheDirty ) {
            m_vectorChannelNameCache.clear();

            if( ( m_fileList.size() > 0 || has_scene_particle_source( t ) ) ) {
                boost::shared_ptr<frantic::particles::streams::particle_istream> pin =
                    GetParticleIStream( t, get_default_camera() );
                ::get_vector_channel_names( pin, m_vectorChannelNameCache );
            }

            m_isVectorChannelNameCacheDirty = false;
        }
        std::copy( m_vectorChannelNameCache.begin(), m_vectorChannelNameCache.end(),
                   std::back_insert_iterator<std::vector<frantic::tstring>>( outChannelNames ) );
    } catch( const std::exception& e ) {
        const frantic::tstring errmsg =
            _T("GetVectorChannelsFromFiles: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        clear_mesh( mesh );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

frantic::tstring Frost::get_geometry_orientation_vector_channel() {
    if( pblock2 ) {
        return m_geometryOrientationVectorChannel.at_time( 0 );
    } else {
        return _T("");
    }
}

void Frost::set_geometry_orientation_vector_channel( const frantic::tstring& s ) {
    if( pblock2 ) {
        m_geometryOrientationVectorChannel.at_time( 0 ) = s;
    }
}

int Frost::get_divergence_axis_space() { return m_geometryOrientationDivergenceAxisSpace.at_time( 0 ); }

void Frost::set_divergence_axis_space( const int s ) { m_geometryOrientationDivergenceAxisSpace.at_time( 0 ) = s; }

const std::vector<int>& Frost::get_divergence_axis_space_codes() {
    return m_orientationDivergenceAxisSpaceDisplayCodes;
}

frantic::tstring Frost::get_divergence_axis_space_name( int code ) {
    std::map<int, frantic::tstring>::iterator i = m_orientationDivergenceAxisSpaceNames.find( code );
    if( i == m_orientationDivergenceAxisSpaceNames.end() ) {
        return _T("<unknown:") + boost::lexical_cast<frantic::tstring>( code ) + _T(">");
    } else {
        return i->second;
    }
}

void Frost::set_geometry_orientation( const frantic::graphics::vector3f& orientation, TimeValue t ) {
    m_geometryOrientationX.at_time( t ) = orientation.x;
    m_geometryOrientationY.at_time( t ) = orientation.y;
    m_geometryOrientationZ.at_time( t ) = orientation.z;
}

void Frost::set_geometry_orientation_from_node( INode* node, TimeValue t ) {
    if( node ) {
        try {
            Matrix3 tm = node->GetObjTMAfterWSM( t );
            frantic::graphics::vector3f eulerAngles = extract_euler_xyz( tm );
            set_geometry_orientation( vector3f( radians_to_degrees( eulerAngles[0] ),
                                                radians_to_degrees( eulerAngles[1] ),
                                                radians_to_degrees( eulerAngles[2] ) ),
                                      t );
        } catch( std::exception& e ) {
            report_warning( _T("Could not get orientation from node.  Error: ") +
                            frantic::strings::to_tstring( e.what() ) );
        }
    }
}

void Frost::set_divergence_axis( const frantic::graphics::vector3f& axis, TimeValue t ) {
    m_geometryOrientationDivergenceAxisX.at_time( t ) = axis.x;
    m_geometryOrientationDivergenceAxisY.at_time( t ) = axis.y;
    m_geometryOrientationDivergenceAxisZ.at_time( t ) = axis.z;
}

void Frost::set_divergence_axis_from_node_z( INode* node, TimeValue t ) {
    if( node ) {
        try {
            Value* val = frantic::max3d::mxs::expression( _T("theNode.dir" ) )
                             .bind( _T("theNode"), node )
                             .at_time( t )
                             .evaluate<Value*>();
            if( val && is_point3( val ) ) {
                Point3 direction = val->to_point3();
                set_divergence_axis( vector3f( -direction.x, -direction.y, -direction.z ), t );
            }
        } catch( std::exception& e ) {
            report_warning( _T("Could not get axis from node.  Error: ") + frantic::strings::to_tstring( e.what() ) );
        }
    }
}

bool Frost::accept_orientation_look_at_node( INode* node ) {
    if( !node ) {
        return true;
    }

    node->BeginDependencyTest();
    NotifyDependents( FOREVER, 0, REFMSG_TEST_DEPENDENCY );
    if( node->EndDependencyTest() ) {
        return false;
    } else {
        return true;
    }
}

void Frost::set_orientation_look_at_node( INode* node ) {
    if( accept_orientation_look_at_node( node ) ) {
        m_geometryOrientationLookAtNode.at_time( 0 ) = node;
    }
}

INode* Frost::get_orientation_look_at_node() {
    if( pblock2 ) {
        return m_geometryOrientationLookAtNode.at_time( 0 );
    } else {
        return 0;
    }
}

Matrix3 Frost::get_target_tm( TimeValue t ) {
    Matrix3 result;
    result.IdentityMatrix();
    if( pblock2 ) {
        INode* node = m_geometryOrientationLookAtNode.at_time( t );
        if( node ) {
            result = node->GetObjTMAfterWSM( t );
        }
    }
    return result;
}

Matrix3 Frost::get_cached_target_tm() { return m_cachedTargetTm; }

frantic::graphics::vector3f Frost::get_orientation_look_at_pos( TimeValue t ) {
    frantic::graphics::vector3f lookAtPos( 0 );
    INode* lookAtNode = m_geometryOrientationLookAtNode.at_time( t );
    if( lookAtNode ) {
        lookAtPos = from_max_t( lookAtNode->GetObjTMAfterWSM( t ).GetTrans() );
    }
    return lookAtPos;
}

void Frost::update_ui_to_meshing_method( const int oldMeshingMethod ) {
    if( m_meshingMethod.at_time( 0 ) == oldMeshingMethod ) {
        return;
    }

    param_map_info qualityPMI;
    get_quality_param_map_info( qualityPMI );
    HWND hwndCurrentQuality = 0;
    if( m_qualityParamMap ) {
        hwndCurrentQuality = m_qualityParamMap->GetHWnd();
        m_rsQuality = IsRollupPanelOpen( hwndCurrentQuality ) ? false : true;

        DestroyCPParamMap2( m_qualityParamMap );
        m_qualityParamMap = 0;
        hwndCurrentQuality = 0;
    }
    if( qualityPMI.dlgProc ) {
        m_qualityParamMap =
            CreateCPParamMap2( qualityPMI.paramMap, pblock2, ip, ghInstance, MAKEINTRESOURCE( qualityPMI.dialogId ),
                               GetString( qualityPMI.dialogStringId ), m_rsQuality ? APPENDROLL_CLOSED : 0,
                               qualityPMI.dlgProc, hwndCurrentQuality /*, ROLLOUT_CAT_CUSTOMATTRIB*/ );
    } else {
        m_qualityParamMap = 0;
    }

    param_map_info mesherPMI;
    get_mesher_param_map_info( mesherPMI );
    HWND hwndCurrentMesher = 0;
    if( m_mesherParamMap ) {
        hwndCurrentMesher = m_mesherParamMap->GetHWnd();
        m_rsMesher = IsRollupPanelOpen( hwndCurrentMesher ) ? false : true;

        DestroyCPParamMap2( m_mesherParamMap );
        m_mesherParamMap = 0;
        hwndCurrentMesher = 0;
    }
    if( mesherPMI.dlgProc ) {
        m_mesherParamMap =
            CreateCPParamMap2( mesherPMI.paramMap, pblock2, ip, ghInstance, MAKEINTRESOURCE( mesherPMI.dialogId ),
                               GetString( mesherPMI.dialogStringId ), m_rsMesher ? APPENDROLL_CLOSED : 0,
                               mesherPMI.dlgProc, hwndCurrentMesher /*, ROLLOUT_CAT_CUSTOMATTRIB*/ );
    } else {
        m_mesherParamMap = 0;
    }
}

void Frost::get_mesher_param_map_info( param_map_info& out ) {
    out = param_map_info();
    const int meshingMethod = m_meshingMethod.at_time( 0 );
    std::map<int, param_map_info>::const_iterator i = m_mesherParamMapInfo.find( meshingMethod );
    if( i != m_mesherParamMapInfo.end() ) {
        out = i->second;
    }
}

void Frost::get_quality_param_map_info( param_map_info& out ) {
    out = param_map_info();
    const int meshingMethod = m_meshingMethod.at_time( 0 );
    std::map<int, param_map_info>::const_iterator i = m_qualityParamMapInfo.find( meshingMethod );
    if( i != m_qualityParamMapInfo.end() ) {
        out = i->second;
    }
}

bool Frost::get_enable_render_roi() { return m_enableRenderROI.at_time( 0 ); }

void Frost::set_enable_render_roi( bool enable ) { m_enableRenderROI.at_time( 0 ) = enable; }

bool Frost::get_enable_viewport_roi() { return m_enableViewportROI.at_time( 0 ); }

void Frost::set_enable_viewport_roi( bool enable ) { m_enableViewportROI.at_time( 0 ) = enable; }

frantic::graphics::boundbox3f Frost::get_raw_roi_box( TimeValue t ) {
    using namespace frantic::graphics;

    float xCenter = m_roiCenterX.at_time( t );
    float yCenter = m_roiCenterY.at_time( t );
    float zCenter = m_roiCenterZ.at_time( t );
    float sizeX = m_roiSizeX.at_time( t );
    float sizeY = m_roiSizeY.at_time( t );
    float sizeZ = m_roiSizeZ.at_time( t );

    const vector3f center( xCenter, yCenter, zCenter );
    const vector3f size( sizeX, sizeY, sizeZ );

    const vector3f bottomCorner = center - size / 2;
    const vector3f topCorner = center + size / 2;

    return boundbox3f( bottomCorner, topCorner );
}

boost::optional<frantic::graphics::boundbox3f> Frost::get_display_roi_box( TimeValue t ) {
    if( m_enableRenderROI.at_time( t ) || m_enableViewportROI.at_time( t ) ) {
        return get_raw_roi_box( t );
    }

    return boost::optional<boundbox3f>();
}

boost::optional<frantic::graphics::boundbox3f> Frost::get_roi_box( TimeValue t ) {
    if( m_inRenderingMode && m_enableRenderROI.at_time( t ) ||
        !m_inRenderingMode && m_enableViewportROI.at_time( t ) ) {
        return get_raw_roi_box( t );
    }

    return boost::optional<boundbox3f>();
}

Mesh* Frost::get_icon_mesh( TimeValue t ) {
    Mesh* pIconMesh = 0;
    if( m_showIcon.at_time( t ) ) {
        pIconMesh = g_pIconMesh.get();
    }

#if MAX_VERSION_MAJOR >= 14
/*
        if( pIconMesh ) {
                const float scale = m_iconSize.at_time( t );
                if( m_cachedIconMeshScale != scale || m_cachedIconMesh.numVerts == 0 ) {
                        m_cachedIconMesh = ( * pIconMesh );

                        if( m_cachedIconMesh.verts ) {
                                for( int i = 0; i < m_cachedIconMesh.numVerts; ++i ) {
                                        m_cachedIconMesh.verts[i] *= scale;
                                }
                        }

                        m_cachedIconMesh.InvalidateGeomCache();
                        m_cachedIconMesh.InvalidateTopologyCache();

                        m_cachedIconMeshScale = scale;
                }
                pIconMesh = & m_cachedIconMesh;
        }
*/
#endif

    return pIconMesh;
}

bool Frost::use_filled_icon( TimeValue t ) {
#if MAX_VERSION_MAJOR >= 14
    if( MaxSDK::Graphics::IsRetainedModeEnabled() ) {
        return false;
    }
#endif
    return ( m_iconMode.at_time( t ) == ICON_MODE::FILLED );
}

frost_status_code_t Frost::get_parameter_status_code( ParamID id, TimeValue t ) {
    switch( id ) {
    case pb_materialMode: {
        const int materialMode = m_materialMode.at_time( t );
        const int meshingMethod = m_meshingMethod.at_time( t );
        switch( materialMode ) {
        case MATERIAL_MODE::SINGLE:
            return frost_status_code::ok;
        case MATERIAL_MODE::MTLINDEX_CHANNEL: {
            fill_channel_map_cache( t );
            if( m_cachedNativeChannelMaps.size() == 0 ) {
                return frost_status_code::no_particles_found;
            }
            std::size_t positive = 0;
            for( std::size_t i = 0; i < m_cachedNativeChannelMaps.size(); ++i ) {
                if( m_cachedNativeChannelMaps[i].has_channel( Frost_PARTICLE_MTLINDEX_CHANNEL_NAME ) ) {
                    ++positive;
                }
            }
            if( positive == 0 ) {
                return frost_status_code::no_mtlindex_channel;
            } else if( positive == m_cachedNativeChannelMaps.size() ) {
                return frost_status_code::mtlindex_channel_ok;
            } else {
                return frost_status_code::partial_mtlindex_channel;
            }
        } break;
        case MATERIAL_MODE::SHAPE_NUMBER:
        case MATERIAL_MODE::MATERIAL_ID_FROM_GEOMETRY:
        case MATERIAL_MODE::MATERIAL_FROM_GEOMETRY:
            if( meshingMethod == MESHING_METHOD::GEOMETRY ) {
                return frost_status_code::ok;
            } else {
                return frost_status_code::unsupported_material_mode;
            }
            break;
        }
    } break;
    case pb_useRadiusChannel: {
        fill_channel_map_cache( t );
        if( m_cachedNativeChannelMaps.size() == 0 ) {
            return frost_status_code::no_particles_found;
        }
        std::size_t positive = 0;
        for( std::size_t i = 0; i < m_cachedNativeChannelMaps.size(); ++i ) {
            if( m_cachedNativeChannelMaps[i].has_channel( Frost_RADIUS ) ) {
                ++positive;
            }
        }
        if( positive == 0 ) {
            return frost_status_code::no_radius_channel;
        } else if( positive == m_cachedNativeChannelMaps.size() ) {
            return frost_status_code::radius_channel_ok;
        } else {
            return frost_status_code::partial_radius_channel;
        }
    } break;
    case pb_radiusRandomVariation: {
        fill_channel_map_cache( t );
        if( m_cachedNativeChannelMaps.size() == 0 ) {
            return frost_status_code::no_particles_found;
        }
        std::size_t positive = 0;
        for( std::size_t i = 0; i < m_cachedNativeChannelMaps.size(); ++i ) {
            if( m_cachedNativeChannelMaps[i].has_channel( Frost_IDCHANNEL ) ) {
                ++positive;
            }
        }
        if( positive == 0 ) {
            return frost_status_code::no_id_channel;
        } else if( positive == m_cachedNativeChannelMaps.size() ) {
            return frost_status_code::id_channel_ok;
        } else {
            return frost_status_code::partial_id_channel;
        }
    } break;
    case pb_radiusAnimationMode: {
        const int enableRadiusScale = m_enableRadiusScale.at_time( t );
        if( !enableRadiusScale ) {
            return frost_status_code::ok;
        }
        const int radiusAnimationMode = m_radiusAnimationMode.at_time( t );
        if( radiusAnimationMode == RADIUS_ANIMATION_MODE::ABSOLUTE_TIME ) {
            return frost_status_code::ok;
        } else if( radiusAnimationMode == RADIUS_ANIMATION_MODE::AGE ||
                   radiusAnimationMode == RADIUS_ANIMATION_MODE::LIFE_PERCENT ) {
            fill_channel_map_cache( t );
            if( m_cachedNativeChannelMaps.size() == 0 ) {
                return frost_status_code::no_particles_found;
            }
            std::size_t agePositive = 0;
            std::size_t lifespanPositive = 0;
            const std::size_t full = m_cachedNativeChannelMaps.size();
            for( std::size_t i = 0; i < m_cachedNativeChannelMaps.size(); ++i ) {
                if( m_cachedNativeChannelMaps[i].has_channel( Frost_AGE_CHANNEL_NAME ) ) {
                    ++agePositive;
                }
                if( m_cachedNativeChannelMaps[i].has_channel( Frost_LIFESPAN_CHANNEL_NAME ) ) {
                    ++lifespanPositive;
                }
            }

            if( radiusAnimationMode == RADIUS_ANIMATION_MODE::AGE ) {
                if( agePositive == 0 ) {
                    return frost_status_code::no_age_channel;
                } else if( agePositive == full ) {
                    return frost_status_code::ok;
                } else {
                    return frost_status_code::partial_age_channel;
                }
            } else if( radiusAnimationMode == RADIUS_ANIMATION_MODE::LIFE_PERCENT ) {
                if( agePositive == 0 ) {
                    return frost_status_code::no_age_channel;
                } else if( agePositive == full ) {
                    if( lifespanPositive == 0 ) {
                        return frost_status_code::no_lifespan_channel;
                    } else if( lifespanPositive == full ) {
                        return frost_status_code::ok;
                    } else {
                        return frost_status_code::partial_lifespan_channel;
                    }
                } else { // partial age
                    if( lifespanPositive == 0 ) {
                        return frost_status_code::no_lifespan_channel;
                    } else if( lifespanPositive == full ) {
                        return frost_status_code::partial_age_channel;
                    } else { // partial life
                        return frost_status_code::partial_age_channel;
                    }
                }
            }
        } else {
            throw std::runtime_error( "get_parameter_status_code Error: unrecognized radiusAnimationMode: " +
                                      boost::lexical_cast<std::string>( radiusAnimationMode ) );
        }
    } break;
    }
    return frost_status_code::access_failed;
}

frantic::tstring Frost::get_status_code_string( frost_status_code_t msg ) {
    status_code_info_map_t::iterator i = m_statusCodeInfo.find( msg );
    if( i == m_statusCodeInfo.end() ) {
        return _T("<MISSING KEY>");
    } else {
        return i->second.msg;
    }
}

int Frost::get_status_code_weight( frost_status_code_t msg ) {
    status_code_info_map_t::iterator i = m_statusCodeInfo.find( msg );
    if( i == m_statusCodeInfo.end() ) {
        return 0;
    } else {
        return i->second.weight;
    }
}

int Frost::get_status_code_lamp( frost_status_code_t msg ) {
    status_code_info_map_t::iterator i = m_statusCodeInfo.find( msg );
    if( i == m_statusCodeInfo.end() ) {
        return 0;
    } else {
        return i->second.color;
    }
}

// Returns the INode of the first Frost node
INode* Frost::get_inode() {
    INode* inode = 0;

    RefTargetHandle target = this; // m_parent.get_target()
    INode* rootNode = GetCOREInterface()->GetRootNode();

    // Try using the cache.
    // TODO: what if you have multiple instanced Frost nodes, and you
    // delete one of them?  I think the cached INode* may be invalid.
    if( m_thisNode ) {
        inode = m_thisNode;
    }

    // try finding the node in the scene
    if( !inode ) {
        inode = frantic::max3d::get_inode( rootNode, target );
    }

    // try finding the node in xref'ed scenes
    if( !inode ) {
        const int xrefFileCount = rootNode->GetXRefFileCount();
        for( int i = 0; i < xrefFileCount && !inode; ++i ) {
            INode* xrefRootNode = rootNode->GetXRefTree( i );
            if( xrefRootNode ) {
                inode = frantic::max3d::get_inode( xrefRootNode, target );
            }
        }
    }

    if( !inode ) {
        FF_LOG( debug ) << "get_inode: Could not find a match for myself in the scene." << std::endl;
    }

    return inode;
}

boost::shared_ptr<frost::geometry_meshing_parameters_interface>
Frost::create_geometry_meshing_parameters( TimeValue t ) {
    boost::shared_ptr<frost::max3d::max3d_geometry_meshing_parameters> params(
        new frost::max3d::max3d_geometry_meshing_parameters( this, t ) );
    return params;
}

bool Frost::HasAnyParticleSource( TimeValue t ) { return m_fileList.size() > 0 || has_scene_particle_source( t ); }

namespace {

float get_pblock2_float_at_seconds( float timeSeconds, IParamBlock2* pblock, ParamID id ) {
    if( pblock ) {
        float value = 0;
        Interval iv;
        BOOL success = pblock->GetValue( id, SecToTicks( timeSeconds ), value, iv );
        // getting rid of Empty check because it fails if the parameter is unanimated
        // TODO: something better?
        if( success /*&& ! iv.Empty()*/ ) {
            return value;
        }
    }
    return 0;
}

float get_life_percent_seconds( float ageSeconds, float lifespanSeconds, float defaultLifespan, float fullValue ) {
    if( lifespanSeconds <= 0 ) {
        lifespanSeconds = defaultLifespan;
    }
    return frantic::math::clamp( ( ageSeconds / lifespanSeconds ) * fullValue, 0.f, fullValue );
}

} // anonymous namespace

// This creates a particle istream for a PRT mesher/loader node at a tick time t.
boost::shared_ptr<particle_istream> Frost::GetParticleIStream( TimeValue t,
                                                               const frantic::graphics::camera<float>& camera ) {
    TIMESTEP = GetTicksPerFrame() / 4;

    std::vector<boost::shared_ptr<frantic::particles::streams::particle_istream>> pins;
    std::vector<frantic::tstring> pinNames;

    // boost::shared_ptr<set_render_in_scope_collection> setRenderInScopeCollection( new set_render_in_scope_collection(
    // t ) );

    get_particle_streams( t, camera, pins, pinNames );

    if( pins.size() != pinNames.size() ) {
        throw std::runtime_error( "GetParticleIStream: Internal Error: mismatch between streams and names." );
    }

    const float defaultRadius = m_radius.at_time( t );
    const bool useFileRadius = m_useRadiusChannel.at_time( t );
    const int meshingMethod = m_meshingMethod.at_time( t );

    for( std::size_t i = 0; i < pins.size(); ++i ) {
        frantic::channels::channel_map pcm = pins[i]->get_channel_map();

        // use the defaultRadius if the stream has no radius channel
        if( !useFileRadius || !pcm.has_channel( Frost_RADIUS ) ) {
            pins[i].reset( new streams::set_channel_particle_istream<float>( pins[i], Frost_RADIUS, defaultRadius ) );
            set_channel_map_to_native_channel_map( pins[i] );
            pcm = pins[i]->get_channel_map();
        }

        // randomize radius
        if( m_randomizeRadius.at_time( t ) ) {
            float randomVariation = get_radius_random_variation_fraction( t );
            boost::uint32_t randomSeed = static_cast<boost::uint32_t>( m_radiusRandomSeed.at_time( t ) );
            if( !pcm.has_channel( Frost_IDCHANNEL ) ) {
                pcm.append_channel<boost::int32_t>( Frost_IDCHANNEL );
                pins[i]->set_channel_map( pcm );
                pcm = pins[i]->get_channel_map();
                pins[i].reset( new create_id_channel_from_index_particle_istream( pins[i], Frost_IDCHANNEL ) );
            }
            static boost::array<frantic::tstring, 2> inputParamNames = { Frost_IDCHANNEL, Frost_RADIUS };
            pins[i].reset( new streams::apply_function_particle_istream<float( const boost::int32_t&, const float& )>(
                pins[i], boost::bind( &id_randomized_radius, _1, _2, randomVariation, randomSeed ), Frost_RADIUS,
                inputParamNames ) );
            set_channel_map_to_native_channel_map( pins[i] );
            pcm = pins[i]->get_channel_map();
        }

        // radius scale animation
        const bool enableRadiusScale = m_enableRadiusScale.at_time( t );
        if( enableRadiusScale ) {
            const int radiusAnimationMode = m_radiusAnimationMode.at_time( t );
            if( radiusAnimationMode == RADIUS_ANIMATION_MODE::ABSOLUTE_TIME ) {
                const float scale = m_radiusScale.at_time( t );
                static boost::array<frantic::tstring, 1> inputParamNames = { Frost_RADIUS };
                pins[i].reset( new streams::apply_function_particle_istream<float( const float& )>(
                    pins[i], std::bind2nd( std::multiplies<float>(), scale ), Frost_RADIUS, inputParamNames ) );
            } else if( radiusAnimationMode == RADIUS_ANIMATION_MODE::AGE ) {
                const float defaultAge = 0.f;

                if( !pcm.has_channel( Frost_AGE_CHANNEL_NAME ) ) {
                    pins[i].reset( new streams::set_channel_particle_istream<float>( pins[i], Frost_AGE_CHANNEL_NAME,
                                                                                     defaultAge ) );
                    set_channel_map_to_native_channel_map( pins[i] );
                    pcm = pins[i]->get_channel_map();
                }

                static boost::array<frantic::tstring, 2> inputParamNames = { Frost_RADIUS, Frost_AGE_CHANNEL_NAME };
                pins[i].reset( new streams::apply_function_particle_istream_nothreads<float( float, float )>(
                    pins[i],
                    boost::bind( std::multiplies<float>(), _1,
                                 boost::bind( &get_pblock2_float_at_seconds, _2, pblock2, (ParamID)pb_radiusScale ) ),
                    Frost_RADIUS, inputParamNames ) );
                set_channel_map_to_native_channel_map( pins[i] );
                pcm = pins[i]->get_channel_map();
            } else if( radiusAnimationMode == RADIUS_ANIMATION_MODE::LIFE_PERCENT ) {
                const float defaultAge = 0;
                const float defaultLifespan = 1.f;
                if( !pcm.has_channel( Frost_AGE_CHANNEL_NAME ) ) {
                    pins[i].reset( new streams::set_channel_particle_istream<float>( pins[i], Frost_AGE_CHANNEL_NAME,
                                                                                     defaultAge ) );
                    set_channel_map_to_native_channel_map( pins[i] );
                    pcm = pins[i]->get_channel_map();
                }

                if( !pcm.has_channel( Frost_LIFESPAN_CHANNEL_NAME ) ) {
                    pins[i].reset( new streams::set_channel_particle_istream<float>(
                        pins[i], Frost_LIFESPAN_CHANNEL_NAME, defaultLifespan ) );
                    set_channel_map_to_native_channel_map( pins[i] );
                    pcm = pins[i]->get_channel_map();
                }

                static boost::array<frantic::tstring, 3> inputParamNames = { Frost_RADIUS, Frost_AGE_CHANNEL_NAME,
                                                                             Frost_LIFESPAN_CHANNEL_NAME };
                pins[i].reset( new streams::apply_function_particle_istream_nothreads<float( float, float, float )>(
                    pins[i],
                    boost::bind( std::multiplies<float>(), _1,
                                 boost::bind( &get_pblock2_float_at_seconds,
                                              boost::bind( &get_life_percent_seconds, _2, _3, defaultLifespan,
                                                           100.f / static_cast<float>( GetFrameRate() ) ),
                                              pblock2, (ParamID)pb_radiusScale ) ),
                    Frost_RADIUS, inputParamNames ) );
                set_channel_map_to_native_channel_map( pins[i] );
                pcm = pins[i]->get_channel_map();
            } else {
                throw std::runtime_error( "Unrecognized Radius Animation Mode: " +
                                          boost::lexical_cast<std::string>( radiusAnimationMode ) );
            }
        }

        if( meshingMethod == MESHING_METHOD::GEOMETRY ) {
            if( !pcm.has_channel( Frost_IDCHANNEL ) ) {
                pcm.append_channel<boost::int32_t>( Frost_IDCHANNEL );
                pins[i]->set_channel_map( pcm );
                pcm = pins[i]->get_channel_map();
                pins[i].reset( new create_id_channel_from_index_particle_istream( pins[i], Frost_IDCHANNEL ) );
            }
        }

        // Force the channel map to have all the native channels (The Frost_RADIUS channel is just added to the native
        // map above.)
        set_channel_map_to_native_channel_map( pins[i] );
    }

    boost::shared_ptr<particle_istream> resultStream;

    if( pins.size() == 0 ) {
        throw std::runtime_error( "GetParticleIStream: No particle source at time " +
                                  boost::lexical_cast<std::string>( t ) + "." );
    } else if( pins.size() == 1 ) {
        resultStream = pins[0];
    } else {
        resultStream.reset( new concatenated_particle_istream( pins ) );
        set_channel_map_to_native_channel_map( resultStream );
    }

    // if( ! setRenderInScopeCollection.empty() ) {
    // resultStream.reset( new hold_render_particle_istream( resultStream, setRenderInScopeCollection ) );
    //}

    return resultStream;
}

bool Frost::has_scene_particle_source( TimeValue t ) {
    for( std::size_t i = 0; i < m_nodeList.size(); ++i ) {
        if( m_nodeList[(int)i].at_time( t ) ) {
            return true;
        }
    }
    return false;
}

template <class CacheVector>
void clear_stale_cache( CacheVector& cacheVector ) {
    BOOST_FOREACH( typename CacheVector::value_type& cache, cacheVector ) {
        cache.clear_if_old();
    }
}

template <class CacheVector>
void set_stale_cache( CacheVector& cacheVector ) {
    BOOST_FOREACH( typename CacheVector::value_type& cache, cacheVector ) {
        cache.inc_age();
    }
}

void Frost::get_particle_streams( TimeValue t, const frantic::graphics::camera<float>& camera,
                                  std::vector<boost::shared_ptr<frantic::particles::streams::particle_istream>>& pins,
                                  std::vector<frantic::tstring>& pinNames, bool load ) {
    pins.clear();
    pinNames.clear();

    TIMESTEP = GetTicksPerFrame() / 4;

    m_cachedNativeChannelMaps.clear();
    m_cachedNativeChannelMapsTime = t;
    m_gizmoCounter = 0;

    bool useRenderParticles = m_inRenderingMode || m_viewRenderParticles.at_time( t );

    bool forceRender = useRenderParticles && !m_inRenderingMode;

    set_render_in_scope_collection setRenderInScopeCollection( t );

    double loadFraction = 0;
    int loadMode = VIEWPORT_LOAD_MODE::HEAD;
    if( m_inRenderingMode ) {
        loadFraction = 1.0;
        loadMode = VIEWPORT_LOAD_MODE::HEAD;
    } else {
        loadMode = m_viewportLoadMode.at_time( t );
        const float viewportLoadPercent = frantic::math::clamp<float>( m_viewportLoadPercent.at_time( t ), 0.f, 100.f );
        if( viewportLoadPercent < 100.f ) {
            loadFraction = 0.01 * viewportLoadPercent;
        } else {
            loadFraction = 1.0;
        }
    }

    clear_stale_cache( m_nodeParticleCache );
    set_stale_cache( m_nodeParticleCache );

    clear_stale_cache( m_fileParticleCache );
    set_stale_cache( m_fileParticleCache );

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////
    ///////			Get Streams From Nodes
    ///////
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    m_nodeParticleCache.resize( m_nodeList.size() );

    for( std::size_t i = 0; i < m_nodeList.size(); ++i ) {
        INode* prtNode = m_nodeList[(int)i].at_time( t );
        if( prtNode ) {
            if( load ) {
                if( !m_nodeParticleCache[i].is_current( prtNode, m_inRenderingMode, useRenderParticles,
                                                        Interval( t, t ), loadFraction, loadMode ) ) {
                    if( forceRender ) {
                        std::set<ReferenceMaker*> doneNodes;
                        frantic::max3d::rendering::refmaker_call_recursive(
                            prtNode, doneNodes,
                            boost::function<void( Animatable* )>( boost::bind( &set_render_in_scope_collection::insert,
                                                                               boost::ref( setRenderInScopeCollection ),
                                                                               _1 ) ) );
                    }
                    boost::shared_ptr<frantic::particles::streams::particle_istream> nodeStream =
                        get_scene_object_particle_istream( t, prtNode, camera, forceRender );
                    if( nodeStream ) {
                        m_nodeParticleCache[i].set( nodeStream, prtNode, m_inRenderingMode, useRenderParticles,
                                                    Interval( t, t ), loadFraction, loadMode );
                    } else {
                        throw std::runtime_error( "Could not get particle_istream from node: " +
                                                  std::string( frantic::strings::to_string( prtNode->GetName() ) ) );
                    }
                }
            } else {
                if( !m_nodeParticleCache[i].is_current_channel_map( prtNode, m_inRenderingMode, useRenderParticles,
                                                                    Interval( t, t ) ) ) {
                    if( forceRender ) {
                        std::set<ReferenceMaker*> doneNodes;
                        frantic::max3d::rendering::refmaker_call_recursive(
                            prtNode, doneNodes,
                            boost::function<void( Animatable* )>( boost::bind( &set_render_in_scope_collection::insert,
                                                                               boost::ref( setRenderInScopeCollection ),
                                                                               _1 ) ) );
                    }
                    boost::shared_ptr<frantic::particles::streams::particle_istream> nodeStream =
                        get_scene_object_particle_istream( t, prtNode, camera, forceRender );
                    if( nodeStream ) {
                        m_nodeParticleCache[i].set_no_load( nodeStream, prtNode, m_inRenderingMode, useRenderParticles,
                                                            Interval( t, t ) );
                    } else {
                        throw std::runtime_error( "Could not get particle_istream from node: " +
                                                  std::string( frantic::strings::to_string( prtNode->GetName() ) ) );
                    }
                }
            }
            boost::shared_ptr<frantic::particles::streams::particle_istream> stream(
                new frantic::particles::streams::particle_array_particle_istream(
                    m_nodeParticleCache[i].get_particles_ref() ) );
            pins.push_back( stream );
            pinNames.push_back( _T("node: ") + frantic::tstring( prtNode->GetName() ) );
            m_cachedNativeChannelMaps.push_back( stream->get_channel_map() );
        } else {
            m_nodeParticleCache[i].clear_and_deallocate();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////
    ///////			Get Streams From Files
    ///////
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    // time derivative info
    float timeDerivative = 1.f;
    double frame = double( t ) / GetTicksPerFrame();
    bool useBlankFrame = false;
    bool useStaticFrame = false;

    if( m_loadSingleFrame.at_time( t ) ) {
        useStaticFrame = true;
    } else {
        if( m_enablePlaybackGraph.at_time( t ) ) {
            frame = m_playbackGraphTime.at_time( t );
            float intervalStart = m_playbackGraphTime.at_time( t - ( TIMESTEP / 2 ) );
            float intervalEnd = m_playbackGraphTime.at_time( t + ( TIMESTEP / 2 ) );
            timeDerivative = ( intervalEnd - intervalStart ) / TIMESTEP * GetTicksPerFrame();
        }

        frame += m_frameOffset.at_time( t );

        if( m_limitToRange.at_time( t ) ) {
            double first = m_rangeFirstFrame.at_time( t );
            double last = m_rangeLastFrame.at_time( t );

            if( first == last ) {
                useStaticFrame = true;
            }

            if( frame < first ) {
                const int beforeRangeBehavior = m_beforeRangeBehavior.at_time( t );
                switch( beforeRangeBehavior ) {
                case FRAME_RANGE_CLAMP_MODE::HOLD:
                    useStaticFrame = true;
                    break;
                case FRAME_RANGE_CLAMP_MODE::BLANK:
                    useBlankFrame = true;
                    break;
                default:
                    throw std::runtime_error( "Unrecognized before range behavior: " +
                                              boost::lexical_cast<std::string>( beforeRangeBehavior ) );
                }
            } else if( frame > last ) {
                const int afterRangeBehavior = m_afterRangeBehavior.at_time( t );
                switch( afterRangeBehavior ) {
                case FRAME_RANGE_CLAMP_MODE::HOLD:
                    useStaticFrame = true;
                    break;
                case FRAME_RANGE_CLAMP_MODE::BLANK:
                    useBlankFrame = true;
                    break;
                default:
                    throw std::runtime_error( "Unrecognized after range behavior: " +
                                              boost::lexical_cast<std::string>( afterRangeBehavior ) );
                }
            }

            frame = frantic::math::clamp( frame, first, last );
        }
    }

    if( useBlankFrame ) {
        m_fileParticleCache.clear();

        boost::shared_ptr<particle_istream> stream( new frantic::particles::streams::empty_particle_istream(
            get_default_channel_map(), get_default_channel_map() ) );
        pins.push_back( stream );
        pinNames.push_back( _T("file: ") + frantic::tstring( _T("blank") ) );
    } else {
        // Get the velocity scale
        // This scale will be applied to the velocity channel of the final mesh,
        // so we don't modify the velocity now.  However we do need to account
        // for this scale in the position offset.
        float velocityScale = 1.f;
        Interval interval;

        m_fileParticleCache.resize( m_fileList.size() );

        vector<frantic::tstring> filenames;
        vector<float> timeOffsets;
        GetParticleFilenames( t, frame, filenames, timeOffsets );
        if( filenames.size() != m_fileParticleCache.size() ) {
            throw std::runtime_error( "Internal Error: Size mismatch between cache and filename list" );
        }
        for( std::size_t i = 0; i < filenames.size(); ++i ) {
            const frantic::tstring& filename = filenames[i];
            boost::shared_ptr<particle_istream> stream;
            if( load ) {
                if( !m_fileParticleCache[i].is_current( filename, loadFraction, loadMode ) ) {
                    boost::shared_ptr<particle_istream> fileStream =
                        frantic::particles::particle_file_istream_factory( filenames[i] );
                    fileStream = prepare_file_particle_istream( fileStream );
                    m_fileParticleCache[i].set( fileStream, filename, loadFraction, loadMode );
                }
            } else {
                if( !m_fileParticleCache[i].is_current_channel_map( filename ) ) {
                    boost::shared_ptr<particle_istream> fileStream =
                        frantic::particles::particle_file_istream_factory( filenames[i] );
                    fileStream = prepare_file_particle_istream( fileStream );
                    m_fileParticleCache[i].set_no_load( fileStream, filename );
                }
            }
            stream.reset( new frantic::particles::streams::particle_array_particle_istream(
                m_fileParticleCache[i].get_particles_ref() ) );
            if( !stream ) {
                throw std::runtime_error( "Internal Error: File stream is NULL" );
            }
            set_channel_map_to_native_channel_map( stream );
            m_cachedNativeChannelMaps.push_back( stream->get_channel_map() );

            if( useStaticFrame ) {
                stream.reset( new set_channel_particle_istream<vector3f>( stream, Frost_VELOCITY, vector3f( 0.f ) ) );
            } else {
                static boost::array<frantic::tstring, 2> subframePushChannels = { Frost_POSITIONCHANNEL,
                                                                                  Frost_VELOCITY };
                static boost::array<frantic::tstring, 1> velocityScaleChannels = { Frost_VELOCITY };
                if( abs( timeOffsets[i] ) > 0.0001f )
                    stream.reset( new apply_function_particle_istream<vector3f( const vector3f&, const vector3f& )>(
                        stream, boost::bind( add_velocity_to_pos, _1, _2, velocityScale * timeOffsets[i] ),
                        Frost_POSITIONCHANNEL, subframePushChannels ) );

                if( abs( timeDerivative - 1.f ) > 0.0001f )
                    stream.reset( new apply_function_particle_istream<vector3f( const vector3f& )>(
                        stream, boost::bind( scale_vector, _1, timeDerivative ), Frost_VELOCITY,
                        velocityScaleChannels ) );
            }

            // Scale the particles if necessary
            const float particleScaleFactor = static_cast<float>( get_file_particle_scale_factor( t ) );
            if( particleScaleFactor != 1 ) {
                stream.reset( new streams::transformed_particle_istream<float>(
                    stream, transform4f::from_scale( particleScaleFactor ) ) );
            }

            // Transform to match the prt mesher's node.
            // This must happen before setting the radius -- the user
            // specified radius is already in world units so it should not be
            // scaled by this transform.
            INode* thisNode = get_inode();
            if( thisNode ) {
                transform4f tm = thisNode->GetObjectTM( t );
                if( !tm.is_identity() ) {
                    stream.reset( new streams::transformed_particle_istream<float>( stream, tm ) );
                }
            }

            // Force the channel map to have all the native channels (The Frost_RADIUS channel is just added to the
            // native map above.)
            set_channel_map_to_native_channel_map( stream );

            pins.push_back( stream );
            pinNames.push_back( _T("file: ") + filenames[i] );
        }
    }
}

boost::shared_ptr<frantic::particles::streams::particle_istream>
Frost::get_scene_object_particle_istream( TimeValue t, INode* prtNode, const frantic::graphics::camera<float>& camera,
                                          bool forceRenderState ) {
    boost::shared_ptr<frantic::particles::streams::particle_istream> result;
    TimeValue timeStep = 10;

    if( !prtNode ) {
        throw std::runtime_error( "The particle node is NULL" );
    }

    Object* prtObject = prtNode->GetObjectRef();
    if( !prtObject ) {
        throw std::runtime_error( "Could not get object from node: " +
                                  std::string( frantic::strings::to_string( prtNode->GetName() ) ) );
    }
    Object* obj = get_base_object( prtNode ); // prtObject->FindBaseObject(); fails?
    if( !obj ) {
        throw std::runtime_error( "Could not get base object from node: " +
                                  std::string( frantic::strings::to_string( prtNode->GetName() ) ) );
    }
    ObjectState os = prtNode->EvalWorldState( t );

    std::set<INode*> particleGroupNodes;
    extract_particle_groups( prtNode, particleGroupNodes );

    if( frantic::max3d::particles::IMaxKrakatoaPRTObjectPtr krakatoaPrtObject =
            frantic::max3d::particles::GetIMaxKrakatoaPRTObject( obj ) ) {
        FF_LOG( debug ) << "Getting stream from Krakatoa PRT object" << std::endl;
        frantic::max3d::particles::IMaxKrakatoaPRTEvalContextPtr krakatoaEvalContext =
            frantic::max3d::particles::CreateMaxKrakatoaPRTEvalContext( t, Frost_CLASS_ID, &camera,
                                                                        &get_default_krakatoa_channel_map() );
        if( !krakatoaEvalContext.get() ) {
            throw std::runtime_error( "The Krakatoa render context is NULL" );
        }

        Interval validInterval = FOREVER;

        result = krakatoaPrtObject->CreateStream( prtNode, validInterval, krakatoaEvalContext );

        if( result ) {
            // Older versions of Krakatoa may store Age & LifeSpan as ticks so we want to convert those to float time.
            result = frantic::max3d::particles::streams::convert_time_channels_to_seconds( result );
        }
#if defined( PHOENIX_SDK_AVAILABLE )
    } else if( IsPhoenixObject( prtNode, os, GetCOREInterface()->GetTime() ).first ) {
        FF_LOG( debug ) << "Getting stream from Phoenix FD" << std::endl;

        result = GetPhoenixParticleIstream( prtNode, t, get_default_channel_map() );

        if( !result ) {
            // Create an empty particle_istream so we don't cause any problems
            // higher up.
            // TODO: how should we handle the channel map?  radius, id, etc
            // would be defined if this was a regular particle flow event
            frantic::channels::channel_map channelMap = get_default_channel_map();
            channelMap.append_channel<boost::int32_t>( Frost_IDCHANNEL );
            result.reset( new empty_particle_istream( channelMap, channelMap ) );
        }
#endif
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    } else if( tp_interface::get_instance().is_node_thinking_particles( prtNode ) ) {
        FF_LOG( debug ) << "Getting stream from Thinking Particles" << std::endl;
        bool allGroupsFilteredOut = false;
        std::set<ReferenceTarget*> groups;
        extract_tp_groups( prtNode, groups );
        if( m_tpGroupFilterMode.at_time( t ) == PARTICLE_GROUP_FILTER_MODE::SELECTED ) {
            std::set<ReferenceTarget*> allowlist;
            for( std::size_t i = 0; i < m_tpGroupList.size(); ++i ) {
                allowlist.insert( m_tpGroupList[(int)i].at_time( t ) );
            }
            std::set<ReferenceTarget*> acceptedGroups;
            std::set_intersection(
                groups.begin(), groups.end(), allowlist.begin(), allowlist.end(),
                std::insert_iterator<std::set<ReferenceTarget*>>( acceptedGroups, acceptedGroups.begin() ) );
            if( groups.size() > 0 && acceptedGroups.size() == 0 ) {
                allGroupsFilteredOut = true;
            }
            std::swap( groups, acceptedGroups );
        }
        std::vector<boost::shared_ptr<particle_istream>> pins;
        BOOST_FOREACH( ReferenceTarget* group, groups ) {
            boost::shared_ptr<particle_istream> stream(
                tp_interface::get_instance().get_particle_stream( get_default_channel_map(), prtNode, group, t ) );
            pins.push_back( stream );
        }
        if( pins.size() == 1 ) {
            result = pins[0];
        } else if( pins.size() > 1 ) {
            result.reset( new concatenated_particle_istream( pins ) );
        } else if( allGroupsFilteredOut ) {
            // Create an empty particle_istream so we don't cause any problems
            // higher up.
            // TODO: how should we handle the channel map?  radius, id, etc
            // would be defined if this was a regular particle flow event
            frantic::channels::channel_map channelMap = get_default_channel_map();
            channelMap.append_channel<boost::int32_t>( Frost_IDCHANNEL );
            result.reset( new empty_particle_istream( channelMap, channelMap ) );
        }
        if( !result && !frantic::max3d::particles::tp_interface::get_instance().is_available() ) {
            // get_particle_stream() currently throws an exception that explains the problem
            frantic::max3d::particles::tp_interface::get_instance().get_particle_stream( get_default_channel_map(),
                                                                                         prtNode, NULL, t );
            // backup in case get_particle_stream()'s behaviour changes
            throw std::runtime_error( "Thinking Particles is not loaded, or version is not supported, but it is "
                                      "required to get particles from node: " +
                                      std::string( frantic::strings::to_string( prtNode->GetName() ) ) );
        }
#endif
    } else if( particleGroupNodes.size() ) {
        FF_LOG( debug ) << "Getting stream from Particle Flow" << std::endl;
        bool allEventsFilteredOut = false;
        if( m_pfEventFilterMode.at_time( t ) == PARTICLE_GROUP_FILTER_MODE::SELECTED ) {
            std::set<INode*> allowlist;
            for( std::size_t i = 0; i < m_pfEventList.size(); ++i ) {
                allowlist.insert( m_pfEventList[(int)i].at_time( t ) );
            }
            std::set<INode*> acceptedParticleGroupNodes;
            std::set_intersection( particleGroupNodes.begin(), particleGroupNodes.end(), allowlist.begin(),
                                   allowlist.end(),
                                   std::insert_iterator<std::set<INode*>>( acceptedParticleGroupNodes,
                                                                           acceptedParticleGroupNodes.begin() ) );
            if( particleGroupNodes.size() > 0 && acceptedParticleGroupNodes.size() == 0 ) {
                allEventsFilteredOut = true;
            }
            std::swap( particleGroupNodes, acceptedParticleGroupNodes );
        }
        std::vector<boost::shared_ptr<particle_istream>> pins;
        BOOST_FOREACH( INode* particleGroupNode, particleGroupNodes ) {
            IParticleGroup* particleGroup = GetParticleGroupInterface( particleGroupNode->GetObjectRef() );
            if( !particleGroup )
                throw std::runtime_error( "Could not get the Particle Flow IParticleGroup interface from node: " +
                                          std::string( frantic::strings::to_string( prtNode->GetName() ) ) );

            redirect_stream_in_scope redirectStreamInScope( frantic::logging::warning,
                                                            frantic::logging::debug.rdbuf() );
            boost::shared_ptr<particle_istream> stream(
                new frantic::max3d::particles::streams::max_pflow_particle_istream(
                    particleGroupNode, t, get_default_channel_map(), forceRenderState ) );
            pins.push_back( stream );
        }
        if( pins.size() > 0 ) {
            result.reset( new concatenated_particle_istream( pins ) );
        } else if( allEventsFilteredOut ) {
            // Create an empty particle_istream so we don't cause any problems
            // higher up.
            // TODO: how should we handle the channel map?  radius, id, etc
            // would be defined if this was a regular particle flow event
            frantic::channels::channel_map channelMap = get_default_channel_map();
            channelMap.append_channel<boost::int32_t>( Frost_IDCHANNEL );
            result.reset( new empty_particle_istream( channelMap, channelMap ) );
        }
    } else if( IParticleGroup* particleGroup = ParticleGroupInterface( prtObject ) ) {
        FF_LOG( debug ) << "Getting stream from ParticleGroup" << std::endl;
        result.reset( new frantic::max3d::particles::streams::max_pflow_particle_istream(
            prtNode, t, get_default_channel_map(), false ) );
    } else if( IParticleObjectExt* particleObjectExt = GetParticleObjectExtInterface( prtObject ) ) {
        if( frantic::max3d::particles::streams::thinking_particles_particle_istream::can_get_from_node( prtNode ) ) {
            FF_LOG( debug ) << "Getting stream from legacy Thinking node" << std::endl;
            result.reset( new frantic::max3d::particles::streams::thinking_particles_particle_istream(
                prtNode, t, get_default_channel_map() ) ); // TODO
            if( get_legacy_thinking_particles_radius_particle_istream::has_required_channels( result,
                                                                                              Frost_IDCHANNEL ) ) {
                result.reset( new get_legacy_thinking_particles_radius_particle_istream(
                    result, particleObjectExt, Frost_IDCHANNEL, Frost_RADIUS ) );
            }
        } else {
            FF_LOG( debug ) << "Getting stream from ParticleObjectExt" << std::endl;
            result.reset( new frantic::max3d::particles::streams::max_particleobjext_particle_istream(
                prtNode, t, get_default_channel_map() ) );
        }
    } else if( SimpleParticle* simpleParticle =
                   static_cast<SimpleParticle*>( obj->GetInterface( I_SIMPLEPARTICLEOBJ ) ) ) {
        FF_LOG( debug ) << "Getting stream from SimpleParticle" << std::endl;
        result = get_max_legacy_particle_istream( prtNode, t, get_default_channel_map() );
        //} else if( krakatoa_prt_source_particle_istream::can_get_from_node( prtNode, t ) ) {
        //	FF_LOG( debug ) << "Getting stream from IMaxKrakatoaPRTSource" << std::endl;
        //	result.reset( new krakatoa_prt_source_particle_istream( prtNode, t ) );
        //	result = frantic::max3d::particles::transform_stream_with_inode( prtNode, t, timeStep, result );
    } else if( os.obj->CanConvertToType( polyObjectClassID ) ) {
        FF_LOG( debug ) << "Getting stream from Geometry Vertices" << std::endl;
        result.reset( new frantic::max3d::particles::streams::max_geometry_vert_particle_istream(
            prtNode, t, timeStep, get_default_channel_map() ) );
        set_channel_map_to_native_channel_map( result );
        result = frantic::max3d::particles::transform_stream_with_inode( prtNode, t, timeStep, result );
    } else if( obj->ClassID() == SPHEREGIZMO_CLASSID || obj->ClassID() == CYLGIZMO_CLASSID ||
               obj->ClassID() == BOXGIZMO_CLASSID ) {
        FF_LOG( debug ) << "Getting stream from Gizmo" << std::endl;
        result.reset( new gizmo_particle_istream( prtNode, t, timeStep ) );
        set_channel_map_to_native_channel_map( result );
        result = frantic::max3d::particles::transform_stream_with_inode( prtNode, t, timeStep, result );
        result.reset( new frantic::particles::streams::set_channel_particle_istream<boost::int32_t>(
            result, Frost_IDCHANNEL, m_gizmoCounter ) );
        ++m_gizmoCounter;
    }
    // leaving triObject out for now.  it will convert nurbs in a way that may be
    // undesirable -- instead of being placed along the curve, particles are
    // placed on the render mesh's vertices
    // TODO: handle this properly
    /* else if( os.obj->CanConvertToType( triObjectClassID ) ) {
            FF_LOG( debug ) << "Getting stream from TriObject Vertices" << std::endl;
            result.reset( new frantic::max3d::particles::streams::max_triobject_vert_particle_istream( prtNode, t,
    timeStep, get_default_channel_map() ) ); result->set_channel_map( result->get_native_channel_map() ); result =
    frantic::max3d::particles::transform_stream_with_inode( prtNode, t, timeStep, result );
    }*/

    set_channel_map_to_native_channel_map( result );

    // TODO : should I really cover this up here..?
    if( result ) {
        if( result->get_channel_map().channel_count() == 0 ) {
            result.reset( new frantic::particles::streams::empty_particle_istream( get_default_channel_map(),
                                                                                   get_default_channel_map() ) );
        }
    }

    return result;
}

int Frost::get_meshing_method_internal( TimeValue t, bool useRenderMeshing ) {
    if( useRenderMeshing ) {
        return m_meshingMethod.at_time( t );
    } else {
        if( m_previewAsGeometry.at_time( t ) ) {
            return MESHING_METHOD::TETRAHEDRON;
        } else {
            return m_meshingMethod.at_time( t );
        }
    }
}

struct scoped_status_panel_prompt {
    scoped_status_panel_prompt( const std::string& message ) {
        Interface* ip = GetCOREInterface();
        if( ip ) {
            const frantic::tstring tMessage = frantic::strings::to_tstring( message );
            ip->PushPrompt( const_cast<TCHAR*>( tMessage.c_str() ) );
        }
    }
    ~scoped_status_panel_prompt() {
        Interface* ip = GetCOREInterface();
        if( ip ) {
            ip->PopPrompt();
        }
    }
};

void Frost::build_mesh( TimeValue t, const frantic::graphics::camera<float>& camera, Mesh& outMesh ) {

    boost::shared_ptr<frantic::logging::progress_logger> progressLoggerPtr;

    boost::timer timer;

    frantic::tstring nodeName;
    INode* node = get_inode();
    if( node ) {
        const TCHAR* pNodeName = node->GetName();
        if( pNodeName ) {
            nodeName = pNodeName;
        }
    }

    if( is_network_render_server() ) {
        progressLoggerPtr.reset( new frantic::logging::null_progress_logger() );
    } else if( is_render_active() ) {
        progressLoggerPtr.reset(
            new frantic::max3d::logging::check_for_render_abort_progress_logger( GetCOREInterface() ) );
    } else {
        frantic::tstring statusPanelPrefix = _T("Frost: ");
        if( !nodeName.empty() ) {
            statusPanelPrefix = frantic::tstring( _T("Frost (") ) + nodeName + _T("): ");
        }
        progressLoggerPtr.reset( new frost_status_panel_progress_logger( 0, 100, 1000 /*5000*/, statusPanelPrefix ) );
    }

    frantic::logging::progress_logger& progressLogger = *progressLoggerPtr;

    boost::int64_t particleCount = 0;

    // get loading mode
    int loadMode = m_motionBlurMode.at_time( t );

    // mprintf( "build_mesh %s time:%g renderTime:%g\n", (is_render_active(false)?"RENDERING":""),
    // t/(float)GetTicksPerFrame(), GetRenderTime()/(float)GetTicksPerFrame() ); std::ofstream out( "c:/temp/frost.log",
    // std::ios::out | std::ios::app ); out << "build_mesh " << (is_render_active(false)?"RENDERING ": " ") << "time:"
    // << boost::lexical_cast<std::string>( t/(float)GetTicksPerFrame() ) << " renderTime: " <<
    // boost::lexical_cast<std::string>( GetRenderTime()/(float)GetTicksPerFrame() ) << std::endl;

    // This rounds the input value time to the nearest frame.
    TimeValue frameTime = round_to_nearest_wholeframe( t );
    if( loadMode == MOTION_BLUR_MODE::FRAME_VELOCITY_OFFSET && HasValidRenderTime() ) {
        frameTime = round_to_nearest_wholeframe( GetRenderTime() );
    }

    bool useRenderQuality = m_inRenderingMode && !m_renderUsingViewportSettings.at_time( t );

    // If we're drawing the viewport and loading is disabled, return the empty mesh.
    // (ignore disableLoading at render time).
    // Or if we have no files in the file list box, return the empty mesh.
    // Also return an empty mesh if we're supposed to be using renderer instancing.
    // V-Ray sometimes (I'm not sure when -- newer versions?) requests a render mesh
    // even if you've given it a VRenderObject, and we want to avoid actually
    // building the render mesh in that case.
    if( ( ( !m_enableViewportMesh.at_time( frameTime ) ) && !m_inRenderingMode ) ||
        ( ( !m_enableRenderMesh.at_time( frameTime ) ) && m_inRenderingMode ) ||
        ( m_fileList.size() == 0 && !has_scene_particle_source( t ) ) || m_inRenderingInstanceMode ) {
        // Don't clear the cache if we're using instance rendering, because it might
        // be able to use the cached particles.
        if( !m_inRenderingInstanceMode ) {
            clear_particle_cache();
        }
        m_cachedMeshInRenderingMode = m_inRenderingMode;
        m_cachedMeshIsRenderQuality = useRenderQuality;
        m_cachedMeshTime = t;
        m_cachedTrimesh3.clear_and_deallocate();
        mesh_copy( outMesh, m_cachedTrimesh3 );
        invalidate_status_indicators();
        set_valid();
    } else {
        // depending on the load mode, the cache validity and the loading options will differ
        if( loadMode == MOTION_BLUR_MODE::FRAME_VELOCITY_OFFSET ||
            loadMode == MOTION_BLUR_MODE::NEAREST_FRAME_VELOCITY_OFFSET ) {
            if( m_cachedMeshTime != frameTime || m_cachedMeshInRenderingMode != m_inRenderingMode ||
                m_cachedMeshIsRenderQuality != useRenderQuality ) {
                clear_mesh( outMesh );
                m_cachedMeshTime = frameTime;
                m_cachedMeshInRenderingMode = m_inRenderingMode;
                m_cachedMeshIsRenderQuality = useRenderQuality;
                m_cachedTrimesh3.clear_and_deallocate();

                // make a new trimesh3
                frantic::geometry::trimesh3 tempMesh;
                boost::int64_t tempParticleCount;

                frantic::logging::progress_logger_subinterval_tracker tracker( progressLogger, 0, 95 );
                build_trimesh3( frameTime, camera, useRenderQuality, tempMesh, tempParticleCount, progressLogger );

                tracker.reset( 95, 100 );

                mesh_copy_time_offset( outMesh, tempMesh, TicksToSec( t - frameTime ), progressLogger );
                m_cachedTrimesh3.swap( tempMesh );
                m_cachedMeshParticleCount = tempParticleCount;
                particleCount = m_cachedMeshParticleCount;

                invalidate_status_indicators();
                set_valid();
            } else {
                // Copy the cached trimesh3 to the mesh variable
                mesh_copy_time_offset( outMesh, m_cachedTrimesh3, TicksToSec( t - frameTime ), progressLogger );
                particleCount = m_cachedMeshParticleCount;
            }
        } else if( loadMode == MOTION_BLUR_MODE::SUBFRAME_PARTICLE_OFFSET ) {
            if( m_cachedMeshTime != t || m_cachedMeshInRenderingMode != m_inRenderingMode ||
                m_cachedMeshIsRenderQuality != useRenderQuality ) {
                clear_mesh( outMesh );
                m_cachedMeshTime = t;
                m_cachedMeshInRenderingMode = m_inRenderingMode;
                m_cachedMeshIsRenderQuality = useRenderQuality;
                m_cachedTrimesh3.clear_and_deallocate();

                // make a new trimesh3
                frantic::geometry::trimesh3 tempMesh;
                boost::int64_t tempParticleCount;

                frantic::logging::progress_logger_subinterval_tracker tracker( progressLogger, 0, 95 );
                build_trimesh3( t, camera, useRenderQuality, tempMesh, tempParticleCount, progressLogger );

                tracker.reset( 95, 100 );
                mesh_copy( outMesh, tempMesh, progressLogger );

                m_cachedTrimesh3.swap( tempMesh );
                m_cachedMeshParticleCount = tempParticleCount;
                particleCount = m_cachedMeshParticleCount;
                invalidate_status_indicators();
                set_valid();
            } else {
                mesh_copy( outMesh, m_cachedTrimesh3 );
                particleCount = m_cachedMeshParticleCount;
            }
        } else {
            throw std::runtime_error( "Error: Unknown Motion Blur Mode: " +
                                      boost::lexical_cast<std::string>( loadMode ) );
        }
    }

    if( frantic::logging::is_logging_stats() ) {
        FF_LOG( stats ) << "Statistics for frame "
                        << boost::basic_format<TCHAR>( _T("%.3f") ) % ( static_cast<double>( t ) / GetTicksPerFrame() )
                        << " (" << nodeName << ")" << std::endl;
        FF_LOG( stats ) << "Particle count: " << boost::lexical_cast<frantic::tstring>( particleCount ) << std::endl;
        FF_LOG( stats ) << "Face count: " << boost::lexical_cast<frantic::tstring>( outMesh.numFaces ) << std::endl;
        FF_LOG( stats ) << "Vertex count: " << boost::lexical_cast<frantic::tstring>( outMesh.numVerts ) << std::endl;
        FF_LOG( stats ) << "Update time [s]: " << boost::basic_format<TCHAR>( _T("%.3f") ) % timer.elapsed()
                        << std::endl;
    }

    if( t == GetCOREInterface()->GetTime() && !m_inRenderingMode && !is_network_render_server() ) {
        update_status_indicators( t );
    }
}

struct union_of_spheres_params {
    particle_grid_tree* particles;
    channel_propagation_policy* cpp;
    float maximumParticleRadius;
    float effectRadiusScale;
    float implicitThreshold;
    voxel_coord_system meshingVCS;
    int vertRefinement;
    trimesh3* outMesh;
    frantic::logging::progress_logger* progressLogger;
};

struct union_of_spheres_thread_params {
    particle_grid_tree& particles;
    channel_propagation_policy& cpp;
    float maximumParticleRadius;
    float effectRadiusScale;
    float implicitThreshold;
    voxel_coord_system& meshingVCS;
    int vertRefinement;
    trimesh3& outMesh;
    frantic::volumetrics::implicitsurface::shared_progress_logger_proxy* progressLogger;

    union_of_spheres_thread_params& operator=( const union_of_spheres_thread_params& ); // not implemented

    union_of_spheres_thread_params( particle_grid_tree& particles, channel_propagation_policy& cpp,
                                    float maximumParticleRadius, float effectRadiusScale, float implicitThreshold,
                                    voxel_coord_system& meshingVCS, int vertRefinement, trimesh3& outMesh )
        : particles( particles )
        , cpp( cpp )
        , maximumParticleRadius( maximumParticleRadius )
        , effectRadiusScale( effectRadiusScale )
        , implicitThreshold( implicitThreshold )
        , meshingVCS( meshingVCS )
        , vertRefinement( vertRefinement )
        , outMesh( outMesh ) {}
};

DWORD WINAPI union_of_spheres_sparse_threadproc( LPVOID lpParameter ) {
    union_of_spheres_thread_params params = *reinterpret_cast<union_of_spheres_thread_params*>( lpParameter );

    DWORD result = 0;

    if( params.progressLogger ) {
        try {
            union_of_spheres_convert_sparse_particles_to_trimesh3(
                params.particles, params.cpp, params.maximumParticleRadius, params.effectRadiusScale,
                params.implicitThreshold, params.meshingVCS, params.vertRefinement, params.outMesh,
                *params.progressLogger );
        } catch( std::exception& /*e*/ ) {
            result = 1;
        } catch( ... ) {
            result = 1;
        }
    }

    return result;
}

struct metaball_thread_params {
    particle_grid_tree& particles;
    channel_propagation_policy& cpp;
    float maximumParticleRadius;
    float effectRadiusScale;
    float implicitThreshold;
    voxel_coord_system meshingVCS;
    int vertRefinement;
    trimesh3& outMesh;
    frantic::volumetrics::implicitsurface::shared_progress_logger_proxy* progressLogger;

    metaball_thread_params& operator=( const metaball_thread_params& ); // not implemented

    metaball_thread_params( particle_grid_tree& particles, channel_propagation_policy& cpp, float maximumParticleRadius,
                            float effectRadiusScale, float implicitThreshold, voxel_coord_system& meshingVCS,
                            int vertRefinement, trimesh3& outMesh )
        : particles( particles )
        , cpp( cpp )
        , maximumParticleRadius( maximumParticleRadius )
        , effectRadiusScale( effectRadiusScale )
        , implicitThreshold( implicitThreshold )
        , meshingVCS( meshingVCS )
        , vertRefinement( vertRefinement )
        , outMesh( outMesh ) {}
};

DWORD WINAPI metaball_sparse_threadproc( LPVOID lpParameter ) {
    metaball_thread_params params = *reinterpret_cast<metaball_thread_params*>( lpParameter );

    DWORD result = 0;

    if( params.progressLogger ) {
        try {
            metaball_convert_sparse_particles_to_trimesh3( params.particles, params.cpp, params.maximumParticleRadius,
                                                           params.effectRadiusScale, params.implicitThreshold,
                                                           params.meshingVCS, params.vertRefinement, params.outMesh,
                                                           *params.progressLogger );
        } catch( std::exception& /*e*/ ) {
            result = 1;
        } catch( ... ) {
            result = 1;
        }
    }

    return result;
}

struct zhu_bridson_thread_params {
    particle_grid_tree& particles;
    channel_propagation_policy& cpp;
    float maximumParticleRadius;
    float effectRadiusScale;
    float lowDensityTrimmingDensity;
    float lowDensityTrimmingStrength;
    voxel_coord_system& meshingVCS;
    int vertRefinement;
    trimesh3& outMesh;
    frantic::volumetrics::implicitsurface::shared_progress_logger_proxy* progressLogger;

    zhu_bridson_thread_params& operator=( const zhu_bridson_thread_params& );

    zhu_bridson_thread_params( particle_grid_tree& particles, channel_propagation_policy& cpp,
                               float maximumParticleRadius, float effectRadiusScale, float lowDensityTrimmingDensity,
                               float lowDensityTrimmingStrength, voxel_coord_system& meshingVCS, int vertRefinement,
                               trimesh3& outMesh )
        : particles( particles )
        , cpp( cpp )
        , maximumParticleRadius( maximumParticleRadius )
        , effectRadiusScale( effectRadiusScale )
        , lowDensityTrimmingDensity( lowDensityTrimmingDensity )
        , lowDensityTrimmingStrength( lowDensityTrimmingStrength )
        , meshingVCS( meshingVCS )
        , vertRefinement( vertRefinement )
        , outMesh( outMesh ) {}
};

DWORD WINAPI zhu_bridson_sparse_threadproc( LPVOID lpParameter ) {
    zhu_bridson_thread_params params = *reinterpret_cast<zhu_bridson_thread_params*>( lpParameter );
    DWORD result = 0;

    if( params.progressLogger ) {
        try {
            zhu_bridson_convert_sparse_particles_to_trimesh3(
                params.particles, params.cpp, params.maximumParticleRadius, params.effectRadiusScale,
                params.lowDensityTrimmingDensity, params.lowDensityTrimmingStrength, params.meshingVCS,
                params.vertRefinement, params.outMesh, *params.progressLogger );
        } catch( std::exception& /*e*/ ) {
            result = 1;
        } catch( ... ) {
            result = 1;
        }
    }

    return result;
}

DWORD WINAPI anisotropic_sparse_threadproc( LPVOID lpParameter ) {
    frantic::volumetrics::implicitsurface::anisotropic_sparse_params params =
        *reinterpret_cast<frantic::volumetrics::implicitsurface::anisotropic_sparse_params*>( lpParameter );

    DWORD result = 0;

    try {
        anisotropic_convert_sparse_particles_to_trimesh3( params );
    } catch( std::exception& /*e*/ ) {
        result = 1;
    } catch( ... ) {
        result = 1;
    }

    return result;
}

class progress_logger_mt_gui_adapter {
    frantic::logging::progress_logger& m_progressLogger;
    frantic::volumetrics::implicitsurface::shared_progress_logger_proxy m_proxy;

    progress_logger_mt_gui_adapter& operator=( const progress_logger_mt_gui_adapter& ); // not implemented

  public:
    progress_logger_mt_gui_adapter( frantic::logging::progress_logger& progressLogger )
        : m_progressLogger( progressLogger ) {}

    frantic::volumetrics::implicitsurface::shared_progress_logger_proxy* get_proxy() { return &m_proxy; }

    void run( LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter ) {
        DWORD threadId;
        HANDLE hThread = CreateThread( 0, 0, lpStartAddress, lpParameter, CREATE_SUSPENDED, &threadId );
        if( !hThread ) {
            throw std::runtime_error( "Internal Error: Unable to create worker thread." );
        }
        frantic::win32::AutoHandle threadAutoHandle( hThread );

        DWORD result = ResumeThread( hThread );
        if( result == -1 ) {
            throw std::runtime_error( "Internal Error: Unable to resume worker thread." );
        }
        bool done = false;
        while( !done ) {
            result = WaitForMultipleObjectsEx( 1, &hThread, FALSE, 100, FALSE );

            if( result == WAIT_OBJECT_0 || result == WAIT_ABANDONED_0 ) {
                done = true;
            } else if( result == WAIT_TIMEOUT ) {
            } else {
                throw std::runtime_error( "Internal Error: Got unexpected thread polling result: " +
                                          boost::lexical_cast<std::string>( result ) );
            }

            bool cancel = false;
            try {
                m_progressLogger.update_progress( m_proxy.get_progress() );
            } catch( frantic::logging::progress_cancel_exception& /*e*/ ) {
                cancel = true;
            }

            if( cancel ) {
                m_proxy.cancel();
            }
        }

        DWORD exitCode;
        BOOL gotExitCode = GetExitCodeThread( hThread, &exitCode );
        if( gotExitCode ) {
            if( exitCode == 0 ) {
                // yay
            } else if( exitCode == 1 ) {
                throw frantic::logging::progress_cancel_exception( "Process Cancelled" );
            } else {
                throw std::runtime_error( "Internal Error: Got unexpected return code from thread: " +
                                          boost::lexical_cast<std::string>( exitCode ) );
            }
        } else {
            throw std::runtime_error( "Internal Error: Could not get exit code from thread." );
        }
    }

    bool was_cancelled() { return false; }
};

DWORD WINAPI smooth_particle_positions_threadproc( LPVOID lpParameter ) {
    smooth_particle_positions_params params = *reinterpret_cast<smooth_particle_positions_params*>( lpParameter );

    DWORD result = 0;

    try {
        smooth_particle_positions( params.particles, params.effectRadiusScale, params.lambda, params.progressLogger );
    } catch( std::exception& /*e*/ ) {
        result = 1;
    } catch( ... ) {
        result = 1;
    }

    return result;
}

struct calculate_volume_params {
    particle_array* particles;
    frantic::tstring volumeChannelName;
    float effectRadiusScale;
    frantic::volumetrics::implicitsurface::shared_progress_logger_proxy* progressLogger;
};

DWORD WINAPI calculate_volume_with_anisotropic_kernel_threadproc( LPVOID lpParameter ) {
    calculate_volume_params params = *reinterpret_cast<calculate_volume_params*>( lpParameter );

    DWORD result = 0;

    if( params.particles && params.progressLogger ) {
        try {
            calculate_volume_with_anisotropic_kernel( *params.particles, params.volumeChannelName,
                                                      *params.progressLogger );
        } catch( std::exception& /*e*/ ) {
            result = 1;
        } catch( ... ) {
            result = 1;
        }
    }

    return result;
}

DWORD WINAPI calculate_anisotropy_threadproc( LPVOID lpParameter ) {
    calculate_anisotropy_params params = *reinterpret_cast<calculate_anisotropy_params*>( lpParameter );

    DWORD result = 0;

    try {
        calculate_anisotropy( params.particles, params.effectRadiusScale, params.anisotropyWindowRadiusScale, params.kr,
                              /*params.kn, params.ks,*/ params.ne, params.progressLogger );
    } catch( std::exception& /*e*/ ) {
        result = 1;
    } catch( ... ) {
        result = 1;
    }

    return result;
}

// look for infinite values in a contiguous array of floating point numbers
// display a warning on FF_LOG( warning ) if one is found
// convert to float32 if necessary
static void warn_if_infinite( frantic::channels::data_type_t dataType, const char* data, std::size_t count,
                              const char* message ) {
    if( dataType == frantic::channels::data_type_float32 ) {
        const float* f = reinterpret_cast<const float*>( data );
        for( std::size_t i = 0; i < count; ++i ) {
            if( !frantic::math::is_finite( f[i] ) ) {
                FF_LOG( warning ) << message << std::endl;
                return;
            }
        }
    } else {
        channel_type_convertor_function_t convertType =
            get_channel_type_convertor_function( dataType, data_type_float32, _T("Velocity") );
        const std::size_t step = sizeof_channel_data_type( dataType );
        for( std::size_t i = 0; i < count; ++i ) {
            float f;
            convertType( reinterpret_cast<char*>( &f ), data + step * i, 1 );
            if( !frantic::math::is_finite( f ) ) {
                FF_LOG( warning ) << message << std::endl;
                return;
            }
        }
    }
}

// look for infinite values in the "Velocity" channel, if it exists
// display a warning on FF_LOG( warning ) if one is found
static void warn_if_infinite_velocity( const frantic::particles::particle_array& pa ) {
    if( pa.get_channel_map().has_channel( _T("Velocity") ) ) {
        const frantic::channels::channel& ch = pa.get_channel_map()[_T("Velocity")];
        if( ch.data_type() == frantic::channels::data_type_float32 ) {
            const frantic::channels::channel_accessor<frantic::graphics::vector3f> velAcc =
                pa.get_channel_map().get_accessor<frantic::graphics::vector3f>( _T("Velocity") );
            for( std::size_t i = 0; i < pa.size(); ++i ) {
                if( !velAcc( pa[i] ).is_finite() ) {
                    FF_LOG( warning ) << "Particle " << boost::lexical_cast<frantic::tstring>( i )
                                      << " has infinite velocity." << std::endl;
                    return;
                }
            }
        } else {
            frantic::channels::channel_const_cvt_accessor<frantic::graphics::vector3f> velAcc =
                pa.get_channel_map().get_const_cvt_accessor<frantic::graphics::vector3f>( _T("Velocity") );
            for( std::size_t i = 0; i < pa.size(); ++i ) {
                if( !velAcc( pa[i] ).is_finite() ) {
                    FF_LOG( warning ) << "Particle " << boost::lexical_cast<frantic::tstring>( i )
                                      << " has infinite velocity." << std::endl;
                    return;
                }
            }
        }
    }
}

void Frost::build_trimesh3( const TimeValue t, const frantic::graphics::camera<float>& camera,
                            const bool useRenderMeshing, trimesh3& outMesh ) {
    frantic::logging::null_progress_logger nullLogger;
    boost::int64_t tempParticleCount;
    build_trimesh3( t, camera, useRenderMeshing, outMesh, tempParticleCount, nullLogger );
}

namespace {

frantic::graphics::boundbox3f voxelize( const frantic::graphics::boundbox3f& bounds,
                                        const frantic::volumetrics::voxel_coord_system& vcs ) {
    const float voxelLength = vcs.voxel_length();

    const vector3 minCorner =
        vector3::from_ceil( vcs.get_voxel_coord( bounds.minimum() - vector3f( voxelLength / 2 ) ) );
    const vector3 maxCorner =
        vector3::from_floor( vcs.get_voxel_coord( bounds.maximum() - vector3f( voxelLength / 2 ) ) );

    return boundbox3f( vcs.get_world_voxel_center( minCorner ), vcs.get_world_voxel_center( maxCorner ) );
}

float get_max_distance_channel_value( const frantic::particles::particle_array& pa ) {
    frantic::channels::channel_accessor<float> maxDistanceAcc =
        pa.get_channel_map().get_accessor<float>( _T("__MaxDistance") );

    float maxDistance = 0;
    for( std::size_t i = 0, ie = pa.size(); i < ie; ++i ) {
        maxDistance = std::max( maxDistance, maxDistanceAcc( pa[i] ) );
    }
    return maxDistance;
}

} // anonymous namespace

void Frost::build_trimesh3( const TimeValue t, const frantic::graphics::camera<float>& camera,
                            const bool useRenderMeshing, trimesh3& outMesh, boost::int64_t& outParticleCount,
                            frantic::logging::progress_logger& progressLogger ) {
    // We need a task_scheduler_init here because some plugins evaluate
    // us on a new thread (for example, ObjExp).
    tbb::task_scheduler_init taskSchedulerInit;

    outMesh.clear();
    outParticleCount = 0;

    boost::shared_ptr<particle_istream> pin = GetParticleIStream( t, camera );
    validate_channel_map_caches( pin );
    if( pin->particle_count() == 0 ) {
        return;
    }

    if( !pin->get_channel_map().has_channel( Frost_POSITIONCHANNEL ) ) {
        throw std::runtime_error( "The particles are missing a Position channel." );
    }

    if( !pin->get_channel_map().has_channel( Frost_RADIUS ) ) {
        throw std::runtime_error( "The particles are missing a Radius channel." );
    }

    const int meshingMethod = get_meshing_method_internal( t, useRenderMeshing );

    const int materialMode = m_materialMode.at_time( t );
    const MtlID undefinedMaterialID = get_undefined_material_id();

    boost::optional<boundbox3f> particleCullingBox = get_roi_box( t );
    boost::optional<boundbox3f> meshCullingBox;

    // Get the channel names which will be propagated to the mesh, for now it is everything
    frantic::channels::channel_propagation_policy cpp( true );
    for( size_t i = 0; i < pin->get_channel_map().channel_count(); ++i ) {
        const frantic::tstring& channelName = pin->get_channel_map()[i].name();
        if( channelName == _T("Color") || channelName == Frost_VELOCITY || channelName == _T("TextureCoord") ||
            channelName.substr( 0, 12 ) == _T("__mapChannel") || channelName.substr( 0, 7 ) == _T("Mapping") ||
            materialMode == MATERIAL_MODE::MTLINDEX_CHANNEL && channelName == _T("MtlIndex") ) {
            cpp.add_channel( channelName );
        }
    }

    frantic::channels::channel_map newParticleChannelMap;
    for( size_t i = 0; i < pin->get_channel_map().channel_count(); ++i ) {
        const frantic::channels::channel ch = pin->get_channel_map()[i];
        const frantic::tstring& channelName = ch.name();

        if( channelName == Frost_RADIUS ) {
            newParticleChannelMap.define_channel( channelName, ch.arity(), frantic::channels::data_type_float32 );
        } else if( channelName == Frost_POSITIONCHANNEL ) {
            newParticleChannelMap.define_channel( channelName, ch.arity(), frantic::channels::data_type_float32 );
        } else if( cpp.is_channel_included( channelName ) ) {
            newParticleChannelMap.define_channel( channelName, ch.arity(), ch.data_type() );
        } else if( meshingMethod == MESHING_METHOD::GEOMETRY ) {
            // TODO : check if a channel will actually be used
            newParticleChannelMap.define_channel( channelName, ch.arity(), ch.data_type() );
        }
    }
    newParticleChannelMap.end_channel_definition();

    pin->set_channel_map( newParticleChannelMap );

    // Meshing directly from geometry
    if( meshingMethod == MESHING_METHOD::TETRAHEDRON ) {
        if( particleCullingBox ) {
            pin.reset( new box_culling_particle_istream( pin, *particleCullingBox ) );
        }

        vector<char> particle( pin->particle_size() );

        channel_accessor<vector3f> positionAcc = pin->get_channel_map().get_accessor<vector3f>( Frost_POSITIONCHANNEL );
        channel_const_cvt_accessor<float> radiusAcc =
            pin->get_channel_map().get_const_cvt_accessor<float>( Frost_RADIUS );

        std::vector<frantic::tstring> particleChannelNames;
        for( std::size_t i = 0; i < pin->get_channel_map().channel_count(); ++i ) {
            const frantic::channels::channel ch = pin->get_channel_map()[i];
            const frantic::tstring& channelName = ch.name();

            if( cpp.is_channel_included( channelName ) ) {
                particleChannelNames.push_back( channelName );
            }
        }

        // Create accessors and channel converters for the particle input and the mesh output.
        vector<channel_general_accessor> inputChannels;
        for( unsigned i = 0; i < particleChannelNames.size(); ++i ) {
            inputChannels.push_back( pin->get_channel_map().get_general_accessor( particleChannelNames[i] ) );
        }

        std::map<frantic::tstring, frantic::tstring> particleToMeshChannelNames;
        particleToMeshChannelNames[_T("MtlIndex")] = _T("MaterialID");

        frantic::channels::channel_map vertexChannelMap;
        frantic::channels::channel_map faceChannelMap;

        vector<channel_general_accessor> particleVertexChannels;
        vector<channel_general_accessor> particleFaceChannels;

        for( std::size_t particleChannelNumber = 0; particleChannelNumber < particleChannelNames.size();
             ++particleChannelNumber ) {
            const frantic::tstring& particleChannelName = particleChannelNames[particleChannelNumber];
            const channel_general_accessor& ch = inputChannels[particleChannelNumber];

            if( particleChannelName == _T("MtlIndex") ) {
                faceChannelMap.define_channel( _T("MaterialID"), ch.arity(), ch.data_type() );
                particleFaceChannels.push_back( ch );
            } else {
                vertexChannelMap.define_channel( particleChannelName, ch.arity(), ch.data_type() );
                particleVertexChannels.push_back( ch );
            }
        }

        vertexChannelMap.end_channel_definition( 4, true );
        faceChannelMap.end_channel_definition( 4, true );

        trimesh3 primitiveMesh;
        vector<trimesh3_vertex_channel_general_accessor> primitiveVertexChannels;
        vector<trimesh3_face_channel_general_accessor> primitiveFaceChannels;

        primitiveMesh.set_to_tetrahedron( vector3f( 0 ), 1.f );

        primitiveMesh.add_face_channel<int>( Frost_SMOOTHINGGROUPCHANNEL );
        frantic::geometry::trimesh3_face_channel_accessor<int> sgAcc =
            primitiveMesh.get_face_channel_accessor<int>( Frost_SMOOTHINGGROUPCHANNEL );
        sgAcc[0] = 1;
        sgAcc[1] = 2;
        sgAcc[2] = 4;
        sgAcc[3] = 8;

        for( size_t i = 0; i < vertexChannelMap.channel_count(); ++i ) {
            const frantic::channels::channel& ch = vertexChannelMap[i];
            const frantic::tstring& channelName = ch.name();
            primitiveMesh.add_vertex_channel_raw( channelName, ch.arity(), ch.data_type() );
            primitiveVertexChannels.push_back( primitiveMesh.get_vertex_channel_general_accessor( channelName ) );
        }

        for( size_t i = 0; i < faceChannelMap.channel_count(); ++i ) {
            const frantic::channels::channel& ch = faceChannelMap[i];
            const frantic::tstring& channelName = ch.name();
            primitiveMesh.add_face_channel_raw( channelName, ch.arity(), ch.data_type() );
            primitiveFaceChannels.push_back( primitiveMesh.get_face_channel_general_accessor( channelName ) );
        }

        if( pin->particle_count() > 0 ) {
            outMesh.reserve_faces( std::size_t( primitiveMesh.face_count() * pin->particle_count() ) );
            outMesh.reserve_vertices( std::size_t( primitiveMesh.vertex_count() * pin->particle_count() ) );
        }

        // Create vector of channels that should be transformed. Velocity should not be transformed.
        std::vector<std::pair<frantic::tstring, frantic::geometry::trimesh3::vector_type>> transformChannels;
        transformChannels.push_back( std::pair<frantic::tstring, frantic::geometry::trimesh3::vector_type>(
            _T("Normal"), frantic::geometry::trimesh3::NORMAL ) );

        // update progress on the main particle loop
        long long total = pin->particle_count();
        long long count = 0;

        while( pin->get_particle( particle ) ) {
            // Copy all the channels to the primitive mesh
            for( size_t i = 0; i < primitiveVertexChannels.size(); ++i ) {
                const void* inputChannelValue = particleVertexChannels[i].get_channel_data_pointer( particle );
                size_t channelSize = particleVertexChannels[i].primitive_size();
                trimesh3_vertex_channel_general_accessor& meshChannel = primitiveVertexChannels[i];

                for( unsigned j = 0; j < meshChannel.size(); ++j ) {
                    memcpy( meshChannel.data( j ), inputChannelValue, channelSize );
                }
            }
            for( size_t i = 0; i < primitiveFaceChannels.size(); ++i ) {
                const void* inputChannelValue = particleFaceChannels[i].get_channel_data_pointer( particle );
                size_t channelSize = particleFaceChannels[i].primitive_size();
                trimesh3_face_channel_general_accessor& meshChannel = primitiveFaceChannels[i];

                for( unsigned j = 0; j < meshChannel.size(); ++j ) {
                    memcpy( meshChannel.data( j ), inputChannelValue, channelSize );
                }
            }

            // Then combine the primitive mesh with the positioning
            outMesh.combine( transform4f::from_translation( positionAcc( particle ) ) *
                                 transform4f::from_scale( radiusAcc( particle ) ),
                             primitiveMesh, transformChannels );
            ++count;
            ++outParticleCount;
            if( total > 0 ) {
                progressLogger.update_progress( count, total );
            } else {
                progressLogger.update_progress(
                    1, 2 ); // TODO: what ?  (I don't think this can happen with the current caching scheme)
            }
        }
    } else if( meshingMethod == MESHING_METHOD::GEOMETRY ) {
        using namespace frost;

        if( particleCullingBox ) {
            pin.reset( new box_culling_particle_istream( pin, *particleCullingBox ) );
        }

        boost::shared_ptr<max3d::max3d_geometry_meshing_parameters> params(
            new max3d::max3d_geometry_meshing_parameters( this, t ) );

        boost::shared_ptr<geometry_source_interface> sceneGeometrySource( new max3d_geometry_source( m_geometryList ) );

        const geometry_type::option instanceGeometryType = params->get_geometry_type();

        const std::size_t shapeCount = get_instance_geometry_shape_count( instanceGeometryType, sceneGeometrySource );

        typedef std::map<INode*, std::size_t> node_to_shape_number_t;
        node_to_shape_number_t nodeToShapeNumber;

        for( std::size_t i = 0; i < m_geometryList.size(); ++i ) {
            nodeToShapeNumber[m_geometryList[(int)i].at_time( 0 )] = i;
        }

        material_id_mode_info materialModeInfo( materialMode, undefinedMaterialID );

        std::vector<geometry_material_id_map_t>& geometryMaterialIdMaps = materialModeInfo.geometryMaterialIdMaps;
        geometryMaterialIdMaps.resize( shapeCount );
        std::size_t materialIDMapEntryCount =
            std::min( m_geometryMaterialIDNodeList.size(),
                      std::min( m_geometryMaterialIDInList.size(), m_geometryMaterialIDOutList.size() ) );
        bool showedWarning = false;

        for( std::size_t i = 0; i < materialIDMapEntryCount; ++i ) {
            node_to_shape_number_t::const_iterator iter =
                nodeToShapeNumber.find( m_geometryMaterialIDNodeList[(int)i].at_time( 0 ) );
            if( iter == nodeToShapeNumber.end() ) {
                continue;
            }
            std::size_t shapeNumber = iter->second;
            if( shapeNumber >= shapeCount ) {
                continue;
            }

            const int maxId = std::numeric_limits<MtlID>::max();
            int inId = m_geometryMaterialIDInList[(int)i].at_time( 0 );
            if( inId < 0 || inId > maxId ) {
                if( !showedWarning ) {
                    report_warning( _T("geometryMaterialIDInList[") + boost::lexical_cast<frantic::tstring>( i + 1 ) +
                                    _T("] ") + boost::lexical_cast<frantic::tstring>( inId + 1 ) +
                                    _T(" is out of range [1,") + boost::lexical_cast<frantic::tstring>( maxId + 1 ) +
                                    _T("]\n") );
                    showedWarning = true;
                }
                continue;
            }

            int outId = m_geometryMaterialIDOutList[(int)i].at_time( 0 );
            if( outId < 0 || outId > maxId ) {
                if( !showedWarning ) {
                    report_warning( _T("geometryMaterialIDOutList[") + boost::lexical_cast<frantic::tstring>( i + 1 ) +
                                    _T("] ") + boost::lexical_cast<frantic::tstring>( outId + 1 ) +
                                    _T(" is out of range [1,") + boost::lexical_cast<frantic::tstring>( maxId + 1 ) +
                                    _T("]\n") );
                    showedWarning = true;
                }
                continue;
            }

            geometryMaterialIdMaps[shapeNumber][static_cast<MtlID>( inId )] = static_cast<MtlID>( outId );
        }

        std::vector<int>& meshEndMaterialId = materialModeInfo.meshEndMaterialId;
        meshEndMaterialId.clear();
        meshEndMaterialId.resize( shapeCount, 1 );
        if( materialMode == MATERIAL_MODE::MATERIAL_FROM_GEOMETRY ) {
            for( std::size_t shapeNumber = 0; shapeNumber < shapeCount; ++shapeNumber ) {
                boost::uint16_t maxMaterialId = 0;
                if( shapeNumber < geometryMaterialIdMaps.size() ) {
                    geometry_material_id_map_t& materialIdMap = geometryMaterialIdMaps[shapeNumber];
                    for( geometry_material_id_map_t::iterator i = materialIdMap.begin(); i != materialIdMap.end();
                         ++i ) {
                        if( i->first > maxMaterialId ) {
                            maxMaterialId = i->first;
                        }
                    }
                }
                meshEndMaterialId[shapeNumber] = 1 + static_cast<int>( maxMaterialId );
            }
        }

        geometry_convert_particles_to_trimesh3( pin, cpp, params, sceneGeometrySource, materialModeInfo, outMesh,
                                                outParticleCount, progressLogger );

        m_cachedTargetTm = params->get_cached_look_at_transform();
    } else if( meshingMethod == MESHING_METHOD::VERTEX_CLOUD ) {
        if( particleCullingBox ) {
            pin.reset( new box_culling_particle_istream( pin, *particleCullingBox ) );
        }

        // If we don't know the particle count, then let's read the particles into a particle_array.
        // Then we can reset the pin stream to read from the particle_array.
        particle_array particles( pin->get_channel_map() );
        if( pin->particle_count() < 0 ) {
            particles.insert_particles( pin );
            pin.reset( new particle_array_particle_istream( particles, pin->name() ) );
        }

        vector<char> particle( pin->particle_size() );
        channel_accessor<vector3f> positionAcc = pin->get_channel_map().get_accessor<vector3f>( Frost_POSITIONCHANNEL );

        std::vector<frantic::tstring> particleChannelNames;
        for( std::size_t i = 0; i < pin->get_channel_map().channel_count(); ++i ) {
            const frantic::channels::channel ch = pin->get_channel_map()[i];
            const frantic::tstring& channelName = ch.name();

            if( cpp.is_channel_included( channelName ) ) {
                particleChannelNames.push_back( channelName );
            }
        }

        // Create accessors and channel converters for the particle input and the mesh output.
        vector<channel_general_accessor> inputChannels;
        for( unsigned i = 0; i < particleChannelNames.size(); ++i ) {
            inputChannels.push_back( pin->get_channel_map().get_general_accessor( particleChannelNames[i] ) );
        }

        frantic::channels::channel_map vertexChannelMap;

        vector<channel_general_accessor> particleVertexChannels;

        for( std::size_t particleChannelNumber = 0; particleChannelNumber < particleChannelNames.size();
             ++particleChannelNumber ) {
            const frantic::tstring& particleChannelName = particleChannelNames[particleChannelNumber];
            const channel_general_accessor& ch = inputChannels[particleChannelNumber];

            vertexChannelMap.define_channel( particleChannelName, ch.arity(), ch.data_type() );
            particleVertexChannels.push_back( ch );
        }

        vertexChannelMap.end_channel_definition( 4, true );

        // Code to produce the vertex cloud
        outMesh.set_vertex_count( (int)pin->particle_count() );

        vector<trimesh3_vertex_channel_general_accessor> outputChannels;
        for( unsigned i = 0; i < vertexChannelMap.channel_count(); ++i ) {
            const frantic::channels::channel& ch = vertexChannelMap[i];
            outMesh.add_vertex_channel_raw( ch.name(), ch.arity(), ch.data_type() );
            outputChannels.push_back( outMesh.get_vertex_channel_general_accessor( ch.name() ) );
        }

        // update progress on the main particle loop
        int index = 0;
        long long total = pin->particle_count();

        while( pin->get_particle( particle ) ) {
            // Copy the vertex position
            outMesh.get_vertex( index ) = positionAcc( particle );
            // Copy all the rest of the named channels
            for( unsigned i = 0; i < particleVertexChannels.size(); ++i ) {
                memcpy( outputChannels[i].data( index ), particleVertexChannels[i].get_channel_data_pointer( particle ),
                        particleVertexChannels[i].primitive_size() );
            }
            ++index;
            ++outParticleCount;
            if( ( index & 0x3FFF ) == 0 ) {
                progressLogger.update_progress( index, total );
            }
        }
        progressLogger.update_progress( 100.f );
    } else if( meshingMethod == MESHING_METHOD::UNION_OF_SPHERES || meshingMethod == MESHING_METHOD::ZHU_BRIDSON ||
               meshingMethod == MESHING_METHOD::METABALLS ) {
        particle_array pa( create_optimized_channel_map( pin->get_channel_map() ) );
        frantic::logging::progress_logger_subinterval_tracker tracker( progressLogger, 0, 5 );
        pa.insert_particles( pin, progressLogger );
        progressLogger.update_progress( 100.f );
        tracker.reset( 5, 100 );

        if( frantic::logging::is_logging_debug() ) {
            warn_if_infinite_velocity( pa );
        }

        outParticleCount = pa.size();

        std::pair<float, float> particleRadiusExtrema = get_particle_radius_extrema( t, pa );
        const float minimumParticleRadius = particleRadiusExtrema.first;
        const float maximumParticleRadius = particleRadiusExtrema.second;

        float meshingResolution;
        int vertRefinement;
        if( useRenderMeshing ) {
            meshingResolution = m_renderMeshingResolution.at_time( t );
            vertRefinement = m_renderVertRefinementIterations.at_time( t );
        } else {
            meshingResolution = m_viewportMeshingResolution.at_time( t );
            vertRefinement = m_viewportVertRefinementIterations.at_time( t );
        }

        float maximumParticleRadiusForVoxelLength = maximumParticleRadius;
        float meshingVoxelLength = 0;
        const int meshingResolutionMode = m_meshingResolutionMode.at_time( t );
        if( meshingResolutionMode == MESHING_RESOLUTION_MODE::SUBDIVIDE_MAX_RADIUS ) {
            meshingVoxelLength = maximumParticleRadius / meshingResolution;
        } else if( meshingResolutionMode == MESHING_RESOLUTION_MODE::VOXEL_LENGTH ) {
            if( useRenderMeshing ) {
                meshingVoxelLength = m_renderVoxelLength.at_time( t );
            } else {
                meshingVoxelLength = m_viewportVoxelLength.at_time( t );
            }
            if( meshingVoxelLength <= 0 ) {
                throw std::runtime_error( "Voxel Length must be greater than zero." );
            }
        } else {
            throw std::runtime_error( "Unrecognized meshingResolutionMode: " +
                                      boost::lexical_cast<std::string>( meshingResolutionMode ) );
        }

        frantic::volumetrics::voxel_coord_system meshingVCS( vector3f( 0 ), meshingVoxelLength );
        if( meshingVCS.voxel_length() == 0 ) {
            return;
        }

        float effectRadiusScale;
        if( meshingMethod == MESHING_METHOD::UNION_OF_SPHERES ) {
            effectRadiusScale = ( minimumParticleRadius == 0 )
                                    ? 2.f
                                    : std::min( 2.f, 1.f + 2.f * meshingVCS.voxel_length() / minimumParticleRadius );
        } else if( meshingMethod == MESHING_METHOD::ZHU_BRIDSON ) {
            effectRadiusScale = m_zhuBridsonBlendRadiusScale.at_time( t );
        } else if( meshingMethod, MESHING_METHOD::METABALLS ) {
            effectRadiusScale = m_metaballRadiusScale.at_time( t );
        } else {
            outMesh.clear();
            throw std::runtime_error( "Internal Error: Unrecognized implicit surface meshing method: " +
                                      boost::lexical_cast<std::string>( meshingMethod ) );
        }

        // build the pgt
        const float particleVoxelLength =
            std::max<float>( maximumParticleRadius * effectRadiusScale, 0.5f * meshingVoxelLength );
        voxel_coord_system particleVCS( vector3f( 0 ), particleVoxelLength );
        if( particleVCS.voxel_length() == 0 ) {
            return;
        }

        if( particleCullingBox ) {
            particleCullingBox->expand( maximumParticleRadius * effectRadiusScale + 2 * meshingVoxelLength );
        }

        particle_grid_tree particles( pa.get_channel_map(), particleVCS );
        { // scope for radiusAcc and positionAcc
            frantic::channels::channel_accessor<float> radiusAcc =
                pa.get_channel_map().get_accessor<float>( Frost_RADIUS );
            frantic::channels::channel_accessor<frantic::graphics::vector3f> positionAcc =
                pa.get_channel_map().get_accessor<frantic::graphics::vector3f>( Frost_POSITIONCHANNEL );
            std::size_t infinitePositionCount = 0;
            for( frantic::particles::particle_array::iterator i( pa.begin() ); i != pa.end(); ++i ) {
                const char* p = *i;
                const vector3f position = positionAcc( p );
                if( position.is_finite() ) {
                    if( radiusAcc( p ) > 0 ) {
                        bool accept = true;
                        if( particleCullingBox && !particleCullingBox->contains( position ) ) {
                            accept = false;
                        }
                        if( accept ) {
                            particles.insert( p );
                        }
                    }
                } else {
                    ++infinitePositionCount;
                }
            }
            if( infinitePositionCount > 0 ) {
                FF_LOG( warning ) << "Removed " << boost::lexical_cast<frantic::tstring>( infinitePositionCount )
                                  << " particle" << ( infinitePositionCount > 1 ? "s" : "" )
                                  << " with non-finite Position" << std::endl;
            }
        }

        pa.clear();
        pa.trim();

        frantic::graphics::boundbox3f worldParticleBounds = particles.compute_particle_bounds();
        worldParticleBounds.expand( effectRadiusScale * maximumParticleRadius );
        const frantic::graphics::boundbox3 voxelParticleBounds = meshingVCS.get_voxel_bounds( worldParticleBounds );
        const bool useSparseMeshing = get_volume_double( voxelParticleBounds ) > SPARSE_MESHING_VOXEL_COUNT_THRESHOLD;

        if( meshingMethod == MESHING_METHOD::UNION_OF_SPHERES ) {
            float implicitThreshold = 2.f * meshingVCS.voxel_length();
            if( useSparseMeshing ) {
                progress_logger_mt_gui_adapter uiDelegate( progressLogger );
                union_of_spheres_thread_params params( particles, cpp, maximumParticleRadius, effectRadiusScale,
                                                       implicitThreshold, meshingVCS, vertRefinement, outMesh );
                params.progressLogger = uiDelegate.get_proxy();
                uiDelegate.run( &union_of_spheres_sparse_threadproc, &params );
                // union_of_spheres_convert_sparse_particles_to_trimesh3( particles, cpp, maximumParticleRadius,
                // effectRadiusScale, implicitThreshold, meshingVCS, vertRefinement, outMesh, progressLogger );
            } else {
                union_of_spheres_convert_particles_to_trimesh3( particles, cpp, maximumParticleRadius,
                                                                effectRadiusScale, implicitThreshold, meshingVCS,
                                                                vertRefinement, outMesh, progressLogger );
            }
        } else if( meshingMethod == MESHING_METHOD::ZHU_BRIDSON ) {
            const bool enableLowDensityTrimming = m_zhuBridsonEnableLowDensityTrimming.at_time( t );
            float lowDensityTrimmingDensity =
                enableLowDensityTrimming ? m_zhuBridsonLowDensityTrimmingThreshold.at_time( t ) : 0.f;
            float lowDensityTrimmingStrength = m_zhuBridsonLowDensityTrimmingStrength.at_time( t );
            if( useSparseMeshing ) {
                progress_logger_mt_gui_adapter uiDelegate( progressLogger );
                zhu_bridson_thread_params params( particles, cpp, maximumParticleRadiusForVoxelLength,
                                                  effectRadiusScale, lowDensityTrimmingDensity,
                                                  lowDensityTrimmingStrength, meshingVCS, vertRefinement, outMesh );
                params.progressLogger = uiDelegate.get_proxy();
                uiDelegate.run( &zhu_bridson_sparse_threadproc, &params );
                // zhu_bridson_convert_sparse_particles_to_trimesh3( particles, cpp,
                // maximumParticleRadiusForVoxelLength, effectRadiusScale, lowDensityTrimmingDensity,
                // lowDensityTrimmingStrength, meshingVCS, vertRefinement, outMesh, progressLogger );
            } else {
                zhu_bridson_convert_particles_to_trimesh3(
                    particles, cpp, maximumParticleRadiusForVoxelLength, effectRadiusScale, lowDensityTrimmingDensity,
                    lowDensityTrimmingStrength, meshingVCS, vertRefinement, outMesh, progressLogger );
            }
        } else if( meshingMethod == MESHING_METHOD::METABALLS ) {
            float implicitThreshold = m_metaballIsosurfaceLevel.at_time( t );
            if( useSparseMeshing ) {
                progress_logger_mt_gui_adapter uiDelegate( progressLogger );
                metaball_thread_params params( particles, cpp, maximumParticleRadius, effectRadiusScale,
                                               implicitThreshold, meshingVCS, vertRefinement, outMesh );
                params.progressLogger = uiDelegate.get_proxy();
                uiDelegate.run( &metaball_sparse_threadproc, &params );
                // metaball_convert_sparse_particles_to_trimesh3( particles, cpp, maximumParticleRadius,
                // effectRadiusScale, implicitThreshold, meshingVCS, vertRefinement, outMesh, progressLogger );
            } else {
                metaball_convert_particles_to_trimesh3( particles, cpp, maximumParticleRadius, effectRadiusScale,
                                                        implicitThreshold, meshingVCS, vertRefinement, outMesh,
                                                        progressLogger );
            }
        } else {
            outMesh.clear();
            throw std::runtime_error( "Internal Error: Unrecognized implicit surface meshing method: " +
                                      boost::lexical_cast<std::string>( meshingMethod ) );
        }

        meshCullingBox = get_roi_box( t );
        if( meshCullingBox ) {
            meshCullingBox = voxelize( *meshCullingBox, meshingVCS );
        }
    } else if( meshingMethod == MESHING_METHOD::ANISOTROPIC ) {

        const frantic::tstring volumeChannelName = _T("__Volume");

        pin.reset( new positive_radius_culled_particle_istream( pin, boost::none ) );
        pin->set_channel_map( newParticleChannelMap );

        if( particleCullingBox ) {
            pin = broad_cull_anisotropic_particles( t, pin, particleCullingBox );
            pin->set_channel_map( newParticleChannelMap );
        }

        // Load particles into a particle array
        frantic::channels::channel_map channelMap( create_optimized_channel_map( pin->get_channel_map() ) );
        channelMap.append_channel<float>( volumeChannelName );
        channelMap.append_channel<float>( _T("__MaxDistance") );
        channelMap.append_channel( _T("__Anisotropy"), 6, frantic::channels::data_type_float32 );
        channelMap.append_channel<float>( _T("__invcsv") );
        particle_array pa( channelMap );
        frantic::logging::progress_logger_subinterval_tracker tracker( progressLogger, 0, 5 );
        pa.insert_particles( pin, progressLogger );
        progressLogger.update_progress( 100.f );

        outParticleCount = pa.size();

        // Get the max particle radius
        std::pair<float, float> particleRadiusExtrema = get_particle_radius_extrema( t, pa );
        const float maximumParticleRadius = particleRadiusExtrema.second;

        float meshingResolution;
        int vertRefinement;
        if( useRenderMeshing ) {
            meshingResolution = m_renderMeshingResolution.at_time( t );
            vertRefinement = m_renderVertRefinementIterations.at_time( t );
        } else {
            meshingResolution = m_viewportMeshingResolution.at_time( t );
            vertRefinement = m_viewportVertRefinementIterations.at_time( t );
        }

        meshingResolution = frantic::math::clamp( meshingResolution, std::numeric_limits<float>::min(),
                                                  std::numeric_limits<float>::max() );

        float meshingVoxelLength = 0;
        const int meshingResolutionMode = m_meshingResolutionMode.at_time( t );
        if( meshingResolutionMode == MESHING_RESOLUTION_MODE::SUBDIVIDE_MAX_RADIUS ) {
            meshingVoxelLength = maximumParticleRadius / meshingResolution;
        } else if( meshingResolutionMode == MESHING_RESOLUTION_MODE::VOXEL_LENGTH ) {
            if( useRenderMeshing ) {
                meshingVoxelLength = m_renderVoxelLength.at_time( t );
            } else {
                meshingVoxelLength = m_viewportVoxelLength.at_time( t );
            }
            if( meshingVoxelLength <= 0 ) {
                throw std::runtime_error( "Voxel Length must be greater than zero." );
            }
        } else {
            throw std::runtime_error( "Unrecognized meshingResolutionMode: " +
                                      boost::lexical_cast<std::string>( meshingResolutionMode ) );
        }

        const float compactSupportScale = m_anisotropicRadiusScale.at_time( t );
        const float anisotropyWindowScale = m_anisotropicWindowScale.at_time( t );

        const float kr = m_anisotropicMaxAnisotropy.at_time( t );
        const std::size_t ne = m_anisotropicMinNeighborCount.at_time( t );
        const float implicitThreshold = m_anisotropicIsosurfaceLevel.at_time( t );

        // Calculate anisotropy matrices
        tracker.reset( 5, 10 );
        calculate_anisotropy( pa, compactSupportScale, compactSupportScale * anisotropyWindowScale, kr, /*kn, ks,*/ ne,
                              progressLogger );
        progressLogger.update_progress( 100.f );

        // Smooth particle positions
        const float smoothingEffectRadiusScale = m_anisotropicPositionSmoothingWindowScale.at_time( t );
        const float lambda = m_anisotropicPositionSmoothingWeight.at_time( t );
        const bool enableSmoothing = m_anisotropicEnablePositionSmoothing.at_time( t ) && lambda > 0.01f;

        if( enableSmoothing ) {
            tracker.reset( 10, 15 );
            smooth_particle_positions( pa, smoothingEffectRadiusScale * compactSupportScale, lambda, progressLogger );
            progressLogger.update_progress( 100.f );
        }

        // Calculate particle density
        tracker.reset( 15, 20 );
        calculate_volume_with_anisotropic_kernel( pa, volumeChannelName, progressLogger );
        progressLogger.update_progress( 100.f );

        frantic::volumetrics::voxel_coord_system meshingVCS( vector3f( 0 ), meshingVoxelLength );
        if( meshingVCS.voxel_length() == 0 ) {
            return;
        }

        // Build the PGT
        // TODO: calculate the particle density on the pgt instead of hash grid?
        const float particleVoxelLength =
            std::max<float>( maximumParticleRadius * compactSupportScale, 0.5f * meshingVoxelLength );
        voxel_coord_system particleVCS( vector3f( 0 ), particleVoxelLength );
        if( particleVCS.voxel_length() <= 0 ) {
            return;
        }

        if( particleCullingBox ) {
            particleCullingBox->expand( get_max_distance_channel_value( pa ) + 2 * meshingVoxelLength );
        }

        particle_grid_tree particles( pa.get_channel_map(), particleVCS );
        { // scope for radiusAcc and positionAcc
            frantic::channels::channel_accessor<float> radiusAcc =
                pa.get_channel_map().get_accessor<float>( Frost_RADIUS );
            frantic::channels::channel_accessor<frantic::graphics::vector3f> positionAcc =
                pa.get_channel_map().get_accessor<frantic::graphics::vector3f>( Frost_POSITIONCHANNEL );
            std::size_t infinitePositionCount = 0;
            for( frantic::particles::particle_array::iterator i( pa.begin() ); i != pa.end(); ++i ) {
                const char* p = *i;
                const vector3f position = positionAcc( p );
                if( position.is_finite() ) {
                    if( radiusAcc( p ) > 0 ) {
                        bool accept = true;
                        if( particleCullingBox && !particleCullingBox->contains( position ) ) {
                            accept = false;
                        }
                        if( accept ) {
                            particles.insert( p );
                        }
                    }
                } else {
                    ++infinitePositionCount;
                }
            }
            if( infinitePositionCount > 0 ) {
                FF_LOG( warning ) << "Removed " << boost::lexical_cast<frantic::tstring>( infinitePositionCount )
                                  << " particle" << ( infinitePositionCount > 1 ? "s" : "" )
                                  << " with non-finite Position" << std::endl;
            }
        }

        pa.clear();
        pa.trim();

        frantic::graphics::boundbox3f worldParticleBounds = particles.compute_particle_bounds();
        worldParticleBounds.expand( compactSupportScale * maximumParticleRadius );
        const frantic::graphics::boundbox3 voxelParticleBounds = meshingVCS.get_voxel_bounds( worldParticleBounds );
        const bool useSparseMeshing = get_volume_double( voxelParticleBounds ) > SPARSE_MESHING_VOXEL_COUNT_THRESHOLD;

        tracker.reset( 20, 100 );
        if( useSparseMeshing ) {
            progress_logger_mt_gui_adapter uiDelegate( progressLogger );
            frantic::volumetrics::implicitsurface::anisotropic_sparse_params params(
                particles, cpp, implicitThreshold, meshingVCS, vertRefinement, outMesh, *uiDelegate.get_proxy() );
            uiDelegate.run( &anisotropic_sparse_threadproc, &params );
        } else {
            anisotropic_convert_particles_to_trimesh3( particles, cpp, implicitThreshold, meshingVCS, vertRefinement,
                                                       outMesh, progressLogger );
        }

        meshCullingBox = get_roi_box( t );
        if( meshCullingBox ) {
            meshCullingBox = voxelize( *meshCullingBox, meshingVCS );
        }
    } else {
        outMesh.clear();
        throw std::runtime_error( "Unrecognized meshing method: " + boost::lexical_cast<std::string>( meshingMethod ) );
    }

    if( meshCullingBox ) {
        cull( outMesh, *meshCullingBox );
    }

    if( materialMode == MATERIAL_MODE::MTLINDEX_CHANNEL ) {
        if( meshingMethod == MESHING_METHOD::UNION_OF_SPHERES || meshingMethod == MESHING_METHOD::ZHU_BRIDSON ||
            meshingMethod == MESHING_METHOD::METABALLS || meshingMethod == MESHING_METHOD::ANISOTROPIC ) {
            if( outMesh.has_vertex_channel( Frost_PARTICLE_MTLINDEX_CHANNEL_NAME ) ) {
                if( outMesh.has_face_channel( Frost_MESH_MTLINDEX_CHANNEL_NAME ) ) {
                    outMesh.erase_face_channel( Frost_MESH_MTLINDEX_CHANNEL_NAME );
                }
                frantic::geometry::const_trimesh3_vertex_channel_cvt_accessor<boost::int64_t> inAcc =
                    ( (const trimesh3&)outMesh )
                        .get_vertex_channel_cvt_accessor<boost::int64_t>( Frost_PARTICLE_MTLINDEX_CHANNEL_NAME );
                outMesh.add_face_channel<MtlID>( Frost_MESH_MTLINDEX_CHANNEL_NAME );
                frantic::geometry::trimesh3_face_channel_accessor<MtlID> outAcc =
                    outMesh.get_face_channel_accessor<MtlID>( Frost_MESH_MTLINDEX_CHANNEL_NAME );
                for( std::size_t faceIndex = 0; faceIndex < outMesh.face_count(); ++faceIndex ) {
                    const frantic::graphics::vector3 face = inAcc.face( faceIndex );
                    // vote for MtlID.
                    // TODO : use another strategy, like account for max weight on the face ?
                    MtlID a = static_cast<MtlID>( inAcc.get( face.x ) );
                    MtlID b = static_cast<MtlID>( inAcc.get( face.y ) );
                    MtlID c = static_cast<MtlID>( inAcc.get( face.z ) );
                    if( b == c ) {
                        outAcc[faceIndex] = b;
                    } else {
                        outAcc[faceIndex] = a;
                    }
                }
                outMesh.erase_vertex_channel( Frost_PARTICLE_MTLINDEX_CHANNEL_NAME );
            }
        }
    }

    if( outMesh.face_count() > m_displayFaceLimit ) {
        outMesh.set_face_count( m_displayFaceLimit );
    }

    conform_output_mesh_channel_types( outMesh );

    if( m_writeVelocityMapChannel.at_time( t ) ) {
        const frantic::tstring outChannelName =
            _T("Mapping") + boost::lexical_cast<frantic::tstring>( m_velocityMapChannel.at_time( t ) );
        if( outMesh.has_vertex_channel( outChannelName ) ) {
            outMesh.erase_vertex_channel( outChannelName );
        }
        if( outMesh.has_vertex_channel( Frost_VELOCITYCHANNEL ) ) {
            frantic::geometry::trimesh3_vertex_channel_general_accessor inAcc =
                outMesh.get_vertex_channel_general_accessor( Frost_VELOCITYCHANNEL );
            if( inAcc.has_custom_faces() ) {
                throw std::runtime_error( std::string() +
                                          "Error: " + frantic::strings::to_string( Frost_VELOCITYCHANNEL ) +
                                          " channel has custom faces." );
            }
            outMesh.add_vertex_channel_raw( outChannelName, inAcc.arity(), inAcc.data_type() );
            frantic::geometry::trimesh3_vertex_channel_general_accessor outAcc =
                outMesh.get_vertex_channel_general_accessor( outChannelName );
            const std::size_t primitiveSize = inAcc.primitive_size();
            for( std::size_t i = 0; i < outMesh.vertex_count(); ++i ) {
                memcpy( outAcc.data( i ), inAcc.data( i ), primitiveSize );
            }
        } else {
            outMesh.add_vertex_channel<vector3f>( outChannelName );
            frantic::geometry::trimesh3_vertex_channel_accessor<vector3f> outAcc =
                outMesh.get_vertex_channel_accessor<vector3f>( outChannelName );
            for( std::size_t i = 0; i < outMesh.vertex_count(); ++i ) {
                outAcc[i] = vector3f( 0 );
            }
        }
    }

    ++m_buildTrimesh3Count;

    if( frantic::logging::is_logging_debug() && outMesh.has_vertex_channel( _T("Velocity") ) ) {
        frantic::geometry::trimesh3_vertex_channel_general_accessor generalAcc =
            outMesh.get_vertex_channel_general_accessor( _T("Velocity") );
        warn_if_infinite( generalAcc.data_type(), generalAcc.data( 0 ), generalAcc.size() * generalAcc.arity(),
                          "Infinite Velocity in pre-transform output mesh." );
    }

    INode* thisNode = get_inode();
    if( thisNode ) {
        Matrix3 tm = thisNode->GetNodeTM( t );
        outMesh.transform( from_max_t( Inverse( tm ) ) );
    }

    if( outMesh.has_vertex_channel( _T("Velocity") ) ) {
        frantic::geometry::trimesh3_vertex_channel_general_accessor generalAcc =
            outMesh.get_vertex_channel_general_accessor( _T("Velocity") );
        warn_if_infinite( generalAcc.data_type(), generalAcc.data( 0 ), generalAcc.size() * generalAcc.arity(),
                          "Infinite Velocity in output mesh." );
    }

    progressLogger.update_progress( 100.0 );
}

#ifdef FROST_LEVEL_SET

void Frost::BuildLevelSet( TimeValue t, Matrix3 tm, rle_level_set& outRLS ) {
    try {
        outRLS.clear();

        // fetch paramaters and set up the level set vcs
        float scale;

        if( m_useCustomScale.at_time( t ) ) {
            scale = m_customScale.at_time( t );
        } else {
            // m_particleScaleFactor is applied to the particle_istream from files
            // already
            scale = 1.f;
        }

        boost::shared_ptr<particle_istream> pin = GetParticleIStream( t );

        // Attach the input transform to the particle stream and also scale the particles according
        // to the given custom level set scale.
        pin.reset( new streams::transformed_particle_istream( pin, transform4f::from_scale( 1.0f / scale ) *
                                                                       transform4f( tm ) ) );
        // Radius scaling is part of transformed_particle_istream now
        // pin.reset( new streams::channel_scale_particle_istream( pin, std::string(Frost_RADIUS), 1.0f/scale));

        particle_array pa( pin->get_channel_map() );
        pa.insert_particles( pin );

        std::pair<float, float> particleRadiusExtrema = get_particle_radius_extrema( t, pa );
        const float minimumParticleRadius = particleRadiusExtrema.first;
        const float maximumParticleRadius = particleRadiusExtrema.second;

        pin.reset( new particle_array_particle_istream( pa ) );
        // make sure radius, position, and velocity are of type float32, as
        // required by particles_to_level_set or by the ls mesher
        frantic::channels::channel_map newParticleChannelMap;
        for( size_t i = 0; i < pin->get_channel_map().channel_count(); ++i ) {
            const frantic::channels::channel ch = pin->get_channel_map()[i];
            const std::string& channelName = ch.name();

            if( channelName == Frost_RADIUS ) {
                newParticleChannelMap.define_channel( channelName, ch.arity(), frantic::channels::data_type_float32 );
            } else if( channelName == Frost_POSITIONCHANNEL ) {
                newParticleChannelMap.define_channel( channelName, ch.arity(), frantic::channels::data_type_float32 );
            } else if( channelName == Frost_VELOCITY ) {
                newParticleChannelMap.define_channel( channelName, ch.arity(), frantic::channels::data_type_float32 );
            } else {
                newParticleChannelMap.define_channel( channelName, ch.arity(), ch.data_type() );
            }
        }
        newParticleChannelMap.end_channel_definition();
        pin->set_channel_map( newParticleChannelMap );

        float meshingResolution;
        int vertRefinement;

        if( !m_renderUsingViewportSettings.at_time( t ) ) {
            meshingResolution = m_renderMeshingResolution.at_time( t );
            vertRefinement = m_renderVertRefinementIterations.at_time( t );
        } else {
            meshingResolution = m_viewportMeshingResolution.at_time( t );
            vertRefinement = m_viewportVertRefinementIterations.at_time( t );
        }

        float meshingVoxelLength;
        vector3f origin;

        float maximumParticleRadiusForVoxelLength = maximumParticleRadius;
        if( m_useParticleRadiusAsMaxFileRadius.at_time( t ) ) {
            maximumParticleRadiusForVoxelLength = m_radius.at_time( t );
            if( maximumParticleRadiusForVoxelLength < maximumParticleRadius ) {
                TCHAR* buf;
                buf = FormatUniverseValue( maximumParticleRadius );
                throw std::runtime_error(
                    "Maximum Radius must be greater than or equal to the largest particle size, which is " +
                    std::string( buf ) );
            }
        } else {
            maximumParticleRadiusForVoxelLength = maximumParticleRadius;
        }

        if( m_useCustomVoxelLength.at_time( t ) ) {
            meshingVoxelLength = m_customVoxelLength.at_time( t );
        } else {
            meshingVoxelLength = maximumParticleRadiusForVoxelLength / meshingResolution;
        }

        if( m_useCustomOrigin.at_time( t ) )
            origin =
                vector3f( m_customOriginX.at_time( t ), m_customOriginY.at_time( t ), m_customOriginZ.at_time( t ) );
        else
            origin = vector3f( 0 );

        voxel_coord_system meshingVCS( origin, meshingVoxelLength );

        outRLS = rle_level_set( meshingVCS, rle_index_spec(), vector<float>(), 0, 0 );

        if( pin->particle_count() == 0 )
            return;

        frantic::channels::channel_propagation_policy cpp;

        const int meshingMethod = get_meshing_method( t, !m_renderUsingViewportSettings.at_time( t ) );

        if( meshingMethod == MESHING_METHOD::UNION_OF_SPHERES ) {

            // defaults
            // This used to be 2.  The extra logic is to get closer bounds at higher resolutions.
            float effectRadiusScale =
                ( minimumParticleRadius == 0 )
                    ? 2.f
                    : std::min( 2.f, 1.f + 2.f * meshingVCS.voxel_length() / minimumParticleRadius );
            float implicitThreshold = 2.f * meshingVCS.voxel_length();

            voxel_coord_system particleVCS( vector3f( 0 ), maximumParticleRadius * effectRadiusScale );
            particle_grid_tree particles( pin->get_channel_map(), particleVCS );
            particles.insert_particles( pin );
            pin.reset();
            pa.clear();
            pa.trim();

            union_of_spheres_convert_particles_to_level_set( particles, cpp, maximumParticleRadius, effectRadiusScale,
                                                             implicitThreshold, meshingVCS, outRLS );

        } else if( meshingMethod == MESHING_METHOD_ZHU_BRIDSON ) {
            float effectRadius = m_zhuBridsonBlendRadiusScale.at_time( t );
            float lowDensityTrimmingDensity = m_zhuBridsonLowDensityTrimmingThreshold.at_time( t );
            float lowDensityTrimmingStrength = m_zhuBridsonLowDensityTrimmingStrength.at_time( t ) / scale;

            voxel_coord_system particleVCS( vector3f( 0 ), maximumParticleRadius * effectRadius );
            particle_grid_tree particles( pin->get_channel_map(), particleVCS );
            particles.insert_particles( pin );
            pin.reset();
            pa.clear();
            pa.trim();

            zhu_bridson_convert_particles_to_level_set( particles, cpp, maximumParticleRadiusForVoxelLength,
                                                        effectRadius, lowDensityTrimmingDensity,
                                                        lowDensityTrimmingStrength, meshingVCS, outRLS );

        } else if( meshingMethod == MESHING_METHOD_METABALLS ) {
            float effectRadiusScale = m_metaballRadiusScale.at_time( t );
            float implicitThreshold = m_metaballIsosurfaceLevel.at_time( t );

            voxel_coord_system particleVCS( vector3f( 0 ), maximumParticleRadius * effectRadiusScale );
            particle_grid_tree particles( pin->get_channel_map(), particleVCS );
            particles.insert_particles( pin );
            pin.reset();
            pa.clear();
            pa.trim();

            metaball_convert_particles_to_level_set( particles, cpp, maximumParticleRadius, effectRadiusScale,
                                                     implicitThreshold, meshingVCS, outRLS );

        } else {
            outRLS.clear();
            throw std::runtime_error( "Cannot create level set for meshing method: " +
                                      boost::lexical_cast<std::string>( meshingMethod ) );
        }

        if( m_trimToInterface.at_time( t ) ) {
            outRLS.tag_interface_voxels( "Populated" );
            outRLS.trim_to_populated( "Populated" );
            outRLS.erase_channel( "Populated" );
        }

        if( outRLS.has_channel( Frost_VELOCITY ) ) {
            float velocityScaleFloat;
            Interval interval;
            m_velocityScale->GetValue( t, &velocityScaleFloat, interval );
            const double velocityScale = static_cast<double>( velocityScaleFloat );

            rle_channel_general_accessor ca = outRLS.get_channel_general_accessor( Frost_VELOCITY );
            if( ca.arity() != 3 ) {
                throw std::runtime_error(
                    "The velocity channel of the output level set has an arity different from 3." );
            }

            channel_scale_function_t applyScale = channel_scale_function( ca.data_type() );
            for( size_t i = 0; i < ca.size(); ++i ) {
                applyScale( velocityScale, ca.data( i ), 3, ca.data( i ) );
            }
        }

    } catch( const std::exception& e ) {
        // TODO: Need a log window to print this to...
        const frantic::tstring errmsg = _T("BuildLevelSet: %s\n") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        report_error( errmsg );
        // mprintf( "Frost: BuildLevelSet:\n%s\n", e.what() );
        // FF_LOG( error ) << "BuildLevelSet: " << e.what() << std::endl;
        clear_mesh( mesh );
        ivalid.SetInstant( t );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

#endif

bool Frost::get_start_frame( float& out ) {
    validate_source_sequences( GetCOREInterface()->GetTime() );

    std::vector<double> mins;

    for( int i = 0; i < (int)( m_fileSequences.size() ); ++i ) {
        const frantic::files::frame_set& fs = m_fileSequences[i].get_frame_set();
        if( !fs.empty() ) {
            double mn = fs.allframe_range().first;
            mins.push_back( mn );
        }
    }

    std::vector<double>::const_iterator elem( std::max_element( mins.begin(), mins.end() ) );

    if( elem == mins.end() ) {
        out = 0;
        return false;
    } else {
        out = static_cast<float>( *elem );
        return true;
    }
}

bool Frost::get_end_frame( float& out ) {
    validate_source_sequences( GetCOREInterface()->GetTime() );

    std::vector<double> maxes;

    for( int i = 0; i < (int)( m_fileSequences.size() ); ++i ) {
        const frantic::files::frame_set& fs = m_fileSequences[i].get_frame_set();
        if( !fs.empty() ) {
            double mx = m_fileSequences[i].get_frame_set().allframe_range().second;
            maxes.push_back( mx );
        }
    }

    std::vector<double>::const_iterator elem( std::min_element( maxes.begin(), maxes.end() ) );

    if( elem == maxes.end() ) {
        out = 0;
        return false;
    } else {
        out = static_cast<float>( *elem );
        return true;
    }
}

TimeValue Frost::round_to_nearest_wholeframe( TimeValue t ) const {
    const TimeValue tpf = GetTicksPerFrame();

    if( t >= 0 ) {
        return ( ( t + tpf / 2 ) / tpf ) * tpf;
    } else {
        return ( ( t - tpf / 2 + 1 ) / tpf ) * tpf;
    }
}

void Frost::fill_channel_map_cache( TimeValue t ) {
    if( t != m_cachedNativeChannelMapsTime ) {
        // eww, refresh the particle map cache
        std::vector<boost::shared_ptr<frantic::particles::streams::particle_istream>> pins;
        std::vector<frantic::tstring> pinNames;
        get_particle_streams( t, get_default_camera(), pins, pinNames, false );
    }
}

void Frost::validate_channel_map_caches( boost::shared_ptr<particle_istream>& pin ) {
    ::get_vector_channel_names( pin, m_vectorChannelNameCache );
    m_isVectorChannelNameCacheDirty = false;
}

float Frost::get_radius_random_variation_fraction( TimeValue t ) {
    return 0.01f * m_radiusRandomVariation.at_time( t );
}

float Frost::GetMaxParticleRadius( FPTimeValue t ) {
    try {
        return get_particle_radius_extrema( t ).second;
    } catch( const std::exception& e ) {
        // TODO: Need a log window to print this to...
        const frantic::tstring errmsg =
            _T("GetMaxParticleRadius: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        clear_mesh( mesh );
        report_error( errmsg );
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
    return 0;
}

std::pair<float, float> Frost::get_particle_radius_extrema( FPTimeValue t ) {
    // null istream so get_particle_radius_extrema requests a stream if necessary
    boost::shared_ptr<frantic::particles::streams::particle_istream> nullStream;
    return get_particle_radius_extrema( t, nullStream );
}

std::pair<float, float> Frost::get_particle_radius_extrema( FPTimeValue t,
                                                            const frantic::particles::particle_array& pa ) {
    boost::shared_ptr<frantic::particles::streams::particle_istream> pin(
        new frantic::particles::streams::particle_array_particle_istream( pa ) );
    return get_particle_radius_extrema( t, pin );
}

std::pair<float, float>
Frost::get_particle_radius_extrema( FPTimeValue t,
                                    boost::shared_ptr<frantic::particles::streams::particle_istream> pin ) {
    if( ( m_fileList.size() == 0 && !has_scene_particle_source( t ) ) ||
        ( !m_useRadiusChannel.at_time( t ) && !m_enableRadiusScale.at_time( t ) ) ) {
        const float r = m_radius.at_time( t );
        if( m_randomizeRadius.at_time( t ) ) {
            const float minFraction = 1.f - get_radius_random_variation_fraction( t );
            return std::make_pair( minFraction * r, r );
        } else {
            return std::make_pair( r, r );
        }
    } else {
        if( !pin ) {
            pin = GetParticleIStream( t, get_default_camera() );
        }

        if( pin->particle_count() == 0 ) {
            return std::make_pair( 0.f, 0.f );
        }
        channel_map pcm = pin->get_channel_map();
        if( !pcm.has_channel( Frost_RADIUS ) ) {
            return std::make_pair( 0.f, 0.f );
        }
        channel_const_cvt_accessor<float> radius = pcm.get_const_cvt_accessor<float>( Frost_RADIUS );
        vector<char> p( pcm.structure_size() );

        if( !pin->get_particle( p ) ) {
            return std::make_pair( 0.f, 0.f );
        }
        float minRadius = radius( p );
        float maxRadius = radius( p );
        while( pin->get_particle( p ) ) {
            const float r = radius( p );
            if( r > maxRadius ) {
                maxRadius = r;
            }
            if( r < minRadius ) {
                minRadius = r;
            }
        }

        return std::make_pair( minRadius, maxRadius );
    }
}

double Frost::get_file_particle_scale_factor( TimeValue t ) {
    const int unit = m_fileLengthUnit.at_time( t );

    switch( unit ) {
    case FILE_LENGTH_UNIT::GENERIC:
        return 1.0;
    case FILE_LENGTH_UNIT::INCHES:
        return 1.0 / get_system_scale_checked( UNITS_INCHES );
    case FILE_LENGTH_UNIT::FEET:
        return 1.0 / get_system_scale_checked( UNITS_FEET );
    case FILE_LENGTH_UNIT::MILES:
        return 1.0 / get_system_scale_checked( UNITS_MILES );
    case FILE_LENGTH_UNIT::MILLIMETERS:
        return 1.0 / get_system_scale_checked( UNITS_MILLIMETERS );
    case FILE_LENGTH_UNIT::CENTIMETERS:
        return 1.0 / get_system_scale_checked( UNITS_CENTIMETERS );
    case FILE_LENGTH_UNIT::METERS:
        return 1.0 / get_system_scale_checked( UNITS_METERS );
    case FILE_LENGTH_UNIT::KILOMETERS:
        return 1.0 / get_system_scale_checked( UNITS_KILOMETERS );
    case FILE_LENGTH_UNIT::CUSTOM:
        return m_fileCustomScale.at_time( t );
    default:
        throw std::runtime_error( "Unrecognized length unit: " + unit );
    }
}

void Frost::report_warning( const frantic::tstring& msg ) {
    try {
        FF_LOG( warning ) << msg << std::endl;
    } catch( std::exception& /*e*/ ) {
    }
}

void Frost::report_error( const frantic::tstring& msg ) {
    try {
        FF_LOG( error ) << msg << std::endl;
    } catch( std::exception& /*e*/ ) {
    }
}

__forceinline Mesh& Frost::get_active_mesh() { return mesh; }

__forceinline Mesh& Frost::get_inactive_mesh() { return mesh; }
const frantic::channels::channel_map& Frost::get_default_channel_map() { return m_defaultChannelMap; }

const frantic::channels::channel_map& Frost::get_default_krakatoa_channel_map() { return m_defaultKrakatoaChannelMap; }

const frantic::channels::channel_map& Frost::get_empty_channel_map() { return m_emptyChannelMap; }

void Frost::clear_particle_cache() {
    m_nodeParticleCache.clear();
    m_fileParticleCache.clear();
}

frantic::tstring Frost::get_node_name() {
    frantic::tstring name = _T("<error>");

    ULONG handle = 0;
    this->NotifyDependents( FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE );

    INode* node = 0;
    if( handle ) {
        node = GetCOREInterface()->GetINodeByHandle( handle );
    }
    if( node ) {
        const TCHAR* pName = node->GetName();
        if( pName ) {
            name = pName;
        } else {
            name = _T("<null>");
        }
    } else {
        name = _T("<missing>");
    }
    return name;
}

bool Frost::use_vray_instancing() {
    return m_useRenderInstancing.at_time( 0 ) && m_meshingMethod.at_time( 0 ) == MESHING_METHOD::GEOMETRY &&
           m_enableRenderMesh.at_time( 0 );
}

frantic::particles::particle_istream_ptr
Frost::broad_cull_anisotropic_particles( TimeValue t, frantic::particles::particle_istream_ptr pin,
                                         const boost::optional<frantic::graphics::boundbox3f>& bounds ) {
    if( bounds ) {
        const frantic::channels::channel_map channelMap = pin->get_channel_map();

        // Copy for getting the radius extrema.  TODO: improve this!
        frantic::particles::particle_array allParticles( channelMap );
        allParticles.insert_particles( pin );

        std::pair<float, float> particleRadiusExtrema = get_particle_radius_extrema( t, allParticles );
        const float maxRadius = particleRadiusExtrema.second;

        const float maxAnisotropy = m_anisotropicMaxAnisotropy.at_time( t );
        const float compactSupportScale = m_anisotropicRadiusScale.at_time( t );

        const float maxAnisotropicDistance =
            std::max( maxAnisotropy ? 1.33f / maxAnisotropy : 1, 1 / 0.15f ) * compactSupportScale * maxRadius;

        const float smoothingEffectRadiusScale = m_anisotropicPositionSmoothingWindowScale.at_time( t );
        const float lambda = m_anisotropicPositionSmoothingWeight.at_time( t );
        const bool enableSmoothing = m_anisotropicEnablePositionSmoothing.at_time( t ) && lambda > 0.01f;

        float smoothingWindowRadius = 0;
        float maxSmoothingDistance = 0;
        if( enableSmoothing ) {
            smoothingWindowRadius = smoothingEffectRadiusScale * compactSupportScale * maxRadius;
            maxSmoothingDistance = lambda * smoothingWindowRadius;
        }

        const float maxDistance = std::max( smoothingWindowRadius, maxAnisotropicDistance ) +
                                  2 * maxAnisotropicDistance + maxSmoothingDistance;

        pin = boost::make_shared<particle_array_particle_istream>( allParticles );

        boundbox3f cullingBounds = *bounds;
        cullingBounds.expand( maxDistance );
        pin.reset( new box_culling_particle_istream( pin, cullingBounds ) );

        boost::shared_ptr<frantic::particles::particle_array> culledParticles =
            boost::make_shared<frantic::particles::particle_array>( channelMap );
        culledParticles->insert_particles( pin );

        pin = boost::make_shared<shared_particle_container_particle_istream<particle_array>>( culledParticles );

        pin->set_channel_map( channelMap );
    }

    return pin;
}
