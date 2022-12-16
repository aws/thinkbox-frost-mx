// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/files/filename_sequence.hpp>
#include <frantic/geometry/trimesh3.hpp>
#include <frantic/graphics/camera.hpp>
#include <frantic/logging/progress_logger.hpp>
#include <frantic/particles/particle_array.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#ifdef ENABLE_FROST_DISK_CACHE
#include <frantic/geometry/trimesh3_file_io.hpp>
#include <frantic/geometry/xmesh_sequence_saver.hpp>
#include <frantic/volumetrics/levelset/rle_level_set.hpp>
#endif

#include <frantic/max3d/fpwrapper/mixin_wrapper.hpp>

#include "FrostInterface.hpp"
#include "FrostVRayLibrary.hpp"
#include "const_value_simulated_object_accessor.hpp"
#include "particle_cache_entry.hpp"
#include "simple_object_accessor.hpp"
#include "tp_group_info.hpp"

#include <boost/format.hpp>
#include <boost/optional.hpp>

#define Frost_CLASS_ID Class_ID( 0x2a730cdb, 0x746b394f )
#define Frost_SCRIPTED_CLASS_ID Class_ID( 0x144a36a6, 0x521a5c16 )
#define Frost_INTERFACE_ID Interface_ID( 0x67357e6, 0x5cf92e2d )
#define Frost_CLASS_NAME _T("Frost")
#define Frost_DISPLAY_NAME _T("Frost")
#define Frost_INTERFACE_NAME _T("FrostObjectInterface")
#define Frost_CATEGORY _T("Thinkbox")
#define Frost_TEXTURECOORDCHANNEL _T("TextureCoord")
#define Frost_SMOOTHINGGROUPCHANNEL _T("SmoothingGroup")
#define Frost_SHAPEINDEXCHANNEL _T("ShapeIndex")
#define Frost_POSITIONCHANNEL _T("Position")
#define Frost_IDCHANNEL _T("ID")
#define Frost_VELOCITY _T("Velocity")
#define Frost_VELOCITYCHANNEL Frost_VELOCITY
#define Frost_RADIUS _T("Radius")
#define Frost_SHAPEINDEX _T("ShapeIndex")
#define Frost_MESH_MTLINDEX_CHANNEL_NAME _T("MaterialID")
#define Frost_PARTICLE_MTLINDEX_CHANNEL_NAME _T("MtlIndex")
#define Frost_AGE_CHANNEL_NAME _T("Age")
#define Frost_LIFESPAN_CHANNEL_NAME _T("LifeSpan")
#define Frost_COLOR_CHANNEL_NAME _T("Color")

// This must be left in for backwards compatibility
// We were using it to indicate that files were saved
// with a licensed version of Frost as an anti-piracy measure
// We're continuting to add this marker to files now
// in-case someone needs to open a scene using an
// old version of Frost.
namespace CHUNK_ID {
enum chunk_id_enum { LICENSE };
};

#define SPARSE_MESHING_VOXEL_COUNT_THRESHOLD 500000

namespace FRAME_RANGE_CLAMP_MODE {
enum frame_range_clamp_mode_enum {
    HOLD,
    BLANK,
    //
    COUNT
};
};

#define FRAME_RANGE_CLAMP_MODE_HOLD_BEFORE _T("Hold First")
#define FRAME_RANGE_CLAMP_MODE_HOLD_AFTER _T("Hold Last")
#define FRAME_RANGE_CLAMP_MODE_BLANK _T("Blank")

#define DEFAULT_FRAME_RANGE_CLAMP_MODE ( FRAME_RANGE_CLAMP_MODE::HOLD )

namespace FILE_LENGTH_UNIT {
enum file_length_unit_enum {
    GENERIC,
    INCHES,
    FEET,
    MILES,
    MILLIMETERS,
    CENTIMETERS,
    METERS,
    KILOMETERS,
    CUSTOM,
    //
    COUNT
};
};

#define FILE_LENGTH_UNIT_GENERIC _T("Generic")
#define FILE_LENGTH_UNIT_INCHES _T("Inches")
#define FILE_LENGTH_UNIT_FEET _T("Feet")
#define FILE_LENGTH_UNIT_MILES _T("Miles")
#define FILE_LENGTH_UNIT_MILLIMETERS _T("Millimeters")
#define FILE_LENGTH_UNIT_CENTIMETERS _T("Centimeters")
#define FILE_LENGTH_UNIT_METERS _T("Meters")
#define FILE_LENGTH_UNIT_KILOMETERS _T("Kilometers")
#define FILE_LENGTH_UNIT_CUSTOM _T("Custom")

#define DEFAULT_FILE_LENGTH_UNIT ( FILE_LENGTH_UNIT::GENERIC )

namespace MESHING_METHOD {
enum meshing_method_enum {
    TETRAHEDRON,
    GEOMETRY,
    UNION_OF_SPHERES,
    METABALLS,
    ZHU_BRIDSON,
    VERTEX_CLOUD,
    ANISOTROPIC,
    //
    COUNT
};
};

#define MESHING_METHOD_GEOMETRY _T("Geometry")
#define MESHING_METHOD_UNION_OF_SPHERES _T("Union of Spheres")
#define MESHING_METHOD_METABALLS _T("Metaballs")
#define MESHING_METHOD_ZHU_BRIDSON _T("Zhu/Bridson")
#define MESHING_METHOD_ANISOTROPIC _T("Anisotropic")
#define MESHING_METHOD_VERTEX_CLOUD _T("Vertex Cloud")
#define MESHING_METHOD_TETRAHEDRA _T("Tetrahedron")

#define DEFAULT_MESHING_METHOD ( MESHING_METHOD::UNION_OF_SPHERES )

namespace VIEWPORT_LOAD_MODE {
enum viewport_load_mode_enum {
    HEAD,
    STRIDE,
    ID,
    //
    COUNT
};
};

#define VIEWPORT_LOAD_MODE_HEAD _T("Mesh First N Particles")
#define VIEWPORT_LOAD_MODE_STRIDE _T("Mesh Every Nth Particle")
#define VIEWPORT_LOAD_MODE_ID _T("Mesh Every Nth by ID")

