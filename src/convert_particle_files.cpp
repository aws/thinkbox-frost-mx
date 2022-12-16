// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include "resource.h"

#include "convert_particle_files.hpp"

#include <tbb/atomic.h>
#include <tbb/task_scheduler_init.h>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <frantic/files/filename_sequence.hpp>
#include <frantic/files/files.hpp>

#include <frantic/particles/particle_array.hpp>
#include <frantic/particles/streams/apply_function_particle_istream.hpp>
#include <frantic/particles/streams/particle_istream.hpp>
#include <frantic/particles/streams/prt_particle_ostream.hpp>

#include "utility.hpp"

extern HINSTANCE ghInstance;

namespace {

frantic::tstring to_human_readable( boost::int64_t number ) {
    if( number >= 1000000000 ) {
        std::basic_stringstream<TCHAR> ss;
        ss << boost::basic_format<TCHAR>( _T("%.2f G") ) % ( number / 1.e9 );
        return ss.str();
    } else if( number >= 1000000 ) {
        std::basic_stringstream<TCHAR> ss;
        ss << boost::basic_format<TCHAR>( _T("%.2f M") ) % ( number / 1.e6 );
        return ss.str();
    } else if( number >= 1000 ) {
        return boost::lexical_cast<frantic::tstring>( number / 1000 ) + _T(" k");
    } else {
        return boost::lexical_cast<frantic::tstring>( number );
    }
}

bool in_closed_interval( int i, int first, int last ) { return i >= first && i <= last; }

} // namespace

template <typename In, typename Out, typename Pred>
Out copy_if( In first, In last, Out res, Pred Pr ) {
    while( first != last ) {
        if( Pr( *first ) )
            *res++ = *first;
        ++first;
    }
    return res;
}

convert_particle_files_params::convert_particle_files_params()
    : colorScale( 1.f ) {}

// 3ds Max seems to have started sending WM_USER on button click.
// Switch to a higher value (WM_USER+2) for now.
// TODO: Something else ?  Set a separate "done" flag when sending
// WM_USER_DONE, so we can tell if it's the real message?
#define WM_USER_DONE ( WM_USER + 2 )
#define WM_USER_SET_LINE ( WM_USER + 3 )

/*
class progress_dialog_info {
public:
        virtual ~progress_dialog_info() {
        }

        virtual void set_title( const std::string & title ) = 0;
        virtual const std::string & get_title() = 0;

        virtual void set_line( int line, const std::string & s ) = 0;
        virtual const std::string & get_line( int line ) = 0;

        virtual bool is_cancelled() = 0;
        virtual void set_cancelled() = 0;

        virtual void set_hwnd( HWND hwnd ) = 0;

        virtual void set_done( UINT_PTR code ) = 0;
};
*/

class progress_dialog_info {
  protected:
    HWND m_hwnd;
    frantic::tstring m_title;
    frantic::tstring m_line0;
    frantic::tstring m_line1;
    frantic::tstring m_line2;
    frantic::tstring m_line3;
    frantic::tstring m_line4;
    frantic::tstring m_emptyString;
    tbb::atomic<int> m_isCancelled;

  public:
    progress_dialog_info()
        : m_hwnd( 0 ) {
        m_isCancelled = 0;
    }

    virtual ~progress_dialog_info() {}

    void set_title( const frantic::tstring& title ) { m_title = title; }

    void set_line( int line, const frantic::tstring& s ) {
        switch( line ) {
        case 0:
            m_line0 = s;
            break;
        case 1:
            m_line1 = s;
            break;
        case 2:
            m_line2 = s;
            break;
        case 3:
            m_line3 = s;
            break;
        case 4:
            m_line4 = s;
            break;
        }
        if( m_hwnd ) {
            PostMessage( m_hwnd, WM_USER_SET_LINE, 0, 0 );
        }
    }

    bool is_cancelled() { return ( m_isCancelled != 0 ); }

    void set_cancelled() { m_isCancelled = 1; }

    void set_hwnd( HWND hwnd ) { m_hwnd = hwnd; }

    void set_done( UINT_PTR code ) {
        if( m_hwnd ) {
            PostMessage( m_hwnd, WM_USER_DONE, code, 0 );
        }
    }

    const frantic::tstring& get_title() { return m_title; }

    const frantic::tstring& get_line( int line ) {
        switch( line ) {
        case 0:
            return m_line0;
        case 1:
            return m_line1;
        case 2:
            return m_line2;
        case 3:
            return m_line3;
        case 4:
            return m_line4;
        default:
            return m_emptyString;
        }
    }
};

class conversion_progress_logger {
  public:
    conversion_progress_logger() {}
    ~conversion_progress_logger() {}
    virtual void set_total_files( std::size_t /*fileCount*/ ) {}
    virtual void set_current_file( std::size_t /*currentFile*/ ) {}
    virtual void set_progress_particles( boost::int64_t /*doneParticles*/ ) {}
    virtual bool is_cancelled() { return false; }
    virtual void set_done( UINT_PTR /*result*/ ) {}
};

class dialog_conversion_progress_logger : public conversion_progress_logger, public progress_dialog_info {
    std::size_t m_fileCount;

