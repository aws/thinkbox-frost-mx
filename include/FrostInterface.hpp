// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/optional.hpp>

#include <maxtypes.h>

#include <frantic/graphics/camera.hpp>

class INode;

namespace frost {
class geometry_meshing_parameters_interface;
}

class geometry_source_interface;

namespace frantic {
namespace particles {
namespace streams {
class particle_istream;
}
} // namespace particles
} // namespace frantic

/**
 * Interface for functions that are called by the Frost V-Ray plugin.
 */
class FrostInterface {
  public:
    virtual boost::shared_ptr<frost::geometry_meshing_parameters_interface>
    create_geometry_meshing_parameters( TimeValue t ) = 0;

    /**
     * Return true if the Frost node has any particle source.
     *
     * GetParticleIStream() will throw an exception if the node doesn't have
     * any particle sources, so you can use this function to avoid such an
     * exception.
     */
    virtual bool HasAnyParticleSource( TimeValue t ) = 0;
    virtual boost::shared_ptr<frantic::particles::streams::particle_istream>
    GetParticleIStream( TimeValue time, const frantic::graphics::camera<float>& camera ) = 0;

    virtual MtlID get_undefined_material_id() = 0;
    virtual int get_material_mode() = 0;

    virtual std::size_t get_custom_geometry_count() = 0;
    virtual INode* get_custom_geometry( std::size_t i ) = 0;

    virtual std::size_t get_geometry_material_id_node_count() = 0;
    virtual INode* get_geometry_material_id_node( std::size_t i ) = 0;

    virtual std::size_t get_geometry_material_id_in_count() = 0;
    virtual int get_geometry_material_id_in( std::size_t i ) = 0;

    virtual std::size_t get_geometry_material_id_out_count() = 0;
    virtual int get_geometry_material_id_out( std::size_t i ) = 0;

    virtual boost::optional<frantic::graphics::boundbox3f> get_roi_box( TimeValue t ) = 0;
};