#define DEFAULT_VIEWPORT_LOAD_MODE ( VIEWPORT_LOAD_MODE::HEAD )

namespace RADIUS_ANIMATION_MODE {
enum radius_animation_mode_enum {
    ABSOLUTE_TIME,
    AGE,
    LIFE_PERCENT,
    //
    COUNT
};
};

#define RADIUS_ANIMATION_MODE_ABSOLUTE_TIME _T("Time")
#define RADIUS_ANIMATION_MODE_AGE _T("Age")
#define RADIUS_ANIMATION_MODE_LIFE_PERCENT _T("Life %")

#define DEFAULT_RADIUS_ANIMATION_MODE ( RADIUS_ANIMATION_MODE::ABSOLUTE_TIME )

namespace MOTION_BLUR_MODE {
enum motion_blur_mode_enum {
    FRAME_VELOCITY_OFFSET,
    SUBFRAME_PARTICLE_OFFSET,
    NEAREST_FRAME_VELOCITY_OFFSET,
    //
    COUNT
};
};

#define MOTION_BLUR_FRAME_VELOCITY_OFFSET _T("Frame Velocity Offset")
#define MOTION_BLUR_SUBFRAME_POSITION _T("Subframe Particle Position")
#define MOTION_BLUR_NEAREST_FRAME_VELOCITY_OFFSET _T("Nearest Frame Velocity Offset")

#define DEFAULT_MOTION_BLUR_MODE ( MOTION_BLUR_MODE::FRAME_VELOCITY_OFFSET )

namespace GEOMETRY_TYPE {
enum geometry_type_enum {
    PLANE,
    SPRITE,
    TETRAHEDRON,
    CUSTOM_GEOMETRY,
    BOX,
    SPHERE20,
    //
    COUNT
};
};

#define GEOMETRY_TYPE_PLANE _T("Plane")
#define GEOMETRY_TYPE_SPRITE _T("Sprite")
#define GEOMETRY_TYPE_TETRAHEDRON _T("Tetrahedron")
#define GEOMETRY_TYPE_BOX _T("Box")
#define GEOMETRY_TYPE_SPHERE20 _T("Sphere")
#define GEOMETRY_TYPE_CUSTOM _T("Custom Geometry")

#define DEFAULT_GEOMETRY_TYPE ( GEOMETRY_TYPE::PLANE )

namespace GEOMETRY_SELECTION_MODE {
enum geometry_selection_mode_enum {
    CYCLE,
    RANDOM_BY_ID,
    SHAPEINDEX_CHANNEL,
    //
    COUNT
};
};

#define GEOMETRY_SELECTION_CYCLE _T("Cycle Shapes")
#define GEOMETRY_SELECTION_RANDOM _T("Random Shape By ID")
#define GEOMETRY_SELECTION_SHAPEINDEX _T("Use ShapeIndex Channel")

#define DEFAULT_GEOMETRY_SELECTION_MODE ( GEOMETRY_SELECTION_MODE::CYCLE )

namespace GEOMETRY_TIMING_BASE_MODE {
enum geometry_timing_base_mode_enum {
    TIME_0,
    CURRENT_TIME,
    //
    COUNT
};
};

#define GEOMETRY_TIMING_BASE_TIME_0 _T("Use Time 0")
#define GEOMETRY_TIMING_BASE_CURRENT_TIME _T("Use Current Time")

#define DEFAULT_GEOMETRY_TIMING_BASE_MODE ( GEOMETRY_TIMING_BASE_MODE::CURRENT_TIME )

namespace GEOMETRY_TIMING_OFFSET_MODE {
enum geometry_timing_offset_mode_enum {
    NO_OFFSET,
    RANDOM_BY_ID,
    ABSTIME_CHANNEL,
    TIMEOFFSET_CHANNEL,
    GEOMTIME_CHANNEL,
    //
    COUNT
};
};

#define GEOMETRY_TIMING_OFFSET_NO_OFFSET _T("No Time Offset")
#define GEOMETRY_TIMING_OFFSET_RANDOM _T("Random Offset By ID")
#define GEOMETRY_TIMING_OFFSET_ABSTIME _T("Use AbsTime Channel")
#define GEOMETRY_TIMING_OFFSET_TIMEOFFSET _T("Use TimeOffset Channel")
#define GEOMETRY_TIMING_OFFSET_GEOMTIME _T("Add GeomTime Channel")

#define DEFAULT_GEOMETRY_TIMING_OFFSET_MODE ( GEOMETRY_TIMING_OFFSET_MODE::NO_OFFSET )

namespace GEOMETRY_ORIENTATION_MODE {
enum geometry_orientation_mode_enum {
    LOOK_AT,
    MATCH_OBJECT,
    ORIENTATION_CHANNEL,
    VECTOR_CHANNEL,
    SPECIFY,
    //
    COUNT
};
};

#define GEOMETRY_ORIENTATION_LOOK_AT _T("Look At Target Object")
#define GEOMETRY_ORIENTATION_MATCH_OBJECT _T("Match Target Object")
#define GEOMETRY_ORIENTATION_ORIENTATION_CHANNEL _T("Use Orientation Channel")
#define GEOMETRY_ORIENTATION_VECTOR_CHANNEL _T("Use Vector Channel")
#define GEOMETRY_ORIENTATION_SPECIFY _T("Specify Orientation")

#define DEFAULT_GEOMETRY_ORIENTATION_MODE ( GEOMETRY_ORIENTATION_MODE::SPECIFY )

namespace AXIS_SPACE {
enum axis_space_enum {
    WORLD,
    LOCAL,
    //
    COUNT
};
};

#define AXIS_SPACE_WORLD _T("World")
#define AXIS_SPACE_LOCAL _T("Local")

#define DEFAULT_AXIS_SPACE ( AXIS_SPACE::WORLD )

namespace PARTICLE_GROUP_FILTER_MODE {
enum particle_group_filter_mode_enum {
    ALL,
    SELECTED,
    //
    COUNT
};
};

#define DEFAULT_PARTICLE_GROUP_FILTER_MODE ( PARTICLE_GROUP_FILTER_MODE::ALL )

