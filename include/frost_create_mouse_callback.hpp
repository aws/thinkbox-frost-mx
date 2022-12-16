// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "Frost.hpp"

class frost_create_mouse_callback : public CreateMouseCallBack {
  private:
    Frost* m_obj;
    IPoint2 m_sp0;
    Point3 m_p0;

  public:
    int proc( ViewExp* vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat );
    void SetObj( Frost* obj );
};