  public:
    dialog_conversion_progress_logger()
        : m_fileCount( 1 ) {
        progress_dialog_info::set_title( _T("Frost - Converting Particle Files") );
    }
    virtual void set_total_files( std::size_t fileCount ) { m_fileCount = fileCount; }
    virtual void set_current_file( std::size_t currentFile ) {
        set_line( 0, _T("Converting file ") + boost::lexical_cast<frantic::tstring>( currentFile + 1 ) + _T(" of ") +
                         boost::lexical_cast<frantic::tstring>( m_fileCount ) + _T(".") );
        set_line( 1, _T("Starting...") );
    }
    virtual void set_progress_particles( boost::int64_t doneParticles ) {
        set_line( 1, _T("Done ") + to_human_readable( doneParticles ) + _T(" particles...") );
    }
    virtual bool is_cancelled() { return progress_dialog_info::is_cancelled(); }
    virtual void set_done( UINT_PTR result ) { progress_dialog_info::set_done( result ); }
};

class detect_color_format_progress_logger {
  public:
    detect_color_format_progress_logger() {}
    ~detect_color_format_progress_logger() {}
    virtual void set_total_files( std::size_t /*fileCount*/ ) {}
    virtual void set_current_file( std::size_t /*currentFile*/ ) {}
    virtual void set_progress_particles( boost::int64_t /*doneParticles*/ ) {}
    virtual void set_minimum_color( double /*value*/ ) {}
    virtual void set_maximum_color( double /*value*/ ) {}
    virtual void set_got_decimal( bool /*value*/ ) {}
    virtual bool is_cancelled() { return false; }
    virtual void set_done( UINT_PTR /*result*/ ) {}
};

class dialog_detect_color_format_progress_logger : public detect_color_format_progress_logger,
                                                   public progress_dialog_info {
    std::size_t m_fileCount;

  public:
    dialog_detect_color_format_progress_logger()
        : m_fileCount( 1 ) {
        progress_dialog_info::set_title( _T("Frost - Scanning Particle Files") );
    }
    virtual void set_total_files( std::size_t fileCount ) { m_fileCount = fileCount; }
    virtual void set_current_file( std::size_t currentFile ) {
        set_line( 0, _T("Scanning file ") + boost::lexical_cast<frantic::tstring>( currentFile + 1 ) + _T(" of ") +
                         boost::lexical_cast<frantic::tstring>( m_fileCount ) + _T(".") );
        set_line( 1, _T("Starting...") );
    }
    virtual void set_progress_particles( boost::int64_t doneParticles ) {
        set_line( 1, _T("Done ") + to_human_readable( doneParticles ) + _T(" particles...") );
    }
    virtual void set_minimum_color( double value ) {
        set_line( 2, _T("Minimum: ") + boost::lexical_cast<frantic::tstring>( value ) );
    }
    virtual void set_maximum_color( double value ) {
        set_line( 3, _T("Maximum: ") + boost::lexical_cast<frantic::tstring>( value ) );
    }
    virtual void set_got_decimal( bool value ) {
        set_line( 4, frantic::tstring( _T("Found decimal: ") ) + ( value ? _T("yes") : _T("no") ) );
    }
    virtual bool is_cancelled() { return progress_dialog_info::is_cancelled(); }
    virtual void set_done( UINT_PTR result ) { progress_dialog_info::set_done( result ); }
};

class delete_file_on_scope_exit {
    frantic::tstring m_filename;

  public:
    delete_file_on_scope_exit( TCHAR* filename )
        : m_filename( filename ) {}
    ~delete_file_on_scope_exit() { DeleteFile( m_filename.c_str() ); }
};

frantic::channels::channel_map build_out_channel_map( const frantic::channels::channel_map& inChannelMap,
                                                      bool forceFloatColor, std::size_t alignmentPadding ) {
    frantic::channels::channel_map outChannelMap;
    for( std::size_t i = 0; i < inChannelMap.channel_count(); ++i ) {
        const frantic::channels::channel& ch = inChannelMap[i];
        if( forceFloatColor && ch.name() == _T("Color") ) {
            outChannelMap.define_channel( ch.name(), ch.arity(), frantic::channels::data_type_float32 );
        } else {
            outChannelMap.define_channel( ch.name(), ch.arity(), ch.data_type() );
        }
    }
    outChannelMap.end_channel_definition( alignmentPadding ); // 1 for file
    return outChannelMap;
}