namespace MATERIAL_MODE {
enum material_mode_enum {
    SINGLE,
    MTLINDEX_CHANNEL,
    SHAPE_NUMBER,
    MATERIAL_ID_FROM_GEOMETRY,
    MATERIAL_FROM_GEOMETRY,
    //
    COUNT
};
};

#define MATERIAL_MODE_SINGLE _T("Single Material ID")
#define MATERIAL_MODE_MTLINDEX_CHANNEL _T("ID from MtlIndex Channel")
#define MATERIAL_MODE_SHAPE_NUMBER _T("ID from Shape Number")
#define MATERIAL_MODE_MATERIAL_ID_FROM_GEOMETRY _T("ID from Geometry")
#define MATERIAL_MODE_MATERIAL_FROM_GEOMETRY _T("Material from Geometry")

#define DEFAULT_MATERIAL_MODE ( MATERIAL_MODE::MATERIAL_ID_FROM_GEOMETRY )

namespace MESHING_RESOLUTION_MODE {
enum meshing_resolution_mode_enum {
    SUBDIVIDE_MAX_RADIUS,
    VOXEL_LENGTH,
    //
    COUNT
};
};

#define DEFAULT_MESHING_RESOLUTION_MODE ( MESHING_RESOLUTION_MODE::SUBDIVIDE_MAX_RADIUS )

namespace ICON_MODE {
enum icon_mode_enum {
    WIREFRAME,
    FILLED,
    //
    COUNT
};
};

#define DEFAULT_ICON_MODE ( ICON_MODE::FILLED )

namespace frost_status_code {
enum frost_status_code_t {
    access_failed,
    no_particles_found,
    ok,
    warning,
    error,

    no_radius_channel,
    radius_channel_ok,
    partial_radius_channel,

    no_id_channel,
    id_channel_ok,
    partial_id_channel,

    no_age_channel,
    partial_age_channel,
    no_lifespan_channel,
    partial_lifespan_channel,

    no_mtlindex_channel,
    mtlindex_channel_ok,
    partial_mtlindex_channel,
    unsupported_material_mode,

    //
    count
};
};

using frost_status_code::frost_status_code_t;

struct frost_status_code_info {
    frantic::tstring msg;
    int weight;
    int color;
    frost_status_code_info()
        : weight( 0 )
        , color( 0 ) {}
    frost_status_code_info( const frantic::tstring& msg, int weight, int color )
        : msg( msg )
        , weight( weight )
        , color( color ) {}
};

struct param_map_info {
    MapID paramMap;
    int dialogId;
    int dialogStringId;
    ParamMap2UserDlgProc* dlgProc;

    param_map_info()
        : paramMap( 0 )
        , dialogId( 0 )
        , dialogStringId( 0 )
        , dlgProc( 0 ) {}
    param_map_info( MapID paramMap, int dialogId, int dialogStringId, ParamMap2UserDlgProc* dlgProc )
        : paramMap( paramMap )
        , dialogId( dialogId )
        , dialogStringId( dialogStringId )
        , dlgProc( dlgProc ) {}
};

typedef std::map<boost::uint16_t, boost::uint16_t> geometry_material_id_map_t;

namespace frost {

class geometry_meshing_parameters_interface;

} // namespace frost

namespace frost {
namespace max3d {

class max3d_geometry_meshing_parameters;

}
} // namespace frost

namespace VUtils {

class VRenderObject;

} // namespace VUtils

