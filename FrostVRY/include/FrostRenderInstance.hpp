// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FrostVRayAPIDefs.hpp"
#include "MeshShape.hpp"

#include <frantic/particles/particle_array.hpp>

#include <genlight.h>
#include <imtl.h>
#include <inode.h>
#include <object.h>
#include <render.h>

#include <mesh_file.h>
#include <meshprimitives.h>
#include <rayprimitives.h>
#include <vraygeom.h>
#include <vrayinterface.h>
#include <vrayplugins.h>
#if VRAY_DLL_VERSION >= 0x40000
#include <matrix_simd.hpp>
#endif

#if defined( tstring )
#undef tstring
#endif
#if defined( min )
#undef min
#endif
#if defined( max )
#undef max
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_array.hpp>

#include <vector>

class FrostInterface;

class FrostRenderInstance : public VR::VRenderInstance,
                            public VR::Shadeable,
                            public VR::ShadeData,
                            public VR::ShadeInstance,
                            public VR::GeometryGenerator {
  public:
    FrostRenderInstance( FrostInterface* instancer, VUtils::VRenderObject* renderObject, INode* node,
                         VR::VRayCore* vray, int renderID );
    ~FrostRenderInstance( void );

    // From VRenderInstance
    void renderBegin( TimeValue t, VR::VRayCore* vray ) override;
    void frameBegin( TimeValue t, VR::VRayCore* vray ) override;
    void compileGeometry( VR::VRayCore* vray ) override;
    void frameEnd( VR::VRayCore* vray ) override;
    void renderEnd( VR::VRayCore* vray ) override;

    // From RenderInstance
    Interval MeshValidity( void ) override { return FOREVER; }
    Point3 GetFaceNormal( int faceNum ) override { return Point3( 0, 0, 1 ); }
    Point3 GetFaceVertNormal( int faceNum, int vertNum ) override { return Point3( 0, 0, 1 ); }
    void GetFaceVertNormals( int faceNum, Point3 n[3] ) override {}
    Point3 GetCamVert( int vertNum ) override { return Point3( 0, 0, 0 ); }
    void GetObjVerts( int fnum, Point3 obp[3] ) override {}
    void GetCamVerts( int fnum, Point3 cp[3] ) override {}

    // From Shadeable
    void shade( VR::VRayContext& vri ) override;

    virtual PluginInterface* newInterface( InterfaceID id ) override;
    virtual PluginBase* getPlugin( void ) override { return NULL; }

    // From ShadeData
    vray_vector_t getUVWcoords( const VR::VRayContext& vri, int channel ) override;

#if VRAY_DLL_VERSION < 0x50000
    void getUVWderivs( const VR::VRayContext& vri, int channel, vray_vector_t derivs[2] ) override;
    void getUVWbases( const VR::VRayContext& vri, int channel, vray_vector_t bases[3] ) override;
#else
    void getUVWderivs( const VR::VRayContext& vri, int channel, vray_vector_t derivs[2],
                       const VUtils::UVWFlags uvwFlags ) override;
    void getUVWbases( const VR::VRayContext& vri, int channel, vray_vector_t bases[3],
                      const VUtils::UVWFlags uvwFlags ) override;
#endif

    vray_vector_t getUVWnormal( const VR::VRayContext& vri, int channel ) override;

    // Returns the material ID at the surface point
    int getMtlID( const VUtils::VRayContext& vri ) override;
    int getGBufID( void ) override { return 0; }
    int getEdgeVisibility( const VR::VRayContext& vri ) override;
    int getSurfaceRenderID( const VR::VRayContext& rc ) override { return VRenderInstance::renderID; }
    int getSurfaceHandle( const VR::VRayContext& rc ) override { return VRenderInstance::nodeHandle; }
    int getMaterialRenderID( const VR::VRayContext& rc ) override { return 0; }

    // From ShadeInstance
    vray_vector_t getBasePt( const VR::VRayContext& vri, BaseTime baseTime ) override;
    vray_vector_t getBasePtObj( const VR::VRayContext& vri ) override;

    vray_vector_t getBaseGNormal( const VR::VRayContext& vri, BaseTime baseTime ) override;
    vray_vector_t getBaseNormal( const VR::VRayContext& vri, BaseTime baseTime ) override;
    vray_vector_t getBaseBumpNormal( const VR::VRayContext& vri, BaseTime baseTime ) override;

    vray_vector_t worldToObjectVec( const VR::VRayContext& vri, const vray_vector_t& pt ) override;
    vray_vector_t worldToObjectPt( const VR::VRayContext& vri, const vray_vector_t& pt ) override;

    vray_vector_t objectToWorldVec( const VR::VRayContext& vri, const vray_vector_t& vec ) override;
    vray_vector_t objectToWorldPt( const VR::VRayContext& vri, const vray_vector_t& pt ) override;
    trace_point_t getShadowPt( const VR::VRayContext& vri ) override;
    vray_vector_t getVelocity( const VR::VRayContext& vri ) override;

    // From GeometryGenerator
    /// Fills in the intersection data for the primitive in rsray.is. for single ray context
    virtual void setIntersectionData( VR::RSRay& rsray, void* isData ) override;

    // From BSDFInterface
    // virtual VR::BSDFSampler* newBSDF( const VR::VRayContext& rc, VR::BSDFFlags flags );
    // virtual void deleteBSDF( const VR::VRayContext& rc, VR::BSDFSampler* bsdf );

#if VRAY_DLL_VERSION < 0x40000
    /// Fills in ray-bunch's intersection data for this primitive
    virtual void setIntersectionData( const VR::RayBunchParams& params, VR::PrimitiveIntersections& result,
                                      const RAY_IDX* idxs, size_t count ) override;
#endif

    /// Returns the zero-based index of the given primitive, generated by this geometry generator. This can be used to
    /// synchronize additional information about the child primitives.
    virtual int getPrimitiveIndex( const VUtils::GenericPrimitive* primitive ) const { return -1; }

    /// Return non-const shadeable object
    virtual VR::Shadeable* getShadeable();

    /// Return additional shading data
    virtual VR::VRayShadeInstance* getExtShadeData();

    /// Return additional texture mapping information
    virtual VR::VRayShadeData* getExtTexMapping();

    virtual VR::VRayVolume* getVolumeShader();

#if VRAY_DLL_VERSION < 0x60000
    virtual VR::VRaySurfaceProperties* getExtSurfaceProperties();
#else
    virtual const VR::VRaySurfaceProperties* getExtSurfaceProperties();
#endif


  private:
    Class_ID getRenderCameraClassID() const;
    void generateInstances( TimeValue t, VR::VRayCore* vray, VR::VRayRenderGlobalContext2* globContext2 );
    void deleteInstances( VR::VRayCore* vray );

    // Enumerate all particles
    void enumParticles( TimeValue t, VR::VRayCore* vray, VR::VRayRenderGlobalContext2* globContext2 );

    vray_vector_t get_particle_motion_per_exposure( int particleIndex );
    vray_vector_t get_particle_motion_per_frame( int particleIndex );

    VUtils::MeshInterface* get_shape_mesh( int shapeIndex ) { return m_shapes[shapeIndex].meshInterface.get(); }

    FrostInterface* m_frost;

    // Transformation of the plane
    VR::Transform tm, itm;

    Mesh m_dummyMesh; // Dummy 3dsmax mesh

    int m_firstPrimitiveId;

    // All primitives go in either m_staticPrimitives or m_movingPrimitives,
    // depending on whether we're using motion blur.
    bool m_useMovingPrimitives;
    typedef VUtils::StaticMeshInterfacePrimitive static_primitive_t;
    typedef VUtils::MovingMeshInterfacePrimitive moving_primitive_t;
    boost::scoped_array<static_primitive_t> m_staticPrimitives;
    boost::scoped_array<moving_primitive_t> m_movingPrimitives;

    std::vector<int> m_shapeIndices;

    boost::ptr_vector<MeshShape> m_shapes;

    int m_materialMode;

    frantic::particles::particle_array m_particles;

    std::vector<frantic::channels::channel_accessor<frantic::graphics::vector3f>> m_mapChannelAccessors;
    frantic::channels::channel_accessor<boost::int32_t> m_materialIdAccessor;

    float m_secondsPerMotionBlurInterval;
    float m_secondsPerFrame;
    frantic::channels::channel_accessor<frantic::graphics::vector3f> m_velocityAccessor;
};