void convert_file( const frantic::tstring& infile, const frantic::tstring& outfile, float colorScale,
                   conversion_progress_logger* progress ) {
    if( infile == outfile ) {
        throw std::runtime_error( "The input file and output file must be different, but they are both\n" +
                                  frantic::strings::to_string( infile ) );
    }

    // set in exception handler
    std::string errMsg = "";

    try {
        // needed because convert_file() can be run on a separate thread,
        // and apply_function_particle_istream uses tbb
        tbb::task_scheduler_init taskSchedulerInit;

        boost::filesystem::path filePath( outfile );
        const frantic::tstring prefixString = frantic::files::to_tstring( filePath.filename() );
        const frantic::tstring remoteDirectory = frantic::files::to_tstring( filePath.parent_path() );

        TCHAR tempFileName[MAX_PATH];
        UINT tempFileNameNumber = GetTempFileName( remoteDirectory.c_str(), prefixString.c_str(), 0, tempFileName );
        if( tempFileNameNumber == 0 ) {
            std::stringstream ss;
            ss << "Could not get temporary file for writing to \'" << frantic::strings::to_string( outfile ) << "\'."
               << std::endl;
            ss << "Error " << boost::lexical_cast<std::string>( GetLastError() ) << ": "
               << frantic::win32::GetLastErrorMessageA() << std::endl;
            throw std::runtime_error( ss.str() );
        }

        const bool forceFloatColor = colorScale != 1.f;
        boost::array<frantic::tstring, 1> colorScaleChannels = { _T("Color") };

        delete_file_on_scope_exit deleteFileOnScopeExit( tempFileName );

        { // scope for in/out streams and their file handles
            boost::shared_ptr<frantic::particles::streams::particle_istream> in =
                frantic::particles::particle_file_istream_factory( infile );
            if( colorScale != 1.f && in->get_channel_map().has_channel( _T("Color") ) ) {
                in->set_channel_map( build_out_channel_map( in->get_channel_map(), true, 4 ) );
                in.reset( new frantic::particles::streams::apply_function_particle_istream<frantic::graphics::vector3f(
                              const frantic::graphics::vector3f& )>( in, boost::bind( scale_vector, _1, colorScale ),
                                                                     _T("Color"), colorScaleChannels ) );
            }
            boost::shared_ptr<frantic::particles::streams::particle_ostream> out(
                new frantic::particles::streams::prt_particle_ostream(
                    tempFileName, in->get_channel_map(),
                    build_out_channel_map( in->get_channel_map(), forceFloatColor, 1 ) ) );
            frantic::particles::particle_array pa( in->get_channel_map() );
            const std::size_t BUFFER_SIZE = 50000;
            pa.resize( 50000 );
            boost::int64_t cumulativeParticleCount = 0;
            bool notDone;
            do {
                std::size_t particleCount = BUFFER_SIZE;
                notDone = in->get_particles( pa.at( 0 ), particleCount );
                for( std::size_t i = 0; i < particleCount; ++i ) {
                    out->put_particle( pa.at( i ) );
                }
                cumulativeParticleCount += particleCount;
                if( progress && progress->is_cancelled() ) {
                    break;
                }
                if( progress ) {
                    progress->set_progress_particles( cumulativeParticleCount );
                }
            } while( notDone );
        }

        if( !progress || !progress->is_cancelled() ) {
            BOOL moveResult = MoveFileEx( tempFileName, outfile.c_str(), MOVEFILE_REPLACE_EXISTING );
            if( moveResult == 0 ) {
                std::stringstream ss;
                ss << "The temporary file \'" << frantic::strings::to_string( tempFileName ) << "\' could not be moved"
                   << std::endl;
                ss << "to destination file \'" << frantic::strings::to_string( outfile ) << "\'" << std::endl;
                ss << "Error " << boost::lexical_cast<std::string>( moveResult ) << ": "
                   << frantic::win32::GetLastErrorMessageA() << std::endl;
                throw std::runtime_error( ss.str() );
            }
        }
    } catch( std::exception& e ) {
        errMsg = "Error converting file \'" + frantic::strings::to_string( infile ) + "\': " + e.what();
    }

    if( !errMsg.empty() ) {
        throw std::runtime_error( errMsg );
    }
}

class detect_color_format_worker_params {
    detect_color_format_worker_params();

    bool eval( boost::logic::tribool& outResult, frantic::tstring* outReason ) {
        outResult = boost::logic::indeterminate;
        if( outReason ) {
            *outReason = _T("");
        }

        if( gotFile ) {
            if( gotColorChannel ) {
                if( gotByteColorChannel ) {
                    outResult = true;
                    if( outReason ) {
                        *outReason = _T("Found byte Color channel.");
                    }
                    return true;
                } else {
                    // if( gotParticle ) {
                    if( gotColorParticle ) {
                        if( minColor >= 0 && maxColor <= 1.0 ) {
                            if( maxColor > 0 ) {
                                if( gotDecimal ) {
                                    outResult = false;
                                    return true;
                                } else {
                                    if( outReason ) {
                                        *outReason = _T("Found Color in range [0..1], but no decimal value found.");
                                    }
                                    return false;
                                }
                            } else {
                                if( outReason ) {
                                    *outReason = _T("All particles have 0 Color.");
                                }
                                return false;
                            }
                        } else if( minColor >= 0 ) {
                            if( maxColor <= 255.0 ) {
                                outResult = true;
                                return true;
                            } else {
                                if( outReason ) {
                                    *outReason = _T("Found out-of-range Color: ") +
                                                 boost::lexical_cast<frantic::tstring>( maxColor );
                                }
                                return true;
                            }
                        } else {
                            if( outReason ) {
                                *outReason =
                                    _T("Found negative Color: ") + boost::lexical_cast<frantic::tstring>( minColor );
                            }
                            return true;
                        }
                    } else {
                        if( outReason ) {
                            *outReason = _T("No particle with Color channel found.");
                        }
                        return false;
                    }
                    //} else {
                    // if( outReason ) {
                    //*outReason = "No particle found.";
                    //}
                    // return false;
                    //}
                }
            } else {
                if( outReason ) {
                    *outReason = _T("No Color channel found.");
                }
                return false;
            }
        } else {
            if( outReason ) {
                *outReason = _T("No file found.");
            }
            return false;
        }
    }

