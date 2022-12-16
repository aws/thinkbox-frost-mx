// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "Frost.hpp"

#include "frost_param_block_desc.hpp"

#include "resource.h"

#include "FrostClassDesc.hpp"
#include "frost_pb_accessor.hpp"

#include "geometry_list_validator.hpp"
#include "node_list_validator.hpp"
#include "orientation_look_at_validator.hpp"

#if MAX_VERSION_MAJOR >= 15
#define P_END p_end
#else
#define P_END end
#endif

static ParamBlockDesc2 frost_param_block_desc(
    frost_param_block, _T( "FrostParams" ), 0, GetFrostClassDesc(),
    P_AUTO_CONSTRUCT | P_AUTO_UI | P_MULTIMAP | P_VERSION,
    // version
    8,
    // param block ref number
    frost_param_block,
    // rollouts
    frost_param_map_count, frost_main_param_map, IDD_FROST, IDS_FROST_DIALOG_TITLE, 0, 0, NULL, frost_help_param_map,
    IDD_HELP, IDS_HELP_DIALOG_TITLE, 0, APPENDROLL_CLOSED, NULL, frost_presets_param_map, IDD_PRESETS,
    IDS_PRESETS_DIALOG_TITLE, 0, APPENDROLL_CLOSED, NULL, frost_particle_objects_param_map, IDD_PARTICLE_OBJECTS,
    IDS_PARTICLE_OBJECTS_DIALOG_TITLE, 0, 0, NULL, frost_particle_flow_events_param_map, IDD_PARTICLE_GROUPS,
    IDS_PARTICLE_FLOW_DIALOG_TITLE, 0, APPENDROLL_CLOSED, NULL, frost_tp_groups_param_map, IDD_PARTICLE_GROUPS,
    IDS_THINKING_PARTICLES_GROUPS_DIALOG_TITLE, 0, APPENDROLL_CLOSED, NULL, frost_particle_files_param_map,
    IDD_PARTICLE_FILES, IDS_PARTICLE_FILES_DIALOG_TITLE, 0, APPENDROLL_CLOSED, NULL, frost_material_param_map,
    IDD_MATERIAL, IDS_MATERIAL_DIALOG_TITLE, 0, APPENDROLL_CLOSED, NULL, frost_meshing_param_map, IDD_MESHING,
    IDS_MESHING_DIALOG_TITLE, 0, 0, NULL, frost_meshing_quality_param_map, IDD_MESHING_QUALITY,
    IDS_MESHING_QUALITY_DIALOG_TITLE, 0, 0, NULL, frost_metaballs_param_map, IDD_METABALLS, IDS_METABALLS_DIALOG_TITLE,
    0, 0, NULL, frost_zhu_bridson_param_map, IDD_ZHU_BRIDSON, IDS_ZHU_BRIDSON_DIALOG_TITLE, 0, 0, NULL,
    frost_geometry_param_map, IDD_GEOMETRY, IDS_GEOMETRY_DIALOG_TITLE, 0, 0, NULL, frost_anisotropic_param_map,
    IDD_ANISOTROPIC, IDS_ANISOTROPIC_DIALOG_TITLE, 0, 0, NULL, frost_roi_param_map, IDD_ROI, IDS_ROI, 0,
    APPENDROLL_CLOSED, NULL,

    // params for main rollout
    pb_showIcon, _T( "showIcon" ), TYPE_BOOL, P_RESET_DEFAULT | P_ANIMATABLE, IDS_SHOWICON, p_default, TRUE, p_accessor,
    GetFrostPBAccessor(), p_ui, frost_main_param_map, TYPE_SINGLECHEKBOX, CHK_ICON_SHOW, P_END, pb_iconSize,
    _T( "iconSize" ), TYPE_WORLD, P_RESET_DEFAULT, IDS_ICONSIZE, p_default, 10.0f, p_range, 0.0f,
    std::numeric_limits<float>::max(), p_accessor, GetFrostPBAccessor(), p_ui, frost_main_param_map, TYPE_SPINNER,
    EDITTYPE_UNIVERSE, IDC_ICON_SIZE, IDC_ICON_SIZE_SPIN, SPIN_AUTOSCALE, P_END,

    pb_updateOnFrostChange, _T( "updateOnFrostChange" ), TYPE_BOOL, P_RESET_DEFAULT, IDS_UPDATEONFROSTCHANGE, p_default,
    TRUE, p_accessor, GetFrostPBAccessor(), p_ui, frost_main_param_map, TYPE_SINGLECHEKBOX,
    IDC_VIEWPORT_UPDATE_ON_FROST_CHANGE, P_END, pb_updateOnParticleChange, _T( "updateOnParticleChange" ), TYPE_BOOL,
    P_RESET_DEFAULT, IDS_UPDATEONPARTICLECHANGE, p_default, FALSE, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_main_param_map, TYPE_SINGLECHEKBOX, IDC_VIEWPORT_UPDATE_ON_PARTICLE_CHANGE, P_END,

    // params for particle objects
    pb_nodeList, _T( "nodeList" ), TYPE_INODE_TAB, 0, P_VARIABLE_SIZE, IDS_NODELIST, p_accessor, GetFrostPBAccessor(),
    p_validator, GetNodeListValidator(), P_END,

    // params for particle flow events
    pb_useAllPFEvents, _T( "useAllPFEvents" ), TYPE_BOOL, P_RESET_DEFAULT | P_OBSOLETE, IDS_USEALLPFEVENTS, p_default,
    TRUE, p_accessor, GetFrostPBAccessor(), p_ui, frost_particle_flow_events_param_map, TYPE_SINGLECHEKBOX,
    IDC_USE_ALL_PARTICLE_FLOW_EVENTS, P_END, pb_pfEventList, _T( "pfEventList" ), TYPE_INODE_TAB, 0, P_VARIABLE_SIZE,
    IDS_PFEVENTLIST, p_accessor, GetFrostPBAccessor(), P_END,

    // params for particle files
    pb_fileList, _T( "fileList" ), TYPE_STRING_TAB, 0, P_VARIABLE_SIZE, IDS_FILELIST, p_accessor, GetFrostPBAccessor(),
    P_END, pb_loadSingleFrame, _T("loadSingleFrame"), TYPE_BOOL, P_RESET_DEFAULT, IDS_LOADSINGLEFRAME, p_default, FALSE,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_particle_files_param_map, TYPE_SINGLECHEKBOX, IDC_LOAD_SINGLE_FRAME,
    P_END, pb_frameOffset, _T("frameOffset"), TYPE_INT, P_RESET_DEFAULT, IDS_FRAMEOFFSET, p_default, 0, p_range,
    -100000, 100000, p_accessor, GetFrostPBAccessor(), p_ui, frost_particle_files_param_map, TYPE_SPINNER, EDITTYPE_INT,
    IDC_FRAME_OFFSET, IDC_FRAME_OFFSET_SPIN, 1.f, P_END, pb_limitToRange, _T("limitToRange"), TYPE_BOOL,
    P_RESET_DEFAULT, IDS_LIMITTORANGE, p_default, FALSE, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_particle_files_param_map, TYPE_SINGLECHEKBOX, IDC_LIMIT_TO_RANGE, P_END, pb_rangeFirstFrame,
    _T("rangeStartFrame"), TYPE_INT, P_RESET_DEFAULT, IDS_RANGESTARTFRAME, p_default, 0, p_enabled, FALSE, p_range,
    -100000, 100000, p_accessor, GetFrostPBAccessor(), p_ui, frost_particle_files_param_map, TYPE_SPINNER, EDITTYPE_INT,
    IDC_FIRST_FRAME, IDC_FIRST_FRAME_SPIN, 1.f, P_END, pb_rangeLastFrame, _T("rangeEndFrame"), TYPE_INT,
    P_RESET_DEFAULT, IDS_RANGEENDFRAME, p_default, 100, p_enabled, FALSE, p_range, -100000, 100000, p_accessor,
    GetFrostPBAccessor(), p_ui, frost_particle_files_param_map, TYPE_SPINNER, EDITTYPE_INT, IDC_LAST_FRAME,
    IDC_LAST_FRAME_SPIN, 1.f, P_END, pb_enablePlaybackGraph, _T("enablePlaybackGraph"), TYPE_BOOL, P_RESET_DEFAULT,
    IDS_ENABLEPLAYBACKGRAPH, p_default, FALSE, p_accessor, GetFrostPBAccessor(), p_ui, frost_particle_files_param_map,
    TYPE_SINGLECHEKBOX, IDC_ENABLE_PLAYBACK_GRAPH, P_END, pb_playbackGraphTime, _T("playbackGraphTime"), TYPE_FLOAT,
    P_ANIMATABLE + P_RESET_DEFAULT, IDS_PLAYBACKGRAPHTIME, p_default, 0.0f, p_range, -std::numeric_limits<float>::max(),
    std::numeric_limits<float>::max(), p_accessor, GetFrostPBAccessor(), p_ui, frost_particle_files_param_map,
    TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PLAYBACK_GRAPH, IDC_PLAYBACK_GRAPH_SPIN, 0.1f, P_END, pb_beforeRangeBehavior,
    _T("beforeRangeBehavior"), TYPE_INT, P_RESET_DEFAULT, IDS_BEFORERANGEBEHAVIOR, p_default,
    DEFAULT_FRAME_RANGE_CLAMP_MODE, p_range, 0, 1, p_accessor, GetFrostPBAccessor(), P_END, pb_afterRangeBehavior,
    _T("afterRangeBehavior"), TYPE_INT, P_RESET_DEFAULT, IDS_AFTERRANGEBEHAVIOR, p_default,
    DEFAULT_FRAME_RANGE_CLAMP_MODE, p_range, 0, 1, p_accessor, GetFrostPBAccessor(), P_END, pb_fileLengthUnit,
    _T("fileLengthUnit"), TYPE_INT, P_RESET_DEFAULT, IDS_FILELENGTHUNIT, p_default, DEFAULT_FILE_LENGTH_UNIT,
    p_accessor, GetFrostPBAccessor(), P_END, pb_fileCustomScale, _T("fileCustomScale"), TYPE_FLOAT, 0,
    IDS_FILECUSTOMSCALE, p_default, 39.3701f, p_enabled, FALSE, p_range, 0.f, 100000.f, p_accessor,
    GetFrostPBAccessor(), p_ui, frost_particle_files_param_map, TYPE_SPINNER, EDITTYPE_FLOAT,
    IDC_PARTICLE_FILES_CUSTOM_SCALE, IDC_PARTICLE_FILES_CUSTOM_SCALE_SPIN, SPIN_AUTOSCALE, P_END,

    // meshing method
    pb_meshingMethod, _T( "meshingMethod" ), TYPE_INT, P_RESET_DEFAULT, IDS_MESHINGMETHOD, p_default,
    DEFAULT_MESHING_METHOD, p_accessor, GetFrostPBAccessor(), P_END, pb_enableRenderMesh, _T( "enableRenderMesh" ),
    TYPE_BOOL, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ENABLERENDERMESH, p_default, TRUE, p_accessor, GetFrostPBAccessor(),
    p_ui, frost_meshing_param_map, TYPE_SINGLECHEKBOX, IDC_ENABLE_RENDER_MESH, P_END, pb_enableViewportMesh,
    _T( "enableViewportMesh" ), TYPE_BOOL, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ENABLEVIEWPORTMESH, p_default, TRUE,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_meshing_param_map, TYPE_SINGLECHEKBOX, IDC_ENABLE_VIEWPORT_MESH,
    P_END, pb_radius, _T( "radius" ), TYPE_WORLD, P_RESET_DEFAULT | P_ANIMATABLE, IDS_RADIUS, p_default, 5.f, p_range,
    0.0001f, 99999.f, p_accessor, GetFrostPBAccessor(), p_ui, frost_meshing_param_map, TYPE_SPINNER, EDITTYPE_UNIVERSE,
    IDC_RADIUS, IDC_RADIUS_SPIN, SPIN_AUTOSCALE, P_END, pb_useRadiusChannel, _T( "useRadiusChannel" ), TYPE_BOOL,
    P_RESET_DEFAULT, IDS_USERADIUSCHANNEL, p_default, FALSE, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_meshing_param_map, TYPE_SINGLECHEKBOX, IDC_USE_RADIUS_CHANNEL, P_END, pb_randomizeRadius,
    _T( "randomizeRadius" ), TYPE_BOOL, P_RESET_DEFAULT, IDS_RANDOMIZERADIUS, p_default, FALSE, p_accessor,
    GetFrostPBAccessor(), p_ui, frost_meshing_param_map, TYPE_SINGLECHEKBOX, IDC_RANDOMIZE_RADIUS, P_END,
    pb_radiusRandomVariation, _T( "radiusRandomVariation" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_RADIUSRANDOMVARIATION, p_default, 40.f, p_range, 0.f, 99.f, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_meshing_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RADIUS_RANDOM_VARIATION,
    IDC_RADIUS_RANDOM_VARIATION_SPIN, SPIN_AUTOSCALE, P_END, pb_radiusSeed, _T( "radiusRandomSeed" ), TYPE_INT,
    P_RESET_DEFAULT, IDS_RADIUSRANDOMSEED, p_default, int( 12345 ), p_range, 0, int( 2000000000 ), p_accessor,
    GetFrostPBAccessor(), p_ui, frost_meshing_param_map, TYPE_SPINNER, EDITTYPE_INT, IDC_RADIUS_RANDOM_SEED,
    IDC_RADIUS_RANDOM_SEED_SPIN, 1.f, P_END, pb_motionBlurMode, _T( "motionBlurMode" ), TYPE_INT, P_RESET_DEFAULT,
    IDS_MOTIONBLURMODE, p_default, DEFAULT_MOTION_BLUR_MODE, p_accessor, GetFrostPBAccessor(), P_END,

    // meshing quality
    pb_renderUsingViewportSettings, _T( "renderUsingViewportSettings" ), TYPE_BOOL, P_RESET_DEFAULT,
    IDS_RENDERUSINGVIEWPORTSETTINGS, p_default, FALSE, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_meshing_quality_param_map, TYPE_SINGLECHEKBOX, IDC_RENDER_USING_VIEWPORT_SETTINGS, P_END,
    pb_renderMeshingResolution, _T( "renderMeshingResolution" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_RENDERMESHINGRESOLUTION, p_default, 3.f, p_range, 0.01f, 99999.f, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_meshing_quality_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RENDER_MESHING_RES, IDC_RENDER_MESHING_RES_SPIN,
    SPIN_AUTOSCALE, P_END, pb_renderVertRefinementIterations, _T( "renderVertRefinementIterations" ), TYPE_INT,
    P_RESET_DEFAULT, IDS_RENDERVERTREFINEMENTITERATIONS, p_default, (int)10, p_range, (int)0, (int)20, p_accessor,
    GetFrostPBAccessor(), p_ui, frost_meshing_quality_param_map, TYPE_SPINNER, EDITTYPE_INT, IDC_RENDER_VERT_REFINE,
    IDC_RENDER_VERT_REFINE_SPIN, 1.f, P_END,

    pb_previewAsGeometry, _T( "" ), TYPE_BOOL, P_RESET_DEFAULT, IDS_PREVIEWASGEOMETRY, p_default, FALSE, p_accessor,
    GetFrostPBAccessor(),
    // p_ui,		frost_meshing_param_map, TYPE_SINGLECHEKBOX, IDC_PREVIEW_AS_GEOMETRY,
    P_END, pb_viewportMeshingResolution, _T( "viewportMeshingResolution" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_VIEWPORTMESHINGRESOLUTION, p_default, 1.5f, p_range, 0.01f, 99999.f, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_meshing_quality_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_VIEWPORT_MESHING_RES,
    IDC_VIEWPORT_MESHING_RES_SPIN, SPIN_AUTOSCALE, P_END, pb_viewportVertRefinementIterations,
    _T( "viewportVertRefinementIterations" ), TYPE_INT, P_RESET_DEFAULT, IDS_VIEWPORTVERTREFINEMENTITERATIONS,
    p_default, (int)0, p_range, (int)0, (int)20, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_meshing_quality_param_map, TYPE_SPINNER, EDITTYPE_INT, IDC_VIEWPORT_VERT_REFINE,
    IDC_VIEWPORT_VERT_REFINE_SPIN, 1.f, P_END,

    // metaballs
    pb_metaballRadiusScale, _T( "metaballRadiusScale" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_METABALLRADIUSSCALE, p_default, 1.5f, p_range, 0.f, 9999.f, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_metaballs_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_METABALL_RADIUS_SCALE, IDC_METABALL_RADIUS_SCALE_SPIN,
    SPIN_AUTOSCALE, P_END, pb_metaballIsosurfaceLevel, _T( "metaballIsosurfaceLevel" ), TYPE_FLOAT,
    P_RESET_DEFAULT | P_ANIMATABLE, IDS_METABALLISOSURFACELEVEL, p_default, 0.3f, p_range, 0.01f, 50.f, p_accessor,
    GetFrostPBAccessor(), p_ui, frost_metaballs_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_METABALL_ISOSURFACE_LEVEL,
    IDC_METABALL_ISOSURFACE_LEVEL_SPIN, SPIN_AUTOSCALE, P_END,

    // zhu bridson
    pb_zhuBridsonBlendRadiusScale, _T( "zhuBridsonBlendRadiusScale" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_ZHUBRIDSONBLENDRADIUSSCALE, p_default, 1.7f, p_range, 1.1f, 9999.f, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_zhu_bridson_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_ZHU_BRIDSON_BLEND_RADIUS,
    IDC_ZHU_BRIDSON_BLEND_RADIUS_SPIN, SPIN_AUTOSCALE, P_END, pb_zhuBridsonEnableLowDensityTrimming,
    _T( "zhuBridsonEnableLowDensityTrimming" ), TYPE_BOOL, P_RESET_DEFAULT, IDS_ZHUBRIDSONENABLELOWDENSITYTRIMMING,
    p_default, TRUE, p_accessor, GetFrostPBAccessor(), p_ui, frost_zhu_bridson_param_map, TYPE_SINGLECHEKBOX,
    IDC_ZHU_BRIDSON_ENABLE_LOW_DENSITY_TRIMMING, P_END, pb_zhuBridsonLowDensityTrimmingThreshold,
    _T( "zhuBridsonLowDensityTrimmingThreshold" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_ZHUBRIDSONLOWDENSITYTRIMMINGTHRESHOLD, p_default, 1.f, p_range, 0.01f, 9999.f, p_accessor, GetFrostPBAccessor(),
    p_ui, frost_zhu_bridson_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_ZHU_BRIDSON_LOW_DENSITY_TRIMMING_THRESHOLD,
    IDC_ZHU_BRIDSON_LOW_DENSITY_TRIMMING_THRESHOLD_SPIN, SPIN_AUTOSCALE, P_END, pb_zhuBridsonLowDensityTrimmingStrength,
    _T( "zhuBridsonLowDensityTrimmingStrength" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_ZHUBRIDSONLOWDENSITYTRIMMINGSTRENGTH, p_default, 15.f, p_range, 0.01f, 9999.f, p_accessor, GetFrostPBAccessor(),
    p_ui, frost_zhu_bridson_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_ZHU_BRIDSON_LOW_DENSITY_TRIMMING_STRENGTH,
    IDC_ZHU_BRIDSON_LOW_DENSITY_TRIMMING_STRENGTH_SPIN, SPIN_AUTOSCALE, P_END,

    // custom geometry
    pb_geometryType, _T( "geometryType" ), TYPE_INT, P_RESET_DEFAULT, IDS_GEOMETRYTYPE, p_default,
    DEFAULT_GEOMETRY_TYPE, p_accessor, GetFrostPBAccessor(), P_END, pb_geometryList, _T( "geometryList" ),
    TYPE_INODE_TAB, 0, P_VARIABLE_SIZE, IDS_GEOMETRYLIST, p_accessor, GetFrostPBAccessor(), p_validator,
    GetGeometryListValidator(), P_END, pb_geometrySelectionMode, _T( "geometrySelectionMode" ), TYPE_INT,
    P_RESET_DEFAULT, IDS_GEOMETRYSELECTIONMODE, p_default, DEFAULT_GEOMETRY_SELECTION_MODE, p_accessor,
    GetFrostPBAccessor(), P_END, pb_geometrySelectionSeed, _T( "geometrySelectionSeed" ), TYPE_INT, P_RESET_DEFAULT,
    IDS_GEOMETRYSELECTIONSEED, p_default, int( 12345 ), p_range, int( 0 ), int( 2000000000 ), p_accessor,
    GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SPINNER, EDITTYPE_INT, IDC_GEOMETRY_SELECTION_SEED,
    IDC_GEOMETRY_SELECTION_SEED_SPIN, 1.f, P_END,

    pb_geometrySampleTimeOffsetMode, _T( "geometrySampleTimeOffsetMode" ), TYPE_INT, P_RESET_DEFAULT,
    IDS_GEOMETRYSAMPLETIMEOFFSETMODE, p_default, DEFAULT_GEOMETRY_TIMING_OFFSET_MODE, p_accessor, GetFrostPBAccessor(),
    P_END, pb_geometrySampleTimeMaxRandomOffset, _T( "geometrySampleTimeMaxRandomOffset" ), TYPE_FLOAT,
    P_RESET_DEFAULT | P_ANIMATABLE, IDS_GEOMETRYSAMPLETIMEMAXRANDOMOFFSET, p_default, 0.f, p_range, 0.f, 999999.f,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SPINNER, EDITTYPE_FLOAT,
    IDC_GEOMETRY_MAX_RANDOM_OFFSET, IDC_GEOMETRY_MAX_RANDOM_OFFSET_SPIN, 1.f, P_END, pb_geometrySampleTimeSeed,
    _T( "geometrySampleTimeSeed" ), TYPE_INT, P_RESET_DEFAULT, IDS_GEOMETRYSAMPLETIMESEED, p_default, int( 12345 ),
    p_range, 0, int( 2000000000 ), p_accessor, GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SPINNER,
    EDITTYPE_INT, IDC_GEOMETRY_SAMPLE_TIME_SEED, IDC_GEOMETRY_SAMPLE_TIME_SEED_SPIN, 1.f, P_END,

    pb_geometryOrientationMode, _T( "geometryOrientationMode" ), TYPE_INT, P_RESET_DEFAULT, IDS_GEOMETRYORIENTATIONMODE,
    p_default, DEFAULT_GEOMETRY_ORIENTATION_MODE, p_accessor, GetFrostPBAccessor(), P_END,
    pb_geometryOrientationLookAtNode, _T( "geometryOrientationLookAtNode" ), TYPE_INODE, 0,
    IDS_GEOMETRYORIENTATIONLOOKATNODE, p_accessor, GetFrostPBAccessor(), p_validator, GetOrientationLookAtValidator(),
    p_ui, frost_geometry_param_map, TYPE_PICKNODEBUTTON, IDC_LOOK_AT_NODE, P_END, pb_geometryOrientationVectorChannel,
    _T( "geometryOrientationVectorChannel" ), TYPE_STRING, 0, IDS_GEOMETRYORIENTATIONVECTORCHANNEL, p_default,
    Frost_POSITIONCHANNEL, p_accessor, GetFrostPBAccessor(), P_END, pb_geometryOrientationX,
    _T( "geometryOrientationX" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE, IDS_GEOMETRYORIENTATIONX, p_default, 0.f,
    p_range, -360.f, 360.f, p_accessor, GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SPINNER,
    EDITTYPE_FLOAT, IDC_GEOMETRY_ORIENTATION_X, IDC_GEOMETRY_ORIENTATION_X_SPIN, 1.f, P_END, pb_geometryOrientationY,
    _T( "geometryOrientationY" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE, IDS_GEOMETRYORIENTATIONY, p_default, 0.f,
    p_range, -360.f, 360.f, p_accessor, GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SPINNER,
    EDITTYPE_FLOAT, IDC_GEOMETRY_ORIENTATION_Y, IDC_GEOMETRY_ORIENTATION_Y_SPIN, 1.f, P_END, pb_geometryOrientationZ,
    _T( "geometryOrientationZ" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE, IDS_GEOMETRYORIENTATIONZ, p_default, 0.f,
    p_range, -360.f, 360.f, p_accessor, GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SPINNER,
    EDITTYPE_FLOAT, IDC_GEOMETRY_ORIENTATION_Z, IDC_GEOMETRY_ORIENTATION_Z_SPIN, 1.f, P_END,

    pb_geometryOrientationDivergence, _T( "geometryOrientationDivergence" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_GEOMETRYORIENTATIONDIVERGENCE, p_default, 0.f, p_range, 0.f, 180.f, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_geometry_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_ORIENTATION_DIVERGENCE, IDC_ORIENTATION_DIVERGENCE_SPIN,
    1.f, P_END, pb_geometryOrientationRestrictDivergenceAxis, _T( "geometryOrientationRestrictDivergenceAxis" ),
    TYPE_BOOL, P_RESET_DEFAULT, IDS_GEOMETRYORIENTATIONRESTRICTDIVERGENCEAXIS, p_default, FALSE, p_accessor,
    GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SINGLECHEKBOX, IDC_RESTRICT_DIVERGENCE_TO_AXIS, P_END,
    pb_geometryOrientationDivergenceAxisSpace, _T( "geometryOrientationDivergenceAxisSpace" ), TYPE_INT,
    P_RESET_DEFAULT, IDS_GEOMETRYORIENTATIONDIVERGENCEAXISSPACE, p_default, DEFAULT_AXIS_SPACE, p_accessor,
    GetFrostPBAccessor(), P_END, pb_geometryOrientationDivergenceAxisX, _T( "geometryOrientationDivergenceAxisX" ),
    TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE, IDS_GEOMETRYORIENTATIONDIVERGENCEAXISX, p_default, 0.f, p_range, -1.f,
    1.f, p_accessor, GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SPINNER, EDITTYPE_FLOAT,
    IDC_DIVERGENCE_X, IDC_DIVERGENCE_X_SPIN, 0.1f, P_END, pb_geometryOrientationDivergenceAxisY,
    _T( "geometryOrientationDivergenceAxisY" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_GEOMETRYORIENTATIONDIVERGENCEAXISY, p_default, 0.f, p_range, -1.f, 1.f, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_geometry_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DIVERGENCE_Y, IDC_DIVERGENCE_Y_SPIN, 0.1f, P_END,
    pb_geometryOrientationDivergenceAxisZ, _T( "geometryOrientationDivergenceAxisZ" ), TYPE_FLOAT,
    P_RESET_DEFAULT | P_ANIMATABLE, IDS_GEOMETRYORIENTATIONDIVERGENCEAXISZ, p_default, 1.f, p_range, -1.f, 1.f,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DIVERGENCE_Z,
    IDC_DIVERGENCE_Z_SPIN, 0.1f, P_END,

    pb_useRenderInstancing, _T( "useRenderInstancing" ), TYPE_BOOL, P_RESET_DEFAULT, IDS_USERENDERINSTANCING, p_default,
    FALSE, p_accessor, GetFrostPBAccessor(), p_ui, frost_geometry_param_map, TYPE_SINGLECHEKBOX,
    IDC_USE_RENDER_INSTANCING, P_END,

    pb_writeVelocityMapChannel, _T( "writeVelocityMapChannel" ), TYPE_BOOL, P_RESET_DEFAULT,
    IDS_WRITEVELOCITYMAPCHANNEL, p_default, FALSE, p_accessor, GetFrostPBAccessor(), p_ui, frost_main_param_map,
    TYPE_SINGLECHEKBOX, IDC_WRITE_VELOCITY_MAP_CHANNEL, P_END, pb_velocityMapChannel, _T( "velocityMapChannel" ),
    TYPE_INT, P_RESET_DEFAULT, IDS_VELOCITYMAPCHANNEL, p_default, 2, p_range, 0, 99, p_accessor, GetFrostPBAccessor(),
    p_ui, frost_main_param_map, TYPE_SPINNER, EDITTYPE_INT, IDC_VELOCITY_MAP_CHANNEL, IDC_VELOCITY_MAP_CHANNEL_SPIN,
    1.f, P_END,

    pb_viewportLoadMode, _T( "viewportLoadMode" ), TYPE_INT, P_RESET_DEFAULT, IDS_VIEWPORTLOADMODE, p_default,
    DEFAULT_VIEWPORT_LOAD_MODE, p_accessor, GetFrostPBAccessor(), P_END, pb_viewportLoadPercent,
    _T( "viewportLoadPercent" ), TYPE_FLOAT, P_RESET_DEFAULT, IDS_VIEWPORTLOADPERCENT, p_default, 100.f, p_range, 0.f,
    100.f, p_accessor, GetFrostPBAccessor(), p_ui, frost_meshing_param_map, TYPE_SPINNER, EDITTYPE_FLOAT,
    IDC_VIEWPORT_PERCENT, IDC_VIEWPORT_PERCENT_SPIN, SPIN_AUTOSCALE, P_END,

    pb_anisotropicRadiusScale, _T( "anisotropicRadiusScale" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_ANISOTROPICRADIUSSCALE, p_default, 4.f, p_range, 0.f, 1000000.f, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_anisotropic_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RADIUS_SCALE, IDC_RADIUS_SCALE_SPIN, SPIN_AUTOSCALE,
    P_END, pb_anisotropicWindowScale, _T( "anisotropicWindowScale" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE,
    IDS_ANISOTROPICWINDOWSCALE, p_default, 2.f, p_range, 1.f, 1000000.f, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_anisotropic_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_WINDOW_SCALE, IDC_WINDOW_SCALE_SPIN, SPIN_AUTOSCALE,
    P_END, pb_anisotropicIsosurfaceLevel, _T( "anisotropicIsosurfaceLevel" ), TYPE_FLOAT,
    P_RESET_DEFAULT | P_ANIMATABLE, IDS_ANISOTROPICISOSURFACELEVEL, p_default, 0.5f, p_range, 0.f, 1000000.f,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_anisotropic_param_map, TYPE_SPINNER, EDITTYPE_FLOAT,
    IDC_IMPLICIT_THRESHOLD, IDC_IMPLICIT_THRESHOLD_SPIN, SPIN_AUTOSCALE, P_END, pb_anisotropicMaxAnisotropy,
    _T( "anisotropicMaxAnisotropy" ), TYPE_FLOAT, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ANISOTROPICMAXANISOTROPY,
    p_default, 4.f, p_range, 1.f, 10.f, p_accessor, GetFrostPBAccessor(), p_ui, frost_anisotropic_param_map,
    TYPE_SPINNER, EDITTYPE_FLOAT, IDC_KR, IDC_KR_SPIN, SPIN_AUTOSCALE, P_END, pb_anisotropicMinNeighborCount,
    _T( "anisotropicMinNeighborCount" ), TYPE_INT, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ANISOTROPICMINNEIGHBORCOUNT,
    p_default, (int)25, p_range, (int)0, (int)1000000, p_accessor, GetFrostPBAccessor(), p_ui,
    frost_anisotropic_param_map, TYPE_SPINNER, EDITTYPE_INT, IDC_NE, IDC_NE_SPIN, SPIN_AUTOSCALE, P_END,
    pb_anisotropicEnablePositionSmoothing, _T( "" /*"anisotropicEnablePositionSmoothing"*/ ), TYPE_BOOL,
    P_RESET_DEFAULT | P_ANIMATABLE, IDS_ANISOTROPICENABLEPOSITIONSMOOTHING, p_default, TRUE, p_accessor,
    GetFrostPBAccessor(),
    // p_ui,		frost_anisotropic_param_map, TYPE_SINGLECHEKBOX, IDC_SMOOTH_PARTICLE_POSITIONS,
    P_END, pb_anisotropicPositionSmoothingWindowScale, _T( "anisotropicPositionSmoothingWindowScale" ), TYPE_FLOAT,
    P_RESET_DEFAULT | P_ANIMATABLE, IDS_ANISOTROPICPOSITIONSMOOTHINGWINDOWSCALE, p_default, 2.f, p_range, 0.f, 100.f,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_anisotropic_param_map, TYPE_SPINNER, EDITTYPE_FLOAT,
    IDC_SMOOTHING_RADIUS_SCALE, IDC_SMOOTHING_RADIUS_SCALE_SPIN, SPIN_AUTOSCALE, P_END,
    pb_anisotropicPositionSmoothingWeight, _T( "anisotropicPositionSmoothingWeight" ), TYPE_FLOAT,
    P_RESET_DEFAULT | P_ANIMATABLE, IDS_ANISOTROPICPOSITIONSMOOTHINGWEIGHT, p_default, 0.9f, p_range, 0.f, 1.f,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_anisotropic_param_map, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_LAMBDA,
    IDC_LAMBDA_SPIN, SPIN_AUTOSCALE, P_END,

    pb_pfEventFilterMode, _T( "pfEventFilterMode" ), TYPE_INT, P_RESET_DEFAULT, IDS_PFEVENTFILTERMODE, p_default,
    DEFAULT_PARTICLE_GROUP_FILTER_MODE, p_accessor, GetFrostPBAccessor(), p_ui, frost_particle_flow_events_param_map,
    TYPE_RADIO, 2, IDC_FILTER_MODE_ALL, IDC_FILTER_MODE_SELECTED, p_vals, ( PARTICLE_GROUP_FILTER_MODE::ALL ),
    ( PARTICLE_GROUP_FILTER_MODE::SELECTED ), P_END,

    pb_materialMode, _T( "materialMode" ), TYPE_INT, P_RESET_DEFAULT, IDS_MATERIALMODE, p_default,
    DEFAULT_MATERIAL_MODE, p_accessor, GetFrostPBAccessor(), P_END, pb_undefinedMaterialId, _T( "undefinedMaterialID" ),
    TYPE_INT, P_RESET_DEFAULT, IDS_UNDEFINEDMATERIALID, p_default, 100, p_accessor, GetFrostPBAccessor(), P_END,

    pb_geometryMaterialIDNodeList, _T( "geometryMaterialIDNodeList" ), TYPE_INODE_TAB, 0, P_VARIABLE_SIZE,
    IDS_GEOMETRYMATERIALIDNODELIST, p_accessor, GetFrostPBAccessor(), P_END, pb_geometryMaterialIDInList,
    _T( "geometryMaterialIDInList" ), TYPE_INDEX_TAB, 0, P_VARIABLE_SIZE, IDS_GEOMETRYMATERIALIDINLIST, p_accessor,
    GetFrostPBAccessor(), P_END, pb_geometryMaterialIDOutList, _T( "geometryMaterialIDOutList" ), TYPE_INDEX_TAB, 0,
    P_VARIABLE_SIZE, IDS_GEOMETRYMATERIALIDOUTLIST, p_accessor, GetFrostPBAccessor(), P_END,

    pb_radiusScale, _T( "radiusScale" ), TYPE_FLOAT, P_RESET_DEFAULT + P_ANIMATABLE, IDS_RADIUSSCALE, p_default, 1.f,
    p_range, 0.f, 1000000.f, p_accessor, GetFrostPBAccessor(), p_ui, frost_meshing_param_map, TYPE_SPINNER,
    EDITTYPE_FLOAT, IDC_RADIUS_SCALE, IDC_RADIUS_SCALE_SPIN, SPIN_AUTOSCALE, P_END, pb_radiusAnimationMode,
    _T( "radiusAnimationMode" ), TYPE_INT, P_RESET_DEFAULT, IDS_RADIUSANIMATIONMODE, p_default,
    DEFAULT_RADIUS_ANIMATION_MODE, p_accessor, GetFrostPBAccessor(), P_END, pb_enableRadiusScale,
    _T( "enableRadiusScale" ), TYPE_BOOL, P_RESET_DEFAULT, IDS_ENABLERADIUSSCALE, p_default, FALSE, p_accessor,
    GetFrostPBAccessor(), p_ui, frost_meshing_param_map, TYPE_SINGLECHEKBOX, IDC_ENABLE_RADIUS_SCALE, P_END,

    pb_geometrySampleTimeBaseMode, _T( "geometrySampleTimeBaseMode" ), TYPE_INT, P_RESET_DEFAULT,
    IDS_GEOMETRYSAMPLETIMEBASEMODE, p_default, DEFAULT_GEOMETRY_TIMING_BASE_MODE, p_accessor, GetFrostPBAccessor(),
    P_END,

    pb_nodeListFlags, _T( "nodeListFlags" ), TYPE_INT_TAB, 0, P_VARIABLE_SIZE, IDS_NODELISTFLAGS, p_accessor,
    GetFrostPBAccessor(), P_END, pb_fileListFlags, _T( "fileListFlags" ), TYPE_INT_TAB, 0, P_VARIABLE_SIZE,
    IDS_FILELISTFLAGS, p_accessor, GetFrostPBAccessor(), P_END,

    pb_meshingResolutionMode, _T( "meshingResolutionMode" ), TYPE_INT, P_RESET_DEFAULT, IDS_MESHINGRESOLUTIONMODE,
    p_default, DEFAULT_MESHING_RESOLUTION_MODE, p_accessor, GetFrostPBAccessor(), p_ui, frost_meshing_quality_param_map,
    TYPE_RADIO, 2, IDC_PARTICLE_SUBDIVISION_RADIO, IDC_VOXEL_LENGTH_RADIO, p_vals,
    ( MESHING_RESOLUTION_MODE::SUBDIVIDE_MAX_RADIUS ), ( MESHING_RESOLUTION_MODE::VOXEL_LENGTH ), P_END,

    pb_renderVoxelLength, _T( "renderVoxelLength" ), TYPE_WORLD, P_RESET_DEFAULT | P_ANIMATABLE, IDS_RENDERVOXELLENGTH,
    p_default, 1.7f, p_range, 0.00001f, std::numeric_limits<float>::max(), p_accessor, GetFrostPBAccessor(), p_ui,
    frost_meshing_quality_param_map, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RENDER_VOXEL_LENGTH,
    IDC_RENDER_VOXEL_LENGTH_SPIN, SPIN_AUTOSCALE, P_END, pb_viewportVoxelLength, _T( "viewportVoxelLength" ),
    TYPE_WORLD, P_RESET_DEFAULT | P_ANIMATABLE, IDS_VIEWPORTVOXELLENGTH, p_default, 3.3f, p_range, 0.00001f,
    std::numeric_limits<float>::max(), p_accessor, GetFrostPBAccessor(), p_ui, frost_meshing_quality_param_map,
    TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_VIEWPORT_VOXEL_LENGTH, IDC_VIEWPORT_VOXEL_LENGTH_SPIN, SPIN_AUTOSCALE, P_END,

    pb_iconMode, _T("iconMode"), TYPE_INT, P_RESET_DEFAULT, IDS_ICONMODE, p_default, DEFAULT_ICON_MODE, p_range, 0, 1,
    p_accessor, GetFrostPBAccessor(), P_END,

    pb_viewRenderParticles, _T( "viewRenderParticles" ), TYPE_INT, P_RESET_DEFAULT, IDS_VIEW_RENDER_PARTICLES,
    p_default, 0, p_accessor, GetFrostPBAccessor(), p_ui, frost_meshing_param_map, TYPE_SINGLECHEKBOX,
    IDC_VIEW_RENDER_PARTICLES, P_END,

    // params for thinkingParticles groups
    pb_tpGroupFilterMode, _T( "tpGroupFilterMode" ), TYPE_INT, P_RESET_DEFAULT, IDS_TPGROUPFILTERMODE, p_default,
    DEFAULT_PARTICLE_GROUP_FILTER_MODE, p_accessor, GetFrostPBAccessor(), p_ui, frost_tp_groups_param_map, TYPE_RADIO,
    2, IDC_FILTER_MODE_ALL, IDC_FILTER_MODE_SELECTED, p_vals, ( PARTICLE_GROUP_FILTER_MODE::ALL ),
    ( PARTICLE_GROUP_FILTER_MODE::SELECTED ), P_END, pb_tpGroupList, _T( "tpGroupList" ), TYPE_REFTARG_TAB, 0,
    P_VARIABLE_SIZE, IDS_TPGROUPLIST, p_accessor, GetFrostPBAccessor(), P_END,

    // params for Region of Interest
    pb_enableROI, _T( "" ), TYPE_BOOL, P_RESET_DEFAULT, IDS_ENABLEROI, p_default, FALSE, p_accessor,
    GetFrostPBAccessor(), P_END,

    pb_enableRenderROI, _T( "enableRenderROI" ), TYPE_BOOL, P_RESET_DEFAULT, IDS_ENABLERENDERROI, p_default, FALSE,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_roi_param_map, TYPE_SINGLECHEKBOX, IDC_ENABLE_RENDER_ROI, P_END,

    pb_enableViewportROI, _T( "enableViewportROI" ), TYPE_BOOL, P_RESET_DEFAULT, IDS_ENABLEVIEWPORTROI, p_default,
    FALSE, p_accessor, GetFrostPBAccessor(), p_ui, frost_roi_param_map, TYPE_SINGLECHEKBOX, IDC_ENABLE_VIEWPORT_ROI,
    P_END,

    pb_roiCenterX, _T( "roiCenterX" ), TYPE_WORLD, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ROICENTERX, p_default, 0.f,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_roi_param_map, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_ROICENTERX,
    IDC_ROICENTERX_SPIN, SPIN_AUTOSCALE, P_END,

    pb_roiCenterY, _T( "roiCenterY" ), TYPE_WORLD, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ROICENTERY, p_default, 0.f,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_roi_param_map, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_ROICENTERY,
    IDC_ROICENTERY_SPIN, SPIN_AUTOSCALE, P_END,

    pb_roiCenterZ, _T( "roiCenterZ" ), TYPE_WORLD, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ROICENTERZ, p_default, 0.f,
    p_accessor, GetFrostPBAccessor(), p_ui, frost_roi_param_map, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_ROICENTERZ,
    IDC_ROICENTERZ_SPIN, SPIN_AUTOSCALE, P_END,

    pb_roiSizeX, _T( "roiSizeX" ), TYPE_WORLD, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ROISIZEX, p_default, 100.f, p_range,
    0.f, std::numeric_limits<float>::max(), p_accessor, GetFrostPBAccessor(), p_ui, frost_roi_param_map, TYPE_SPINNER,
    EDITTYPE_UNIVERSE, IDC_ROISIZEX, IDC_ROISIZEX_SPIN, SPIN_AUTOSCALE, P_END,

    pb_roiSizeY, _T( "roiSizeY" ), TYPE_WORLD, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ROISIZEY, p_default, 100.f, p_range,
    0.f, std::numeric_limits<float>::max(), p_accessor, GetFrostPBAccessor(), p_ui, frost_roi_param_map, TYPE_SPINNER,
    EDITTYPE_UNIVERSE, IDC_ROISIZEY, IDC_ROISIZEY_SPIN, SPIN_AUTOSCALE, P_END,

    pb_roiSizeZ, _T( "roiSizeZ" ), TYPE_WORLD, P_RESET_DEFAULT | P_ANIMATABLE, IDS_ROISIZEZ, p_default, 100.f, p_range,
    0.f, std::numeric_limits<float>::max(), p_accessor, GetFrostPBAccessor(), p_ui, frost_roi_param_map, TYPE_SPINNER,
    EDITTYPE_UNIVERSE, IDC_ROISIZEZ, IDC_ROISIZEZ_SPIN, SPIN_AUTOSCALE, P_END, P_END );

ParamBlockDesc2* GetFrostParamBlockDesc() { return &frost_param_block_desc; }