class Frost : public SimpleObject2,
              public frantic::max3d::fpwrapper::FFMixinInterface<Frost>,
              public FrostInterface,
              boost::noncopyable {
#ifdef ENABLE_FROST_DISK_CACHE
    frantic::files::filename_sequence m_cacheName;
    frantic::geometry::xmesh_sequence_saver m_xss;
#endif

    typedef std::vector<node_particle_cache_entry> node_particle_cache_t;
    node_particle_cache_t m_nodeParticleCache;
    typedef std::vector<file_particle_cache_entry> file_particle_cache_t;
    file_particle_cache_t m_fileParticleCache;

    bool m_inRenderingMode;
    bool m_inRenderingInstanceMode;
    TimeValue TIMESTEP;

    TimeValue m_renderTime;

    frantic::geometry::trimesh3 m_cachedTrimesh3;
    TimeValue m_cachedMeshTime;
    boost::int64_t m_cachedMeshParticleCount;
    bool m_cachedMeshInRenderingMode; // cached while rendering (affects pflow sources for example)
    bool m_cachedMeshIsRenderQuality; // cached using render quality settings?

    std::size_t m_displayFaceLimit;

    bool m_isFrostDirty;
    bool m_isParticlesDirty;
    bool m_isCustomGeometryDirty;
    bool m_isTargetPosDirty;
    bool m_isTargetRotDirty;

    Matrix3 m_cachedTargetTm;

    bool m_isVectorChannelNameCacheDirty;
    std::vector<frantic::tstring> m_vectorChannelNameCache;

    std::vector<frantic::channels::channel_map> m_cachedNativeChannelMaps;
    TimeValue m_cachedNativeChannelMapsTime;

    TimeValue m_statusIndicatorTime;

    boost::int32_t m_gizmoCounter;

    frantic::tstring m_presetName;
    bool m_isLoadingPreset;

    bool m_isSourceParticlesDirty;
    std::vector<frantic::files::filename_sequence> m_fileSequences;

    std::size_t m_buildTrimesh3Count;

    // pointer to the INode that the instance of this object belongs to,
    // currently used in lots of places eww
    INode* m_thisNode;

    simple_object_accessor<bool> m_showIcon;
    simple_object_accessor<float> m_iconSize;
    simple_object_accessor<int> m_iconMode;

    simple_object_accessor<bool> m_updateOnFrostChange;
    simple_object_accessor<bool> m_updateOnParticleChange;

    simple_object_accessor<bool> m_enableViewportMesh;
    simple_object_accessor<bool> m_enableRenderMesh;

    simple_object_accessor<INode*> m_nodeList;

    // simple_object_accessor<bool> m_useAllPFEvents;

    simple_object_accessor<int> m_pfEventFilterMode;
    simple_object_accessor<INode*> m_pfEventList;

    simple_object_accessor<int> m_tpGroupFilterMode;
    simple_object_accessor<ReferenceTarget*> m_tpGroupList;

    simple_object_accessor<frantic::tstring> m_fileList;
    simple_object_accessor<bool> m_loadSingleFrame;
    simple_object_accessor<int> m_frameOffset;
    simple_object_accessor<bool> m_limitToRange;
    simple_object_accessor<int> m_rangeFirstFrame;
    simple_object_accessor<int> m_rangeLastFrame;
    simple_object_accessor<bool> m_enablePlaybackGraph;
    simple_object_accessor<float> m_playbackGraphTime;
    simple_object_accessor<int> m_beforeRangeBehavior;
    simple_object_accessor<int> m_afterRangeBehavior;
    simple_object_accessor<int> m_fileLengthUnit;
    simple_object_accessor<float> m_fileCustomScale;

    simple_object_accessor<int> m_meshingMethod;
    simple_object_accessor<int> m_viewRenderParticles;
    simple_object_accessor<int> m_viewportLoadMode;
    simple_object_accessor<float> m_viewportLoadPercent;
    simple_object_accessor<float> m_radius;
    simple_object_accessor<bool> m_useRadiusChannel;
    simple_object_accessor<bool> m_randomizeRadius;
    simple_object_accessor<float> m_radiusRandomVariation;
    simple_object_accessor<int> m_radiusRandomSeed;
    simple_object_accessor<bool> m_enableRadiusScale;
    simple_object_accessor<float> m_radiusScale;
    simple_object_accessor<int> m_radiusAnimationMode;
    simple_object_accessor<int> m_motionBlurMode;

    simple_object_accessor<bool> m_renderUsingViewportSettings;
    simple_object_accessor<float> m_renderMeshingResolution;
    simple_object_accessor<int> m_renderVertRefinementIterations;
    const_value_simulated_object_accessor<bool> m_previewAsGeometry;
    simple_object_accessor<float> m_viewportMeshingResolution;
    simple_object_accessor<int> m_viewportVertRefinementIterations;
    simple_object_accessor<int> m_meshingResolutionMode;
    simple_object_accessor<float> m_renderVoxelLength;
    simple_object_accessor<float> m_viewportVoxelLength;

    simple_object_accessor<float> m_metaballRadiusScale;
    simple_object_accessor<float> m_metaballIsosurfaceLevel;

    simple_object_accessor<float> m_zhuBridsonBlendRadiusScale;
    simple_object_accessor<bool> m_zhuBridsonEnableLowDensityTrimming;
    simple_object_accessor<float> m_zhuBridsonLowDensityTrimmingThreshold;
    simple_object_accessor<float> m_zhuBridsonLowDensityTrimmingStrength;

    simple_object_accessor<int> m_geometryType;
    simple_object_accessor<INode*> m_geometryList;
    simple_object_accessor<int> m_geometrySelectionMode;
    simple_object_accessor<int> m_geometrySelectionSeed;

    simple_object_accessor<int> m_geometrySampleTimeBaseMode;
    simple_object_accessor<int> m_geometrySampleTimeOffsetMode;
    simple_object_accessor<float> m_geometrySampleTimeMaxRandomOffset;
    simple_object_accessor<int> m_geometrySampleTimeSeed;

    simple_object_accessor<int> m_geometryOrientationMode;
    simple_object_accessor<float> m_geometryOrientationX;
    simple_object_accessor<float> m_geometryOrientationY;
    simple_object_accessor<float> m_geometryOrientationZ;
    simple_object_accessor<INode*> m_geometryOrientationLookAtNode;
    simple_object_accessor<frantic::tstring> m_geometryOrientationVectorChannel;

    simple_object_accessor<float> m_geometryOrientationDivergence;
    simple_object_accessor<bool> m_geometryOrientationRestrictDivergenceAxis;
    simple_object_accessor<int> m_geometryOrientationDivergenceAxisSpace;
    simple_object_accessor<float> m_geometryOrientationDivergenceAxisX;
    simple_object_accessor<float> m_geometryOrientationDivergenceAxisY;
    simple_object_accessor<float> m_geometryOrientationDivergenceAxisZ;

    simple_object_accessor<bool> m_useRenderInstancing;

    simple_object_accessor<bool> m_writeVelocityMapChannel;
    simple_object_accessor<int> m_velocityMapChannel;

    simple_object_accessor<float> m_anisotropicRadiusScale;
    const_value_simulated_object_accessor<float> m_anisotropicWindowScale;
    simple_object_accessor<float> m_anisotropicIsosurfaceLevel;
    simple_object_accessor<float> m_anisotropicMaxAnisotropy;
    simple_object_accessor<int> m_anisotropicMinNeighborCount;
    const_value_simulated_object_accessor<bool> m_anisotropicEnablePositionSmoothing;
    const_value_simulated_object_accessor<float> m_anisotropicPositionSmoothingWindowScale;
    simple_object_accessor<float> m_anisotropicPositionSmoothingWeight;

    simple_object_accessor<int> m_materialMode;
    simple_object_accessor<int> m_undefinedMaterialID;
    simple_object_accessor<INode*> m_geometryMaterialIDNodeList;
    simple_object_accessor<int> m_geometryMaterialIDInList;
    simple_object_accessor<int> m_geometryMaterialIDOutList;

    simple_object_accessor<bool> m_enableRenderROI;
    simple_object_accessor<bool> m_enableViewportROI;
    simple_object_accessor<float> m_roiCenterX;
    simple_object_accessor<float> m_roiCenterY;
    simple_object_accessor<float> m_roiCenterZ;
    simple_object_accessor<float> m_roiSizeX;
    simple_object_accessor<float> m_roiSizeY;
    simple_object_accessor<float> m_roiSizeZ;

    Box3 get_mesh_bound_box( TimeValue t );

    static std::vector<int> m_beforeRangeBehaviorDisplayCodes;
    static std::map<int, frantic::tstring> m_beforeRangeBehaviorNames;

    static std::vector<int> m_afterRangeBehaviorDisplayCodes;
    static std::map<int, frantic::tstring> m_afterRangeBehaviorNames;

    static std::vector<int> m_fileLengthUnitDisplayCodes;
    static std::map<int, frantic::tstring> m_fileLengthUnitNames;

    static std::vector<int> m_meshingMethodDisplayCodes;
    static std::map<int, frantic::tstring> m_meshingMethodNames;

    static std::vector<int> m_viewportLoadModeCodes;
    static std::map<int, frantic::tstring> m_viewportLoadModeNames;

    static std::vector<int> Frost::m_radiusAnimationModeCodes;
    static std::map<int, frantic::tstring> Frost::m_radiusAnimationModeNames;

    static std::vector<int> m_motionBlurDisplayCodes;
    static std::map<int, frantic::tstring> m_motionBlurMethodNames;

    static std::vector<int> m_geometryTypeDisplayCodes;
    static std::map<int, frantic::tstring> m_geometryTypeNames;

    static std::vector<int> m_geometrySelectionModeDisplayCodes;
    static std::map<int, frantic::tstring> m_geometrySelectionModeNames;

    static std::vector<int> m_geometryTimingBaseModeDisplayCodes;
    static std::map<int, frantic::tstring> m_geometryTimingBaseModeNames;

    static std::vector<int> m_geometryTimingOffsetModeDisplayCodes;
    static std::map<int, frantic::tstring> m_geometryTimingOffsetModeNames;

    static std::vector<int> m_geometryOrientationModeDisplayCodes;
    static std::map<int, frantic::tstring> m_geometryOrientationModeNames;

    static std::vector<int> m_orientationDivergenceAxisSpaceDisplayCodes;
    static std::map<int, frantic::tstring> m_orientationDivergenceAxisSpaceNames;

    static std::vector<int> m_materialModeDisplayCodes;
    static std::map<int, frantic::tstring> m_materialModeNames;

    static IParamMap2* m_helpParamMap;
    static bool m_rsHelp;
    static IParamMap2* m_mainParamMap;
    static bool m_rsMain;
    static IParamMap2* m_presetsParamMap;
    static bool m_rsPresets;
    static IParamMap2* m_particleObjectsParamMap;
    static bool m_rsParticleObjects;
    static IParamMap2* m_particleFlowParamMap;
    static bool m_rsParticleFlow;
    static IParamMap2* m_tpGroupsParamMap;
    static bool m_rsTPGroups;
    static IParamMap2* m_particleFilesParamMap;
    static bool m_rsParticleFiles;
    static IParamMap2* m_meshingParamMap;
    static bool m_rsMeshing;
    static IParamMap2* m_materialParamMap;
    static bool m_rsMaterial;
    static IParamMap2* m_roiParamMap;
    static bool m_rsROI;

    static std::map<int, param_map_info> m_mesherParamMapInfo;
    static IParamMap2* m_mesherParamMap;
    static bool m_rsMesher;

    static std::map<int, param_map_info> m_qualityParamMapInfo;
    static IParamMap2* m_qualityParamMap;
    static bool m_rsQuality;

    typedef std::map<frost_status_code_t, frost_status_code_info> status_code_info_map_t;
    static std::map<frost_status_code_t, frost_status_code_info> m_statusCodeInfo;

    static frantic::channels::channel_map m_defaultChannelMap;
    static frantic::channels::channel_map m_defaultKrakatoaChannelMap;
    static frantic::channels::channel_map m_emptyChannelMap;

    class StaticInitializer {
      public:
        StaticInitializer();
    };
    static StaticInitializer m_staticInitializer;

    friend class frost::max3d::max3d_geometry_meshing_parameters;

  public:
    Frost( bool loading = false, bool allowPresets = false );
    ~Frost();

    // To support the Frantic Function Publishing interface
    void InitializeFPDescriptor();

    static IObjParam* ip;
    static Frost* editObj;

    // From BaseObject
    void BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev );
    void EndEditParams( IObjParam* ip, ULONG flags, Animatable* next );
    int HitTest( TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2* p, ViewExp* vpt );
    // void Snap( TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt );
    int Display( TimeValue t, INode* inode, ViewExp* vpt, int flags );
    // IParamArray *GetParamBlock();
    // int GetParamBlockIndex( int id );
    CreateMouseCallBack* GetCreateMouseCallBack();
