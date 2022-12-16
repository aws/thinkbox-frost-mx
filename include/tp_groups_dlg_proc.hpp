// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "group_dlg_proc_base.hpp"

class tp_groups_dlg_proc_traits {
  public:
    enum { PARAM_MAP = frost_tp_groups_param_map };
    enum { GROUP_LIST_PARAMETER = pb_tpGroupList };
    enum { FILTER_MODE_PARAMETER = pb_tpGroupFilterMode };
    enum { FILTER_MODE_SELECTED = PARTICLE_GROUP_FILTER_MODE::SELECTED };

    enum { ADD_TOOLTIP = IDS_PARTICLE_GROUPS_ADD_TOOLTIP };
    enum { REMOVE_TOOLTIP = IDS_PARTICLE_GROUPS_REMOVE_TOOLTIP };
    enum { OPTIONS_TOOLTIP = IDS_PARTICLE_GROUPS_OPTIONS_TOOLTIP };

    static void get_group_names( Frost* frost, std::vector<frantic::tstring>& outNames ) {
        frost->get_tp_group_names( outNames );
    }

    static void clear_groups( Frost* frost ) { frost->clear_tp_groups(); }

    static void remove_groups( Frost* frost, const std::vector<int>& indices ) { frost->remove_tp_groups( indices ); }

    static void get_groups( Frost* frost, std::vector<particle_group_filter_entry::ptr_type>& outGroupList ) {
        // we need to get all TP groups, but also get the currently selected ones,
        // in case the TP object still exists but it has been ( temporarily ? ) removed from the particle objects list
        // so we'll need a set of "all" groups, then append the current groups that aren't in "all"
        std::vector<tp_group_info> selectedTpGroups;
        frost->get_selected_tp_groups( selectedTpGroups );
        std::vector<tp_group_info> allTpGroups;
        frost->get_all_tp_groups( allTpGroups );

        std::set<ReferenceTarget*> allTpGroupSet;
        std::set<ReferenceTarget*> selectedTpGroupSet;

        BOOST_FOREACH( tp_group_info& groupInfo, allTpGroups ) {
            allTpGroupSet.insert( groupInfo.group );
        }

        // for each selected group,
        // if not in "all" set,
        // append it to the "all" set
        BOOST_FOREACH( tp_group_info& groupInfo, selectedTpGroups ) {
            if( allTpGroupSet.find( groupInfo.group ) == allTpGroupSet.end() ) {
                allTpGroups.push_back( groupInfo );
                allTpGroupSet.insert( groupInfo.group );
            }
            selectedTpGroupSet.insert( groupInfo.group );
        }

        outGroupList.clear();
        outGroupList.reserve( allTpGroups.size() );
        BOOST_FOREACH( tp_group_info& groupInfo, allTpGroups ) {
            if( groupInfo.group ) {
                bool keep = true; // false;
                bool include = false;
                if( selectedTpGroupSet.find( groupInfo.group ) != selectedTpGroupSet.end() ) {
                    include = true;
                }
                /*
                if( frost->is_acceptable_tp_group( groupInfo.node, groupInfo.group ) ) {
                        keep = true;
                        include = false;
                } else if( frost->has_tp_group( groupInfo.node, groupInfo.group ) ) {
                        keep = true;
                        include = true;
                }
                */
                if( keep ) {
                    particle_group_filter_entry::ptr_type entry(
                        new thinking_particles_group_filter_entry( groupInfo.node, groupInfo.group, groupInfo.name ) );
                    entry->set_include( include );
                    outGroupList.push_back( entry );
                }
            }
        }
        /*
        outGroupList.clear();
        outGroupList.reserve( allTpGroups.size() );
        BOOST_FOREACH( tp_group_info & groupInfo, allTpGroups ) {
                if( groupInfo.group ) {
                        bool keep = false;
                        bool include = false;
                        if( frost->is_acceptable_tp_group( groupInfo.node, groupInfo.group ) ) {
                                keep = true;
                                include = false;
                        } else if( frost->has_tp_group( groupInfo.node, groupInfo.group ) ) {
                                keep = true;
                                include = true;
                        }
                        if( keep ) {
                                particle_group_filter_entry::ptr_type entry( new thinking_particles_group_filter_entry(
        groupInfo.node, groupInfo.group, groupInfo.name ) ); entry->set_include( include ); outGroupList.push_back(
        entry );
                        }
                }
        }
        */
    }

    static const TCHAR* get_mxs_options_callback() { return _T("on_TPGroupListOptions_pressed"); }

    static const TCHAR* get_mxs_right_click_callback() { return _T("on_TPGroupList_rightClicked"); }

    static const TCHAR* get_dlg_proc_name() { return _T("tp_groups_dlg_proc"); }

    static const TCHAR* get_all_radio_label() { return _T("All Groups"); }

    static const TCHAR* get_selected_radio_label() { return _T("Selected Groups:"); }

    static const TCHAR* get_particle_system_type() { return _T("Thinking Particles"); }

    static const TCHAR* get_group_type_plural() { return _T("Groups"); }
};

typedef group_dlg_proc_base<tp_groups_dlg_proc_traits> tp_groups_dlg_proc;