  public:
    detect_color_format_progress_logger* progress;
    frantic::tstring infile;
    int frameRangeMode;
    int firstFrame;
    int lastFrame;
    float colorScale;
    bool ignoreMissingInputFiles;

    bool gotFile;
    bool gotParticle;
    bool gotColorChannel;
    bool gotByteColorChannel;
    bool gotColorParticle;
    bool gotDecimal;
    double minColor;
    double maxColor;

    std::string errorMessage;

    detect_color_format_worker_params( detect_color_format_progress_logger* progress, const frantic::tstring& infile,
                                       int frameRangeMode, int firstFrame, int lastFrame, bool ignoreMissingInputFiles )
        : progress( progress )
        , infile( infile )
        , frameRangeMode( frameRangeMode )
        , firstFrame( firstFrame )
        , lastFrame( lastFrame )
        , ignoreMissingInputFiles( ignoreMissingInputFiles )
        , colorScale( 1.f )
        , gotFile( false )
        , gotParticle( false )
        , gotColorChannel( false )
        , gotColorParticle( false )
        , gotDecimal( false )
        , minColor( std::numeric_limits<double>::max() )
        , maxColor( -std::numeric_limits<double>::max() ) {}

    bool is_done_decision() {
        boost::logic::tribool temp;
        return eval( temp, 0 );
    }

    boost::logic::tribool get_result( frantic::tstring* reason = 0 ) {
        boost::logic::tribool result;
        eval( result, reason );
        return result;
    }
};

void detect_file_color_format( const frantic::tstring& infile, detect_color_format_worker_params& params,
                               detect_color_format_progress_logger* progress ) {
    // set in exception handler
    std::string errMsg = "";

    try {
        boost::shared_ptr<frantic::particles::streams::particle_istream> in =
            frantic::particles::particle_file_istream_factory( infile );

        params.gotFile = true;

        if( in->get_channel_map().has_channel( _T("Color") ) ) {
            frantic::channels::channel_general_accessor generalAcc =
                in->get_channel_map().get_general_accessor( _T("Color") );
            if( generalAcc.arity() == 3 ) {
                params.gotColorChannel = true;

                if( generalAcc.data_type() == frantic::channels::data_type_uint8 ||
                    generalAcc.data_type() == frantic::channels::data_type_int8 ) {
                    params.gotByteColorChannel = true;
                } else {
                    frantic::channels::channel_const_cvt_accessor<frantic::graphics::vector3fd> acc =
                        in->get_channel_map().get_const_cvt_accessor<frantic::graphics::vector3fd>( _T("Color") );

                    frantic::particles::particle_array pa( in->get_channel_map() );
                    const std::size_t BUFFER_SIZE = 50000;
                    pa.resize( 50000 );
                    boost::int64_t cumulativeParticleCount = 0;

                    bool notDone;
                    do {
                        std::size_t particleCount = BUFFER_SIZE;
                        notDone = in->get_particles( pa.at( 0 ), particleCount );

                        if( particleCount > 0 ) {
                            params.gotColorParticle = true;
                        }

                        for( std::size_t i = 0; i < particleCount; ++i ) {
                            const frantic::graphics::vector3fd particleColor = acc( pa.at( i ) );
                            if( !params.gotDecimal ) {
                                for( int axis = 0; axis < 3; ++axis ) {
                                    if( particleColor[axis] > 0.0 && particleColor[axis] < 1.0 ) {
                                        params.gotDecimal = true;
                                        break;
                                    }
                                }
                            }
                            params.minColor = std::min<double>(
                                params.minColor,
                                std::min( particleColor.x, std::min( particleColor.y, particleColor.z ) ) );
                            params.maxColor = std::max<double>(
                                params.maxColor,
                                std::max( particleColor.x, std::max( particleColor.y, particleColor.z ) ) );
                        }

                        /*
                        progress->set_minimum_color( params.minColor );
                        progress->set_maximum_color( params.maxColor );
                        progress->set_got_decimal( params.gotDecimal );
                        */

                        if( params.is_done_decision() ) {
                            notDone = false;
                        }

                        cumulativeParticleCount += particleCount;
                        if( progress && progress->is_cancelled() ) {
                            break;
                        }
                        if( progress ) {
                            progress->set_progress_particles( cumulativeParticleCount );
                        }
                    } while( notDone );
                }
            }
        }
    } catch( std::exception& e ) {
        errMsg = "Error scanning file \'" + frantic::strings::to_string( infile ) + "\': " + e.what();
    }

    if( !errMsg.empty() ) {
        throw std::runtime_error( errMsg );
    }
}

class convert_particle_files_worker_params {
    convert_particle_files_worker_params();

  public:
    conversion_progress_logger* progress;
    frantic::tstring infile;
    frantic::tstring outfile;
    int frameRangeMode;
    int firstFrame;
    int lastFrame;
    float colorScale;
    bool ignoreMissingInputFiles;
    bool replaceOutputFiles;
    std::string errorMessage;