#if MAX_VERSION_MAJOR >= 24
    const MCHAR* GetObjectName( bool localized );
#elif MAX_VERSION_MAJOR >= 15
    const MCHAR* GetObjectName();
#else
    MCHAR* GetObjectName();
#endif
    BOOL HasViewDependentBoundingBox();

#if MAX_VERSION_MAJOR >= 17
    // IObjectDisplay2 entries
    unsigned long GetObjectDisplayRequirement() const;
#elif MAX_VERSION_MAJOR >= 14
    bool RequiresSupportForLegacyDisplayMode() const;
#endif

#if MAX_VERSION_MAJOR >= 17
    bool PrepareDisplay( const MaxSDK::Graphics::UpdateDisplayContext& prepareDisplayContext );
    bool UpdatePerNodeItems( const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
                             MaxSDK::Graphics::UpdateNodeContext& nodeContext,
                             MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer );
#elif MAX_VERSION_MAJOR >= 15
    bool UpdateDisplay( const MaxSDK::Graphics::MaxContext& maxContext,
                        const MaxSDK::Graphics::UpdateDisplayContext& displayContext );
#elif MAX_VERSION_MAJOR == 14
    bool UpdateDisplay( unsigned long renderItemCategories,
                        const MaxSDK::Graphics::MaterialRequiredStreams& materialRequiredStreams, TimeValue t );
