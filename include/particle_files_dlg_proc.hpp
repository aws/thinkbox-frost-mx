// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "Frost.hpp"

#include <boost/assign/list_of.hpp>

#include "listbox_with_tooltip.hpp"

class particle_files_dlg_proc : public ParamMap2UserDlgProc, public listbox_tooltip_source {
    Frost* m_obj;
    HWND m_hwnd;
    listbox_with_tooltip m_inplaceTooltip;

    std::vector<int> m_beforeRangeIndexToCode;
    std::vector<int> m_afterRangeIndexToCode;
    std::vector<int> m_fileUnitIndexToCode;

    std::vector<int> m_optionButtons;
    void init() {
        m_optionButtons.clear();
        m_optionButtons = { BTN_PARTICLE_FILES_EDIT, IDC_PARTICLE_FILES_CUSTOM_SCALE_OPTIONS };
        m_inplaceTooltip.set_tooltip_source( this );
    }

    void update_particle_files_view( HWND hWnd );
    Value* get_sel_indices( HWND hwndDlg );

  public:
    particle_files_dlg_proc();
    particle_files_dlg_proc( Frost* obj );

    INT_PTR DlgProc( TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
    void Update( TimeValue t );
    void SetThing( ReferenceTarget* obj );
    void SetFrost( Frost* );
    void InvalidateUI( HWND hwnd, int element );
    void UpdateEnables();
    void set_file_list_selection( const std::vector<int>& selection );
    void get_list_item_tooltip( int index, frantic::tstring& outTooltip );
    void DeleteThis();
};
