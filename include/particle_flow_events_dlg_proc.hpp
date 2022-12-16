// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "group_dlg_proc_base.hpp"

class particle_flow_events_dlg_proc_traits {
  public:
    enum { PARAM_MAP = frost_particle_flow_events_param_map };
    enum { GROUP_LIST_PARAMETER = pb_pfEventList };
    enum { FILTER_MODE_PARAMETER = pb_pfEventFilterMode };
    enum { FILTER_MODE_SELECTED = PARTICLE_GROUP_FILTER_MODE::SELECTED };

    enum { ADD_TOOLTIP = IDS_PARTICLE_FLOW_EVENTS_ADD_TOOLTIP };
    enum { REMOVE_TOOLTIP = IDS_PARTICLE_FLOW_EVENTS_REMOVE_TOOLTIP };
    enum { OPTIONS_TOOLTIP = IDS_PARTICLE_FLOW_EVENTS_OPTIONS_TOOLTIP };

    static void get_group_names( Frost* frost, std::vector<frantic::tstring>& outNames ) {
        frost->get_particle_flow_event_names( outNames );
    }

    static void clear_groups( Frost* frost ) { frost->clear_particle_flow_events(); }

    static void remove_groups( Frost* frost, const std::vector<int>& indices ) {
        frost->remove_particle_flow_events( indices );
    }

    static void get_groups( Frost* frost, std::vector<particle_group_filter_entry::ptr_type>& outGroupList ) {
        std::vector<INode*> allPfEvents;
        frost->get_all_particle_flow_events( allPfEvents );

        std::vector<INode*> selectedPfEvents;
        frost->get_selected_particle_flow_events( selectedPfEvents );

        std::set<INode*> allPfEventSet;
        std::set<INode*> selectedPfEventSet;

        BOOST_FOREACH( INode* event, allPfEvents ) {
            allPfEventSet.insert( event );
        }

        BOOST_FOREACH( INode* selectedEvent, selectedPfEvents ) {
            if( allPfEventSet.find( selectedEvent ) == allPfEventSet.end() ) {
                allPfEvents.push_back( selectedEvent );
                allPfEventSet.insert( selectedEvent );
            }
            selectedPfEventSet.insert( selectedEvent );
        }

        outGroupList.clear();
        outGroupList.reserve( allPfEvents.size() );
        BOOST_FOREACH( INode* node, allPfEvents ) {
            if( node ) {
                bool keep = true; // false;
                bool include = false;
                if( selectedPfEventSet.find( node ) != selectedPfEventSet.end() ) {
                    include = true;
                }
                if( keep ) {
                    particle_group_filter_entry::ptr_type entry( new particle_flow_group_filter_entry( node ) );
                    entry->set_include( include );
                    outGroupList.push_back( entry );
                }
            }
        }
        /*
        outGroupList.clear();
        outGroupList.reserve( allPfEvents.size() );
        BOOST_FOREACH( INode * node, allPfEvents ) {
                if( node ) {
                        bool keep = false;
                        bool include = false;
                        if( frost->accept_particle_flow_event( node ) ) {
                                keep = true;
                                include = false;
                        } else if( frost->has_particle_flow_event( node ) ) {
                                keep = true;
                                include = true;
                        }
                        if( keep ) {
                                particle_group_filter_entry::ptr_type entry( new particle_flow_group_filter_entry( node
        ) ); entry->set_include( include ); outGroupList.push_back( entry );
                        }
                }
        }
        */
    }

    static const TCHAR* get_mxs_options_callback() { return _T("on_PFEventListOptions_pressed"); }

    static const TCHAR* get_mxs_right_click_callback() { return _T("on_PFEventList_rightClicked"); }

    static const TCHAR* get_dlg_proc_name() { return _T("particle_flow_events_dlg_proc"); }

    static const TCHAR* get_all_radio_label() { return _T("All Events"); }

    static const TCHAR* get_selected_radio_label() { return _T("Selected Events:"); }

    static const TCHAR* get_particle_system_type() { return _T("Particle Flow"); }

    static const TCHAR* get_group_type_plural() { return _T("Events"); }
};

typedef group_dlg_proc_base<particle_flow_events_dlg_proc_traits> particle_flow_events_dlg_proc;
