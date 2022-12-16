// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "Frost.hpp"

class orientation_look_at_validator : public PBValidator {
    Frost* m_frost;

    static BOOL validate( Frost* frost, PB2Value& v ) {
        if( frost ) {
            INode* node = (INode*)v.r;
            if( frost->accept_orientation_look_at_node( node ) ) {
                return TRUE;
            }
        }
        return FALSE;
    }

  public:
    orientation_look_at_validator()
        : m_frost( 0 ) {}
    BOOL Validate( PB2Value& v ) { return validate( m_frost, v ); }
    BOOL Validate( PB2Value& v, ReferenceMaker* owner, ParamID /*id*/, int /*tabIndex*/ ) {
        return validate( dynamic_cast<Frost*>( owner ), v );
    }
    void SetFrost( Frost* frost ) { m_frost = frost; }
};

orientation_look_at_validator* GetOrientationLookAtValidator();