#endif

    // From Object
    ObjectState Eval( TimeValue time );
    void InitNodeName( MSTR& s );
    Interval ObjectValidity( TimeValue t );
    // int CanConvertToType( Class_ID obtype );
    // Object* ConvertToType( TimeValue t, Class_ID obtype );
    BOOL PolygonCount( TimeValue t, int& numFaces, int& numVerts );
    // Object * FindBaseObject();
    BOOL UseSelectionBrackets();

    // From GeomObject
    // int IntersectRay( TimeValue t, Ray & ray, float & at, Point3 & norm );
    void GetWorldBoundBox( TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
    void GetLocalBoundBox( TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
    // void GetDeformBBox( TimeValue t, Box3 & box, Matrix3 * tm, BOOL useSel );
    // Mesh* GetRenderMesh( TimeValue t, INode * inode, View & iew, BOOL& needDelete );
    int NumberOfRenderMeshes();

    // Animatable methods
    void DeleteThis();
    Class_ID ClassID();
    void FreeCaches();
#if MAX_VERSION_MAJOR >= 24
    void GetClassName( MSTR& s, bool localized );
#else
    void GetClassName( MSTR& s );
#endif
    // int NumSubs();
    // Animatable* SubAnim( int i );
    // MSTR SubAnimName( int i );
    void* GetInterface( ULONG id );
    void ReleaseInterface( ULONG id, void* i );
    BaseInterface* GetInterface( Interface_ID id );
    // void ReleaseInterface( ULONG id, void * ip );
    int RenderBegin( TimeValue t, ULONG flags );
    int RenderEnd( TimeValue t );
    int NumParamBlocks();
    IParamBlock2* GetParamBlock( int i );
    IParamBlock2* GetParamBlockByID( short id );

    // From ReferenceMaker
    // int NumRefs();
    // RefTargetHandle GetReference( int i );
    // void SetReference( int i, RefTargetHandle rtarg );
#if MAX_VERSION_MAJOR >= 17
    RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message,
                                BOOL propagate );
#else
    RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message );
#endif
    IOResult Save( ISave* isave );

    // From ReferenceTarget
    ReferenceTarget* Clone( RemapDir& remap );

    // From SimpleObject
    void BuildMesh( TimeValue t );
    void InvalidateUI();
    // ParamDimension * GetParameterDim( int pbIndex );
    // MSTR GetParameterName( int pbIndex );

    // Frost

    void BuildMesh( TimeValue t, const frantic::graphics::camera<float>& camera );

    void SetRenderTime( TimeValue t );
    TimeValue GetRenderTime();
    bool HasValidRenderTime();
    void ClearRenderTime();

    void set_empty_validity_and_notify_dependents();
    void force_invalidate();
    void force_viewport_update();
    void maybe_invalidate( TimeValue t );
    void invalidate_tab_param( ParamID id, TimeValue t );
    void on_param_set( PB2Value& v, ParamID id, int tabIndex, TimeValue t );
    void on_tab_changed( PBAccessor::tab_changes changeCode, Tab<PB2Value>* tab, ParamID id, int tabIndex, int count );

    void invalidate_status_indicators();
    void update_status_indicators( TimeValue t );

    bool is_saving_to_file();

    bool is_frost_invalid();
    void invalidate_frost();
    bool is_particles_invalid();
    void invalidate_particles();
    bool is_custom_geometry_invalid();
    void invalidate_custom_geometry();
    bool is_target_pos_invalid();
    void invalidate_target_pos();
    bool is_target_rot_invalid();
    void invalidate_target_rot();
    void set_valid();

    void invalidate_preset();
    frantic::tstring get_preset_name();
    void set_preset_name( const frantic::tstring& presetName );
    void invalidate_preset_name_list();
    void get_preset_name_list( std::vector<frantic::tstring>& presetNames );
    static frantic::tstring get_preset_filename( const frantic::tstring& presetName );
    void load_preset_from_filename( const frantic::tstring& filename );

    void invalidate_source_sequences();
    void validate_source_sequences( TimeValue t );
    void UpdateSourceSequences( TimeValue t );
    // std::vector<frantic::tstring> GetVectorChannelNames( TimeValue t );

    void GetParticleFilenames( TimeValue t, double frame, std::vector<frantic::tstring>& outFilenames,
                               std::vector<float>& timeOffsets );

#ifdef ENABLE_FROST_DISK_CACHE
    void SaveMeshSequence( frantic::tstring filename );
    void SaveMesh( frantic::tstring filename, TimeValue t );
    void SaveMeshToSequence( TimeValue t, bool renderMode );
    void SaveLevelSetToSequence( TimeValue t, Matrix3 tm );
    frantic::tstring SetCacheName( const frantic::tstring& filename );
    frantic::tstring StripSequenceNumber( const frantic::tstring& filename );
    frantic::tstring GetTempFileNameWithPrefix( const frantic::tstring& prefix );