    convert_particle_files_worker_params( conversion_progress_logger* progress, const frantic::tstring& infile,
                                          const frantic::tstring& outfile, int frameRangeMode, int firstFrame,
                                          int lastFrame, bool ignoreMissingInputFiles, bool replaceOutputFiles )
        : progress( progress )
        , infile( infile )
        , outfile( outfile )
        , frameRangeMode( frameRangeMode )
        , firstFrame( firstFrame )
        , lastFrame( lastFrame )
        , ignoreMissingInputFiles( ignoreMissingInputFiles )
        , replaceOutputFiles( replaceOutputFiles )
        , colorScale( 1.f ) {}
};

DWORD WINAPI convert_particle_files_worker( LPVOID inState ) {
    convert_particle_files_worker_params* state = reinterpret_cast<convert_particle_files_worker_params*>( inState );
    // these may be modified by proceed()
    bool ignoreMissingInputFiles = state->ignoreMissingInputFiles;
    bool replaceOutputFiles = state->replaceOutputFiles;
    DWORD returnResult = 2;

    try {
        if( state->frameRangeMode == convert_particle_files_params::SINGLE ) {
            state->progress->set_total_files( 1 );
            state->progress->set_current_file( 0 );
            if( !replaceOutputFiles && frantic::files::file_exists( state->outfile ) ) {
                throw std::runtime_error( frantic::strings::to_string( state->outfile ) + " already exists." );
            }
            convert_file( state->infile, state->outfile, state->colorScale, state->progress );
        } else if( state->frameRangeMode == convert_particle_files_params::RANGE ) {
            if( state->firstFrame > state->lastFrame ) {
                throw std::runtime_error(
                    "First frame number in range cannot be greater than last frame number in range." );
            }
            frantic::files::filename_sequence inSequence( state->infile );
            frantic::files::filename_sequence outSequence( state->outfile );
            state->progress->set_total_files( state->lastFrame - state->firstFrame + 1 );
            for( int frameNumber = state->firstFrame; frameNumber <= state->lastFrame; ++frameNumber ) {
                const frantic::tstring infile = inSequence[frameNumber];
                const frantic::tstring outfile = outSequence[frameNumber];
                state->progress->set_current_file( frameNumber - state->firstFrame );
                if( frantic::files::file_exists( infile ) ) {
                    if( !replaceOutputFiles && frantic::files::file_exists( outfile ) ) {
                        throw std::runtime_error( frantic::strings::to_string( outfile ) + " already exists." );
                    }
                    convert_file( infile, outfile, state->colorScale, state->progress );
                } else {
                    if( !ignoreMissingInputFiles ) {
                        throw std::runtime_error( "Input file \'" + frantic::strings::to_string( infile ) +
                                                  "\' could not be found." );
                    }
                }
                if( state->progress->is_cancelled() ) {
                    break;
                }
            }
        } else {
            throw std::runtime_error( "Internal Error: Unrecognized frame range mode: " +
                                      boost::lexical_cast<std::string>( state->frameRangeMode ) );
        }
        returnResult = 0;
    } catch( std::exception& e ) {
        state->errorMessage = e.what();
        returnResult = 2;
    } catch( ... ) {
        state->errorMessage = "An unknown error occurred.";
        returnResult = 2;
    }
    if( state->progress->is_cancelled() ) {
        returnResult = 1;
    }
    state->progress->set_done( returnResult );
    return returnResult;
}

DWORD WINAPI detect_color_format_worker( LPVOID inState ) {
    detect_color_format_worker_params* state = reinterpret_cast<detect_color_format_worker_params*>( inState );
    // these may be modified by proceed()
    bool ignoreMissingInputFiles = state->ignoreMissingInputFiles;
    DWORD returnResult = 2;

    try {
        if( state->frameRangeMode == convert_particle_files_params::SINGLE ) {
            state->progress->set_total_files( 1 );
            state->progress->set_current_file( 0 );
            detect_file_color_format( state->infile, *state, state->progress );
        } else if( state->frameRangeMode == convert_particle_files_params::RANGE ) {
            if( state->firstFrame > state->lastFrame ) {
                throw std::runtime_error(
                    "First frame number in range cannot be greater than last frame number in range." );
            }
            frantic::files::filename_sequence inSequence( state->infile );
            state->progress->set_total_files( state->lastFrame - state->firstFrame + 1 );
            for( int frameNumber = state->firstFrame; frameNumber <= state->lastFrame; ++frameNumber ) {
                const frantic::tstring infile = inSequence[frameNumber];
                state->progress->set_current_file( frameNumber - state->firstFrame );
                if( frantic::files::file_exists( infile ) ) {
                    detect_file_color_format( state->infile, *state, state->progress );
                } else {
                    if( !ignoreMissingInputFiles ) {
                        throw std::runtime_error( "Input file \'" + frantic::strings::to_string( infile ) +
                                                  "\' could not be found." );
                    }
                }
                if( state->progress->is_cancelled() ) {
                    break;
                }
                if( state->is_done_decision() ) {
                    break;
                }
            }
        } else {
            throw std::runtime_error( "Internal Error: Unrecognized frame range mode: " +
                                      boost::lexical_cast<std::string>( state->frameRangeMode ) );
        }
        returnResult = 0;
    } catch( std::exception& e ) {
        state->errorMessage = e.what();
        returnResult = 2;
    } catch( ... ) {
        state->errorMessage = "An unknown error occurred.";
        returnResult = 2;
    }
    if( state->progress->is_cancelled() ) {
        returnResult = 1;
    }
    state->progress->set_done( returnResult );
    return returnResult;
}

