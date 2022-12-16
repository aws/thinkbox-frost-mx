// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

class ParamBlockDesc2;

ParamBlockDesc2* GetFrostParamBlockDesc();

enum {
    frost_param_block,
    //
    frost_param_block_count // keep last
};

// parameter maps which use frost_param_block
enum {
    frost_main_param_map,
    frost_presets_param_map,
    frost_particle_objects_param_map,
    frost_particle_flow_events_param_map,
    frost_particle_files_param_map,
    frost_meshing_param_map,

    frost_meshing_quality_param_map,
    frost_metaballs_param_map,
    frost_zhu_bridson_param_map,
    frost_geometry_param_map,
    frost_anisotropic_param_map,

    frost_material_param_map,

    frost_help_param_map,

    frost_tp_groups_param_map,

    frost_roi_param_map,

    //
    frost_param_map_count // keep last
};

// parameter list
enum {
    pb_showIcon,
    pb_iconSize,

    pb_updateOnFrostChange,
    pb_updateOnParticleChange,

    pb_enableViewportMesh,
    pb_enableRenderMesh,

    pb_nodeList,

    pb_useAllPFEvents,
    pb_pfEventList,

    pb_fileList,
    pb_loadSingleFrame,
    pb_frameOffset,
    pb_limitToRange,
    pb_rangeFirstFrame,
    pb_rangeLastFrame,
    pb_enablePlaybackGraph,
    pb_playbackGraphTime,
    pb_beforeRangeBehavior,
    pb_afterRangeBehavior,
    pb_fileLengthUnit,
    pb_fileCustomScale,

    pb_meshingMethod,
    pb_radius,
    pb_useRadiusChannel,
    pb_randomizeRadius,
    pb_radiusRandomVariation,
    pb_radiusSeed,
    pb_motionBlurMode,

    pb_renderUsingViewportSettings,
    pb_renderMeshingResolution,
    pb_renderVertRefinementIterations,
    pb_previewAsGeometry,
    pb_viewportMeshingResolution,
    pb_viewportVertRefinementIterations,

    pb_metaballRadiusScale,
    pb_metaballIsosurfaceLevel,

    pb_zhuBridsonBlendRadiusScale,
    pb_zhuBridsonEnableLowDensityTrimming,
    pb_zhuBridsonLowDensityTrimmingThreshold,
    pb_zhuBridsonLowDensityTrimmingStrength,

    pb_geometryType,
    pb_geometryList,
    pb_geometrySelectionMode,
    pb_geometrySelectionSeed,
    pb_geometrySampleTimeOffsetMode,
    pb_geometrySampleTimeMaxRandomOffset,
    pb_geometrySampleTimeSeed,
    pb_geometryOrientationMode,
    pb_geometryOrientationX,
    pb_geometryOrientationY,
    pb_geometryOrientationZ,
    pb_geometryOrientationLookAtNode,
    pb_geometryOrientationVectorChannel,
    pb_geometryOrientationDivergence,
    pb_geometryOrientationRestrictDivergenceAxis,
    pb_geometryOrientationDivergenceAxisSpace,
    pb_geometryOrientationDivergenceAxisX,
    pb_geometryOrientationDivergenceAxisY,
    pb_geometryOrientationDivergenceAxisZ,

    pb_writeVelocityMapChannel,
    pb_velocityMapChannel,

    pb_viewportLoadMode,
    pb_viewportLoadPercent,

    pb_anisotropicRadiusScale,
    pb_anisotropicWindowScale,
    pb_anisotropicIsosurfaceLevel,
    pb_anisotropicMaxAnisotropy,
    pb_anisotropicMinNeighborCount,
    pb_anisotropicEnablePositionSmoothing,
    pb_anisotropicPositionSmoothingWindowScale,
    pb_anisotropicPositionSmoothingWeight,

    pb_pfEventFilterMode,

    pb_materialMode,
    pb_undefinedMaterialId,

    pb_geometryMaterialIDNodeList,
    pb_geometryMaterialIDInList,
    pb_geometryMaterialIDOutList,

    pb_radiusScale,
    pb_radiusAnimationMode,
    pb_enableRadiusScale,

    pb_geometrySampleTimeBaseMode,

    pb_nodeListFlags,
    pb_fileListFlags,

    pb_meshingResolutionMode,

    pb_renderVoxelLength,
    pb_viewportVoxelLength,

    pb_iconMode,

    pb_viewRenderParticles,

    pb_tpGroupFilterMode,
    pb_tpGroupList,

    pb_useRenderInstancing,

    pb_enableROI,
    pb_roiCenterX,
    pb_roiCenterY,
    pb_roiCenterZ,
    pb_roiSizeX,
    pb_roiSizeY,
    pb_roiSizeZ,

    pb_enableRenderROI,
    pb_enableViewportROI,

    //
    pb_entryCount // keep last
};
