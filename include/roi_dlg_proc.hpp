// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

class Frost;

class roi_dlg_proc : public ParamMap2UserDlgProc {
  public:
    roi_dlg_proc();

    INT_PTR DlgProc( TimeValue t, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

    void Update( TimeValue t );

    void SetFrost( Frost* frost );

    void DeleteThis();

    void InvalidateUI( HWND hwnd, int element );

    void UpdateManipulateMode();

  private:
    void update_enable_roi( HWND hwnd );

    Frost* m_obj;
    HWND m_hwnd;
};