#endif

    frantic::tstring get_ini_filename();
    void save_preferences();
    void load_preferences();

    bool get_build_during_save();

    bool is_particle_object( INode* node );
    bool has_particle_object( INode* node );
    bool accept_particle_object( INode* node );
    void add_particle_object( INode* node );
    void remove_particle_object( int i );
    INode* get_particle_object( int i );
    void get_particle_object_names( std::vector<frantic::tstring>& names );

    void SetNodeListSelection( const std::vector<int> selection );
    std::vector<int> GetNodeListSelection( void );

    // void SetPFEventListSelection( const std::vector<int> selection );
    // std::vector<int> GetPFEventListSelection( void );

    bool has_particle_file( const frantic::tstring& filename );
    bool accept_particle_file( const frantic::tstring& filename );
    void add_particle_file( const frantic::tstring& filename );
    void remove_particle_file( int i );
    int get_particle_file_count();
    frantic::tstring get_particle_file( int i );
    frantic::tstring get_particle_file_display_name( int i );
    void get_particle_file_display_names( std::vector<frantic::tstring>& displayNames );

    static void begin_pick_particle_object( HWND hwnd );
    static void end_pick_particle_object( HWND hwnd );

    void invalidate_particle_cache();
    void invalidate_particle_node_cache();

    void reload_particle_files( TimeValue t );
    void invalidate_particle_file_cache();
    std::pair<float, float> get_particle_file_frame_range();
    void set_particle_file_first_frame( int frameNumber );
    void set_particle_file_last_frame( int frameNumber );

    static const std::vector<int>& get_before_range_behavior_codes();
    static frantic::tstring get_before_range_behavior_name( int i );
    int get_before_range_behavior();
    void set_before_range_behavior( int behavior );

    static const std::vector<int>& get_after_range_behavior_codes();
    static frantic::tstring get_after_range_behavior_name( int i );
    int get_after_range_behavior();
    void set_after_range_behavior( int behavior );

    int get_file_length_unit();
    void set_file_length_unit( const int unit );
    static const std::vector<int>& get_file_particle_scale_factor_codes();
    static frantic::tstring get_file_length_unit_name( int i );

    bool get_load_single_frame();
    void set_load_single_frame( bool loadSingleFrame );
    bool get_enable_playback_graph();
    bool get_limit_to_range();
    void set_limit_to_range( bool limitToRange );

    void SetToValidFrameRange( TimeValue t );

    bool get_use_custom_file_scale();

    std::vector<int> GetFileListSelection();                       // in interface
    void SetFileListSelection( const std::vector<int> selection ); // in interface

    static const std::vector<int>& get_meshing_method_codes();
    static frantic::tstring get_meshing_method_name( int i );
    int get_meshing_method();
    void set_meshing_method( const int meshingMethod );

    static const std::vector<int>& get_material_mode_codes();
    static frantic::tstring get_material_mode_name( int i );
    int get_material_mode();
    void set_material_mode( const int materialMode );

    MtlID get_undefined_material_id();

    static const std::vector<int>& get_viewport_load_mode_codes();
    static frantic::tstring get_viewport_load_mode_name( int i );
    int get_viewport_load_mode();
    void set_viewport_load_mode( int mode );

    static const std::vector<int>& get_radius_animation_mode_codes();
    static frantic::tstring get_radius_animation_mode_name( int i );
    int get_radius_animation_mode();
    void set_radius_animation_mode( int mode );

    static const std::vector<int>& get_motion_blur_codes();
    static frantic::tstring get_motion_blur_name( int i );
    int get_motion_blur_mode();
    void set_motion_blur_mode( const int motionBlurMode );

    std::vector<int> get_pf_event_list_selection();

    static bool is_particle_flow_node( INode* node );
    bool has_particle_flow_event( INode* node );
    bool accept_particle_flow_event( INode* node );
    void get_selected_particle_flow_events( std::vector<INode*>& outEvents );
    void get_all_particle_flow_events( std::vector<INode*>& outEvents );
    void get_acceptable_particle_flow_events( std::vector<INode*>& );
    void add_particle_flow_event( INode* node );
    void add_particle_flow_events( const std::vector<INode*>& nodes );
    std::size_t get_particle_flow_event_count();
    INode* get_particle_flow_event( int i );
    void clear_particle_flow_events();
    void remove_particle_flow_events( const std::vector<int>& indices );
    void get_particle_flow_event_names( std::vector<frantic::tstring>& eventNames );

    // void RefreshPFEventList();

    bool has_tp_group( INode*, ReferenceTarget* );
    bool is_acceptable_tp_group( INode*, ReferenceTarget* );
    void get_selected_tp_groups( std::vector<tp_group_info>& outGroups );
    void get_all_tp_groups( std::vector<tp_group_info>& outGroups );
    void get_acceptable_tp_groups( std::vector<tp_group_info>& outGroups );
    void add_tp_group( INode*, ReferenceTarget* );
    void clear_tp_groups();
    void remove_tp_groups( const std::vector<int>& indices );
    void get_tp_group_names( std::vector<frantic::tstring>& outGroupNames );

    // void RefreshTPGroupList();

    int get_meshing_resolution_mode();
    void set_meshing_resolution_mode( const int meshingResolutionMode );

    int get_geometry_type();
    void set_geometry_type( const int );
    static const std::vector<int>& get_geometry_type_codes();
    static frantic::tstring get_geometry_type_name( int i );

    bool accept_geometry( TimeValue t, INode* node );
    void add_geometry( TimeValue t, INode* node );
    void remove_geometry( const std::vector<int>& indices );
    void get_geometry_names( std::vector<frantic::tstring>& nodeNames );

    // From FrostInterface
    std::size_t get_custom_geometry_count();
    INode* get_custom_geometry( std::size_t i );

    std::size_t get_geometry_material_id_node_count();
    INode* get_geometry_material_id_node( std::size_t i );

    std::size_t get_geometry_material_id_in_count();
    int get_geometry_material_id_in( std::size_t i );

    std::size_t get_geometry_material_id_out_count();
    int get_geometry_material_id_out( std::size_t i );

    void SetGeometryListSelection( const std::vector<int> selection );
    std::vector<int> GetGeometryListSelection( void );

    int get_geometry_selection_mode();
    void set_geometry_selection_mode( const int );
    static const std::vector<int>& get_geometry_selection_mode_codes();
    static frantic::tstring get_geometry_selection_mode_name( int i );

    int get_geometry_timing_base_mode();
    void set_geometry_timing_base_mode( const int );
    static const std::vector<int>& get_geometry_timing_base_mode_codes();
    static frantic::tstring get_geometry_timing_base_mode_name( int i );

    int get_geometry_timing_offset_mode();
    void set_geometry_timing_offset_mode( const int );
    static const std::vector<int>& get_geometry_timing_offset_mode_codes();
    static frantic::tstring get_geometry_timing_offset_mode_name( int i );

    int get_geometry_orientation_mode();
    void set_geometry_orientation_mode( const int );
    static const std::vector<int>& get_geometry_orientation_mode_codes();
    static frantic::tstring get_geometry_orientation_mode_name( int i );

    void invalidate_vector_channel_name_cache();
    void get_vector_channel_names( std::vector<frantic::tstring>& channelNames, TimeValue t );
    frantic::tstring get_geometry_orientation_vector_channel();
    void set_geometry_orientation_vector_channel( const frantic::tstring& );

    int get_divergence_axis_space();
    void set_divergence_axis_space( const int );
    static const std::vector<int>& get_divergence_axis_space_codes();
    static frantic::tstring get_divergence_axis_space_name( int i );

    void set_geometry_orientation( const frantic::graphics::vector3f&, TimeValue );
    void set_geometry_orientation_from_node( INode* node, TimeValue );
    void set_divergence_axis( const frantic::graphics::vector3f&, TimeValue );
    void set_divergence_axis_from_node_z( INode*, TimeValue );

    bool accept_orientation_look_at_node( INode* node );
    void set_orientation_look_at_node( INode* node );
    INode* get_orientation_look_at_node();
    frantic::graphics::vector3f get_orientation_look_at_pos( TimeValue t );
    Matrix3 get_target_tm( TimeValue t );
    Matrix3 get_cached_target_tm();

    void update_ui_to_meshing_method( const int oldMeshingMethod );

    void get_mesher_param_map_info( param_map_info& );
    void get_quality_param_map_info( param_map_info& );

    Mesh* get_icon_mesh( TimeValue t );

    bool get_enable_render_roi();
    void set_enable_render_roi( bool enable );
    bool get_enable_viewport_roi();
    void set_enable_viewport_roi( bool enable );

    frantic::graphics::boundbox3f get_raw_roi_box( TimeValue t );
    // The ROI box shown as a wireframe in the viewport
    boost::optional<frantic::graphics::boundbox3f> get_display_roi_box( TimeValue t );
    // The ROI box used for meshing
    boost::optional<frantic::graphics::boundbox3f> get_roi_box( TimeValue t );
    bool use_filled_icon( TimeValue t );

    frost_status_code_t get_parameter_status_code( ParamID id, TimeValue t );
    frantic::tstring get_status_code_string( frost_status_code_t msg );
    int get_status_code_weight( frost_status_code_t msg );
    int get_status_code_lamp( frost_status_code_t msg );

    // gets the node to the first node in the scene matching this
    INode* get_inode();

    // gets the maximum particle radius
    float GetMaxParticleRadius( FPTimeValue t ); // in interface

    // gets the minimum particle radius
    // float GetMinParticleRadius( FPTimeValue t );

    static const frantic::channels::channel_map& get_default_channel_map();
    static const frantic::channels::channel_map& get_default_krakatoa_channel_map();
    static const frantic::channels::channel_map& get_empty_channel_map();

    // send warning to the log window
    void report_warning( const frantic::tstring& );
    void report_error( const frantic::tstring& );

    bool HasAnyParticleSource( TimeValue t );

    boost::shared_ptr<frost::geometry_meshing_parameters_interface> create_geometry_meshing_parameters( TimeValue t );

  private:
    bool has_scene_particle_source( TimeValue t );

    void get_particle_streams( TimeValue t, const frantic::graphics::camera<float>& camera,
                               std::vector<boost::shared_ptr<frantic::particles::streams::particle_istream>>& pins,
                               std::vector<frantic::tstring>& pinNames, bool load = true );

  public:
    boost::shared_ptr<frantic::particles::streams::particle_istream>
    GetParticleIStream( TimeValue time, const frantic::graphics::camera<float>& camera );

  private:
    // Return true if the node can be used as a particle source.
    // bool IsParticleSource( INode * node );
    // renamed -> accept_particle_object

    // Return the particle_istream from the specified scene particle object.
    // If no particle_istream could be retrieved, then an exception is thrown.
    boost::shared_ptr<frantic::particles::streams::particle_istream>
    get_scene_object_particle_istream( TimeValue t, INode* prtNode, const frantic::graphics::camera<float>& camera,
                                       bool forceRenderState );

    int get_meshing_method_internal( TimeValue t, bool useRenderSettings );

    // This builds the mesh for the given time
    void build_mesh( TimeValue t, const frantic::graphics::camera<float>& camera, Mesh& mesh );

    // This builds the frame-time cached trimesh3 (with velocity channel for motion blur)
    void build_trimesh3( const TimeValue time, const frantic::graphics::camera<float>& camera,
                         const bool useRenderMeshing,
                         frantic::geometry::trimesh3& outMesh ); // uses a null progress logger
    void build_trimesh3( const TimeValue time, const frantic::graphics::camera<float>& camera,
                         const bool useRenderMeshing, frantic::geometry::trimesh3& outMesh,
                         boost::int64_t& outParticleCount, frantic::logging::progress_logger& progressLogger );

