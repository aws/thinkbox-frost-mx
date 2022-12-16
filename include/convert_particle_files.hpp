// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <boost/logic/tribool.hpp>

struct convert_particle_files_params {
    frantic::tstring infile;
    frantic::tstring outfile;
    int frameRangeMode;
    int firstFrame;
    int lastFrame;
    bool ignoreMissingInputFiles;
    bool replaceOutputFiles;
    float colorScale;

    enum frame_range_mode_enum { SINGLE, RANGE };

    convert_particle_files_params();
};

bool accept_convert_particle_files_params( HWND hwnd, convert_particle_files_params& params );

bool convert_particle_files( const convert_particle_files_params& params );

bool detect_particle_files_color_scale( const convert_particle_files_params& params, boost::logic::tribool& result,
                                        frantic::tstring& outIndeterminateReason );
