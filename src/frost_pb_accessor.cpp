// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "Frost.hpp"
#include "frost_pb_accessor.hpp"

#include "frost_main_dlg_proc.hpp"
#include "geometry_dlg_proc.hpp"
#include "meshing_dlg_proc.hpp"
#include "particle_files_dlg_proc.hpp"
#include "particle_flow_events_dlg_proc.hpp"
#include "particle_objects_dlg_proc.hpp"

void frost_pb_accessor::Set( PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t ) {
    Frost* obj = (Frost*)owner;
    if( !obj ) {
        return;
    }
    IParamBlock2* pb = obj->GetParamBlockByID( frost_param_block );
    if( !pb ) {
        return;
    }

    try {
        obj->on_param_set( v, id, tabIndex, t );
    } catch( std::exception& e ) {
        frantic::tstring errmsg = _T("Frost: Set: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        FF_LOG( error ) << errmsg << std::endl;
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

void frost_pb_accessor::TabChanged( tab_changes changeCode, Tab<PB2Value>* tab, ReferenceMaker* owner, ParamID id,
                                    int tabIndex, int count ) {
    Frost* obj = (Frost*)owner;
    if( !obj ) {
        return;
    }
    IParamBlock2* pb = obj->GetParamBlockByID( frost_param_block );
    if( !pb ) {
        return;
    }

    try {
        obj->on_tab_changed( changeCode, tab, id, tabIndex, count );
    } catch( std::exception& e ) {
        frantic::tstring errmsg = _T("Frost: TabChanged: ") + frantic::strings::to_tstring( e.what() ) + _T("\n");
        FF_LOG( error ) << errmsg << std::endl;
        LogSys* log = GetCOREInterface()->Log();
        log->LogEntry( SYSLOG_ERROR, NO_DIALOG, _T("Frost Error"), _T("%s"), errmsg.c_str() );
        if( is_network_render_server() ) {
            throw MAXException( const_cast<TCHAR*>( errmsg.c_str() ) );
        }
    }
}

frost_pb_accessor* GetFrostPBAccessor() {
    static frost_pb_accessor frostPBAccessor;
    return &frostPBAccessor;
}
