// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
class Frost;

class frost_help_dlg_proc : public ParamMap2UserDlgProc {
    Frost* m_obj;

  public:
    frost_help_dlg_proc();
    frost_help_dlg_proc( Frost* obj );

    INT_PTR DlgProc( TimeValue t, IParamMap2* map, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

    void Update( TimeValue t );

    void UpdateEnables();

    void SetThing( ReferenceTarget* obj );

    void SetFrost( Frost* frost );

    void InvalidateUI( HWND hwnd, int element );

    void DeleteThis();
};