class progress_dlg_proc {
    progress_dialog_info* progress;
    LPTHREAD_START_ROUTINE lpStartAddress;
    LPVOID lpParameter;
    DWORD threadId;
    HANDLE hThread;
    progress_dlg_proc();

  public:
    progress_dlg_proc( progress_dialog_info* progress, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter )
        : hThread( 0 )
        , threadId( 0 )
        , progress( progress )
        , lpStartAddress( lpStartAddress )
        , lpParameter( lpParameter ) {
        if( !progress ) {
            throw std::runtime_error( "progress_dlg_proc Internal Error: progress object is NULL" );
        }
    }
    ~progress_dlg_proc() {
        if( hThread ) {
            CloseHandle( hThread );
        }
    }
    void join() {
        if( hThread ) {
            bool done = false;
            while( !done ) {
                DWORD result = WaitForMultipleObjectsEx( 1, &hThread, FALSE, 100, FALSE );

                if( result == WAIT_OBJECT_0 || result == WAIT_ABANDONED_0 ) {
                    done = true;
                } else if( result == WAIT_TIMEOUT ) {
                } else {
                    throw std::runtime_error(
                        "progress_dlg_proc Internal Error: Got unexpected thread polling result: " +
                        boost::lexical_cast<std::string>( result ) );
                }
            }
        }
        /*
        DWORD exitCode;
        BOOL gotExitCode = GetExitCodeThread( hThread, & exitCode );
        if( gotExitCode ) {
                if( exitCode == 0 ) {
                        // yay
                } else if( exitCode == 1 ) {
                        throw std::runtime_error( "Process Cancelled" );
                } else {
                        throw std::runtime_error( "Internal Error: Got unexpected return code from thread: " +
        boost::lexical_cast<std::string>( exitCode ) );
                }
        } else {
                throw std::runtime_error( "Internal Error: Could not get exit code from thread." );
        }
        return exitCode;
        */
    }
    static INT_PTR CALLBACK dlg_proc( __in HWND hwndDlg, __in UINT uMsg, __in WPARAM wParam, __in LPARAM lParam ) {
        try {
            progress_dlg_proc* pState = reinterpret_cast<progress_dlg_proc*>( GetWindowLongPtr( hwndDlg, DWLP_USER ) );
            switch( uMsg ) {
            case WM_INITDIALOG: {
                pState = reinterpret_cast<progress_dlg_proc*>( lParam );
                SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)pState );

                // default dialog
                if( pState ) {
                    SetWindowText( hwndDlg, pState->progress->get_title().c_str() );
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE0 ), pState->progress->get_line( 0 ).c_str() );
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE1 ), pState->progress->get_line( 1 ).c_str() );
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE2 ), pState->progress->get_line( 2 ).c_str() );
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE3 ), pState->progress->get_line( 3 ).c_str() );
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE4 ), pState->progress->get_line( 4 ).c_str() );
                }

                pState->progress->set_hwnd( hwndDlg );

                // launch worker thread
                pState->hThread = CreateThread( 0, 0, pState->lpStartAddress, pState->lpParameter, CREATE_SUSPENDED,
                                                &( pState->threadId ) );
                if( !pState->hThread ) {
                    throw std::runtime_error( "progress_dlg_proc Internal Error: Unable to create worker thread." );
                }

                DWORD result = ResumeThread( pState->hThread );
                if( result == -1 ) {
                    throw std::runtime_error( "progress_dlg_proc Internal Error: Unable to resume worker thread." );
                }
            } break;
            case WM_COMMAND: {
                const int id = LOWORD( wParam );
                switch( id ) {
                case IDCANCEL:
                    if( pState ) {
                        // set flag
                        pState->progress->set_cancelled();
                    } else {
                        EndDialog( hwndDlg, IDCANCEL );
                        return TRUE;
                    }
                    break;
                }
            } break;
            case WM_USER_DONE: {
                if( pState ) {
                    pState->join();
                }
                WPARAM result = wParam;
                EndDialog( hwndDlg, result );
                return TRUE;
            } break;
            case WM_USER_SET_LINE: {
                if( pState ) {
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE0 ), pState->progress->get_line( 0 ).c_str() );
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE1 ), pState->progress->get_line( 1 ).c_str() );
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE2 ), pState->progress->get_line( 2 ).c_str() );
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE3 ), pState->progress->get_line( 3 ).c_str() );
                    SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE4 ), pState->progress->get_line( 4 ).c_str() );
                }
            } break;
            }
        } catch( std::exception& e ) {
            MessageBox( hwndDlg, ( frantic::strings::to_tstring( std::string( "Error: " ) + e.what() ) ).c_str(),
                        _T("Frost"), MB_OK | MB_ICONEXCLAMATION );
        }
        return FALSE;
    }
};

