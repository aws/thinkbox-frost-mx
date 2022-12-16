// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

class FrostIconMaterial : public Material {
  public:
    FrostIconMaterial( const frantic::graphics::color3f& color = frantic::graphics::color3f( 0.9f ) )
        : Material() {
        shininess = 20.f;
        shinStrength = 1.f;
        set_color( color );
    }

    void set_color( const frantic::graphics::color3f& color ) {
        Ka[0] = color.r;
        Ka[1] = color.g;
        Ka[2] = color.b;
        Kd[0] = color.r;
        Kd[1] = color.g;
        Kd[2] = color.b;
        Ks[0] = 0.3f;
        Ks[1] = 0.3f;
        Ks[2] = 0.3f;
    }
};
