// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maxtypes.h>

#include <frantic/graphics/transform4f.hpp>

#include <frost/geometry_meshing_parameters_interface.hpp>

class Frost;

namespace frost {
namespace max3d {

class max3d_geometry_meshing_parameters : public frost::geometry_meshing_parameters_interface {
  public:
    max3d_geometry_meshing_parameters( Frost* node, TimeValue t );

    virtual double get_time();

    virtual geometry_type::option get_geometry_type();
    virtual hard_edge_type::option get_hard_edge_type();

    virtual geometry_selection_mode::option get_geometry_selection_mode();
    virtual boost::uint32_t get_geometry_selection_seed();

    virtual geometry_sample_time_base_mode::option get_geometry_sample_time_base_mode();
    virtual geometry_sample_time_offset_mode::option get_geometry_sample_time_offset_mode();
    virtual double get_geometry_sample_time_max_random_offset();
    virtual boost::uint32_t get_geometry_sample_time_seed();

    virtual double frames_to_seconds( double frames );

    virtual geometry_orientation_mode::option get_geometry_orientation_mode();
    virtual frantic::graphics::vector3f get_geometry_orientation();
    virtual frantic::graphics::vector3f get_geometry_look_at_position();
    virtual frantic::graphics::vector3f get_geometry_look_at_orientation();
    virtual frantic::tstring get_geometry_orientation_vector_channel();
    virtual float get_geometry_orientation_divergence();

    virtual bool get_orientation_restrict_divergence_axis();
    virtual frantic::graphics::vector3f get_orientation_divergence_axis();
    virtual geometry_orientation_divergence_axis_space::option get_geometry_orientation_divergence_axis_space();

    Matrix3 get_cached_look_at_transform();

  private:
    TimeValue m_t;
    Frost* m_node;

    Matrix3 m_cachedLookAtTransform;
};

} // namespace max3d
} // namespace frost
