// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

class frost_gui_resources {
    bool m_loadedIcons;
    HIMAGELIST m_himlIcons;
    HIMAGELIST m_himlLampIcons;

    frost_gui_resources();
    ~frost_gui_resources();

    void load_icons();

    frost_gui_resources( const frost_gui_resources& );
    frost_gui_resources& operator=( const frost_gui_resources& );

  public:
    static frost_gui_resources& get_instance();

    void apply_custbutton_fast_forward_icon( HWND hwnd );
    void apply_custbutton_x_icon( HWND hwnd );
    void apply_custbutton_down_arrow_icon( HWND hwnd );
    void apply_custbutton_up_arrow_icon( HWND hwnd );
    void apply_custbutton_right_arrow_icon( HWND hwnd );
    void apply_custbutton_left_arrow_icon( HWND hwnd );

    void apply_custimage_clear_lamp_icon( HWND hwnd );
    void apply_custimage_warning_lamp_icon( HWND hwnd );
    void apply_custimage_error_lamp_icon( HWND hwnd );
    void apply_custimage_lamp_icon( HWND hwnd, int i );
};

frost_gui_resources* GetFrostGuiResources();