bool accept_convert_particle_files_params( HWND hwnd, convert_particle_files_params& params ) {
    if( is_network_render_server() ) {
        return true;
    }

    try {
        params.ignoreMissingInputFiles = false;
        params.replaceOutputFiles = false;
        if( params.frameRangeMode == convert_particle_files_params::SINGLE ) {
            if( !frantic::files::file_exists( params.infile ) ) {
                throw std::runtime_error( "Missing input file: \'" + frantic::strings::to_string( params.infile ) +
                                          "\'" );
            }
            if( params.outfile.empty() ) {
                throw std::runtime_error( "No output file specified." );
            }
            if( params.infile == params.outfile ) {
                throw std::runtime_error( "Input file and output file must be different." );
            }
            if( frantic::files::file_exists( params.outfile ) ) {
                // prompt
                const frantic::tstring msg = params.outfile + _T(" already exists.\nDo you want to replace it?");
                int result = MessageBox( hwnd, msg.c_str(), _T("Confirm Save As"), MB_YESNO | MB_ICONEXCLAMATION );
                if( result != IDYES ) {
                    return false;
                }
                params.replaceOutputFiles = ( result == IDYES );
            }
        } else if( params.frameRangeMode == convert_particle_files_params::RANGE ) {
            if( params.firstFrame > params.lastFrame ) {
                throw std::runtime_error(
                    "First frame number in range cannot be greater than last frame number in range." );
            }
            frantic::files::filename_pattern inPattern( params.infile );
            if( inPattern.get_directory( false ).length() == 0 ) {
                throw std::runtime_error( "Input sequence does not have a directory." );
            }
            if( !frantic::files::directory_exists( inPattern.get_directory( false ) ) ) {
                throw std::runtime_error( "Input directory \'" +
                                          frantic::strings::to_string( inPattern.get_directory( false ) ) +
                                          "\' does not exist." );
            }

            if( params.outfile.empty() ) {
                throw std::runtime_error( "No output file specified." );
            }
            frantic::files::filename_pattern outPattern( params.outfile );
            if( outPattern.get_directory( false ).length() == 0 ) {
                throw std::runtime_error( "Output sequence path is missing a directory." );
            }
            if( !frantic::files::directory_exists( outPattern.get_directory( false ) ) ) {
                throw std::runtime_error( "Output directory \'" +
                                          frantic::strings::to_string( outPattern.get_directory( false ) ) +
                                          "\' does not exist." );
            }

            frantic::files::filename_sequence inSequence( params.infile );
            inSequence.sync_frame_set();
            const frantic::files::frame_set& frameSet = inSequence.get_frame_set();
            std::vector<int> allPresentWholeFrames;
            inSequence.wholeframe_numbers( allPresentWholeFrames );
            std::vector<int> presentFrameNumbers;
            ::copy_if( allPresentWholeFrames.begin(), allPresentWholeFrames.end(),
                       std::back_inserter( presentFrameNumbers ),
                       boost::bind( in_closed_interval, _1, params.firstFrame, params.lastFrame ) );
            if( presentFrameNumbers.size() == 0 ) {
                throw std::runtime_error( "No input files found." );
            }
            std::vector<int> missingWholeFrames;
            frameSet.missing_wholeframes( params.firstFrame, params.lastFrame, missingWholeFrames );
            if( missingWholeFrames.size() ) {
                frantic::tstring msg = _T("Missing input frames:\n");
                const std::size_t displayCount = 5;
                for( std::size_t i = 0; i < missingWholeFrames.size() && i < displayCount; ++i ) {
                    msg += boost::lexical_cast<frantic::tstring>( missingWholeFrames[i] );
                    if( i + 1 < displayCount || missingWholeFrames.size() > displayCount ) {
                        msg += _T(", ");
                    }
                }
                if( displayCount < missingWholeFrames.size() ) {
                    msg += _T("plus ") +
                           boost::lexical_cast<frantic::tstring>( missingWholeFrames.size() - displayCount ) +
                           _T(" others.");
                }
                msg += _T("\nDo you want to proceed?");
                int result =
                    MessageBox( hwnd, msg.c_str(), _T("Confirm Missing Frames"), MB_YESNO | MB_ICONEXCLAMATION );
                if( result != IDYES ) {
                    return false;
                }
                params.ignoreMissingInputFiles = ( result == IDYES );
            }

            bool gotOutSequence = false;
            frantic::files::filename_sequence outSequence( params.outfile );
            if( inSequence[0] == outSequence[0] ) {
                throw std::runtime_error( "Input file and output file must be different." );
            }
            try {
                outSequence.sync_frame_set();
                gotOutSequence = true;
            } catch( std::exception& ) {
            }
            if( gotOutSequence ) {
                std::vector<int> allWholeFrameNumbers;
                outSequence.wholeframe_numbers( allWholeFrameNumbers );
                std::vector<int> wholeFrameNumbers;
                ::copy_if( allWholeFrameNumbers.begin(), allWholeFrameNumbers.end(),
                           std::back_inserter( wholeFrameNumbers ),
                           boost::bind( in_closed_interval, _1, params.firstFrame, params.lastFrame ) );
                if( wholeFrameNumbers.size() ) {
                    frantic::tstring msg = _T("The following output files already exist:\n");
                    const std::size_t displayCount = 5;
                    for( std::size_t i = 0; i < wholeFrameNumbers.size() && i < displayCount; ++i ) {
                        msg += outSequence[wholeFrameNumbers[i]] + _T("\n");
                    }
                    if( displayCount < wholeFrameNumbers.size() ) {
                        msg += _T("Plus ") +
                               boost::lexical_cast<frantic::tstring>( wholeFrameNumbers.size() - displayCount ) +
                               _T(" others.");
                    }
                    msg += _T("\nDo you want to replace them?");
                    int result = MessageBox( hwnd, msg.c_str(), _T("Confirm Save As"), MB_YESNO | MB_ICONEXCLAMATION );
                    if( result != IDYES && result != IDNO ) {
                        return false;
                    }
                    params.replaceOutputFiles = ( result == IDYES );
                }
            }
        } else {
            throw std::runtime_error( "Internal Error: Unrecognized frame range mode: " +
                                      boost::lexical_cast<std::string>( params.frameRangeMode ) );
        }
        return true;
    } catch( std::exception& e ) {
        const std::string msg = std::string( e.what() );
        MessageBoxA( hwnd, msg.c_str(), "Error", MB_OK | MB_ICONEXCLAMATION );
    }
    return false;
}