#ifdef FROST_LEVEL_SET
    // This builds a level set at the given time from the loaded particles, using the selected is policy
    void BuildLevelSet( TimeValue time, Matrix3 tm, frantic::volumetrics::levelset::rle_level_set& outRLS );
#endif

    // Gets the start/end frames shared by the all the sequences.
    // Ie, the start/end frames of the intersection of the sets of sequence numbers.
    bool get_start_frame( float& out );
    bool get_end_frame( float& out );

    TimeValue round_to_nearest_wholeframe( TimeValue t ) const;

    void fill_channel_map_cache( TimeValue t );
    // do the particles have the specified channel? yes no some noparticles
    // Value* HasChannel( FPTimeValue t, const frantic::tstring & channelName );

    void validate_channel_map_caches( boost::shared_ptr<frantic::particles::streams::particle_istream>& pin );

    float get_radius_random_variation_fraction( TimeValue t );

    // get the (min,max) particle radius
    std::pair<float, float> get_particle_radius_extrema( FPTimeValue t );
    std::pair<float, float> get_particle_radius_extrema( FPTimeValue t, const frantic::particles::particle_array& pa );
    std::pair<float, float>
    get_particle_radius_extrema( FPTimeValue t, boost::shared_ptr<frantic::particles::streams::particle_istream> pin );

    double get_file_particle_scale_factor( TimeValue t );

    __forceinline Mesh& get_active_mesh();
    __forceinline Mesh& get_inactive_mesh();
    void clear_particle_cache();

    frantic::tstring get_node_name();

    bool use_vray_instancing();

    /**
     * Cull particles that may affect meshing within the culling bounds,
     * using the maximum possible effect distance for anisotropic meshing.
     * This doesn't account for the particle distribution at all -- it
     * only accounts for the maximum particle radius and the anisotropic
     * meshing parameters.  The actual effect distance is normally much
     * smaller.
     */
    frantic::particles::particle_istream_ptr
    broad_cull_anisotropic_particles( TimeValue t, frantic::particles::particle_istream_ptr pin,
                                      const boost::optional<frantic::graphics::boundbox3f>& bounds );

    static FrostVRayLibrary::ptr_type m_frostVRayLibrary;
};

Frost* GetFrost( Object* obj );
