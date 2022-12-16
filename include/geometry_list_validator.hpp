// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "Frost.hpp"

class geometry_list_validator : public PBValidator {
    Frost* m_frost;

    static BOOL validate( Frost* frost, PB2Value& v ) {
        if( frost ) {
            INode* node = (INode*)v.r;
            if( frost->accept_geometry( 0, node ) ) {
                return TRUE;
            }
        }
        return FALSE;
    }

  public:
    geometry_list_validator()
        : m_frost( 0 ) {}
    BOOL Validate( PB2Value& v ) { return validate( m_frost, v ); }
    BOOL Validate( PB2Value& v, ReferenceMaker* owner, ParamID /*id*/, int /*tabIndex*/ ) {
        return validate( dynamic_cast<Frost*>( owner ), v );
    }
    void SetFrost( Frost* frost ) { m_frost = frost; }
};

geometry_list_validator* GetGeometryListValidator();