bool convert_particle_files( const convert_particle_files_params& params ) {
    boost::shared_ptr<conversion_progress_logger> progress;

    if( is_network_render_server() ) {
        progress.reset( new conversion_progress_logger );
    } else {
        progress.reset( new dialog_conversion_progress_logger );
    }

    convert_particle_files_worker_params workerParams( progress.get(), params.infile, params.outfile,
                                                       params.frameRangeMode, params.firstFrame, params.lastFrame,
                                                       params.ignoreMissingInputFiles, params.replaceOutputFiles );
    workerParams.colorScale = params.colorScale;

    INT_PTR result = -1;

    if( is_network_render_server() ) {
        result = convert_particle_files_worker( &workerParams );
    } else {
        LPTHREAD_START_ROUTINE proc = &convert_particle_files_worker;
        progress_dlg_proc progressDialog( dynamic_cast<progress_dialog_info*>( progress.get() ), proc, &workerParams );
        result = DialogBoxParam( ghInstance, MAKEINTRESOURCE( IDD_CONVERT_PARTICLE_FILES_PROGRESS ),
                                 GetCOREInterface()->GetMAXHWnd(), &progress_dlg_proc::dlg_proc,
                                 (LPARAM)( &progressDialog ) );
    }
    if( result == -1 ) {
        throw std::runtime_error( "convert_particle_files Internal Error: conversion result is " +
                                  boost::lexical_cast<std::string>( result ) );
    } else if( result == 0 ) {
        return true;
    } else if( result == 1 ) {
        return false;
    } else {
        throw std::runtime_error( workerParams.errorMessage );
        // MessageBox( hwndDlg, ( std::string( "Error: " ) + workerParams.errorMessage ).c_str(), "Frost", MB_OK |
        // MB_ICONEXCLAMATION );
    }
}

bool detect_particle_files_color_scale( const convert_particle_files_params& params, boost::logic::tribool& outResult,
                                        frantic::tstring& outIndeterminateReason ) {
    boost::shared_ptr<detect_color_format_progress_logger> progress;

    outResult = boost::logic::indeterminate;
    outIndeterminateReason.assign( _T("") );

    if( is_network_render_server() ) {
        progress.reset( new detect_color_format_progress_logger );
    } else {
        progress.reset( new dialog_detect_color_format_progress_logger );
    }

    detect_color_format_worker_params workerParams( progress.get(), params.infile, params.frameRangeMode,
                                                    params.firstFrame, params.lastFrame,
                                                    params.ignoreMissingInputFiles );

    INT_PTR result = -1;

    if( is_network_render_server() ) {
        result = detect_color_format_worker( &workerParams );
    } else {
        LPTHREAD_START_ROUTINE proc = &detect_color_format_worker;
        progress_dlg_proc progressDialog( dynamic_cast<progress_dialog_info*>( progress.get() ), proc, &workerParams );
        result = DialogBoxParam( ghInstance, MAKEINTRESOURCE( IDD_CONVERT_PARTICLE_FILES_PROGRESS ),
                                 GetCOREInterface()->GetMAXHWnd(), &progress_dlg_proc::dlg_proc,
                                 (LPARAM)( &progressDialog ) );
    }
    if( result == -1 ) {
        throw std::runtime_error( "detect_particle_files_color_format Internal Error: conversion result is " +
                                  boost::lexical_cast<std::string>( result ) );
    } else if( result == 0 ) {
        outResult = workerParams.get_result( &outIndeterminateReason );
        return true;
    } else if( result == 1 ) {
        return false;
    } else {
        throw std::runtime_error( workerParams.errorMessage );
        // MessageBox( hwndDlg, ( std::string( "Error: " ) + workerParams.errorMessage ).c_str(), "Frost", MB_OK |
        // MB_ICONEXCLAMATION );
    }
}
