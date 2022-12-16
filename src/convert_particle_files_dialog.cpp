// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
// todo : make sure input and output files are different
// todo : test frame sequence
// todo : test errors in input files
// todo : human-readable progress
// todo : add conversion file menu item

#include "stdafx.h"

#include "resource.h"

#include "convert_particle_files.hpp"
#include "convert_particle_files_dialog.hpp"

#include <tbb/atomic.h>

#include <boost/format.hpp>

#include <frantic/files/filename_sequence.hpp>
#include <frantic/files/files.hpp>
#include <frantic/particles/particle_array.hpp>
#include <frantic/particles/streams/particle_istream.hpp>
#include <frantic/particles/streams/particle_ostream.hpp>
#include <frantic/particles/streams/prt_particle_ostream.hpp>
#include <frantic/win32/wrap.hpp>

#include <frantic/max3d/max_utility.hpp>

extern HINSTANCE ghInstance;

class convert_particle_files_dlg_proc {
    static void update_infile_display( convert_particle_files_dlg_proc* pState, HWND hwndDlg );
    static void update_outfile_display( convert_particle_files_dlg_proc* pState, HWND hwndDlg );
    static void update_frame_range_mode_display( convert_particle_files_dlg_proc* pState, HWND hwndDlg );
    static void update_frame_range_display( convert_particle_files_dlg_proc* pState, HWND hwndDlg );
    static void update_frame_range_from_infile( convert_particle_files_dlg_proc* pState, HWND hwndDlg,
                                                bool showError = false );
    static void update_scale_color_byte_to_float_display( convert_particle_files_dlg_proc* pState, HWND hwndDlg );
    void set_infile( const frantic::tstring& infile, bool updateOutputFile, HWND hwndDlg );

  public:
    frantic::tstring infile;
    frantic::tstring outfile;
    int frameRangeMode;
    int firstFrame;
    int lastFrame;
    bool ignoreMissingInputFiles;
    bool replaceOutputFiles;
    bool autoUpdateOutputFile;
    frantic::tstring notification;
    bool lockInfile;
    bool scaleColorByteToFloat;

    convert_particle_files_dlg_proc();

    static INT_PTR CALLBACK dlg_proc( __in HWND hwndDlg, __in UINT uMsg, __in WPARAM wParam, __in LPARAM lParam );
};

#if 0
class progress_dialog_state {
protected:
	HWND m_hwnd;
	std::string m_title;
	std::string m_line0;
	std::string m_line1;
	std::string m_line2;
	std::string m_emptyString;
	tbb::atomic<int> m_isCancelled;
public:
	progress_dialog_state()
		:	m_hwnd( 0 )
	{
		m_isCancelled = 0;
	}

	virtual ~progress_dialog_state() {
	}

	void set_title( const std::string & title ) {
		m_title = title;
	}

	void set_line( int line, const std::string & s ) {
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
		}
		if( m_hwnd ) {
			PostMessage( m_hwnd, WM_USER_SET_LINE, 0, 0 );
		}
	}

	bool is_cancelled() {
		return ( m_isCancelled != 0 );
	}

	void set_cancelled() {
		m_isCancelled = 1;
	}

	void set_hwnd( HWND hwnd ) {
		m_hwnd = hwnd;
	}

	void set_done( UINT_PTR code ) {
		if( m_hwnd ) {
			PostMessage( m_hwnd, WM_USER_DONE, code, 0 );
		}
	}

	const std::string & get_title() {
		return m_title;
	}

	const std::string & get_line( int line ) {
		switch( line ) {
			case 0:
				return m_line0;
			case 1:
				return m_line1;
			case 2:
				return m_line2;
			default:
				return m_emptyString;
		}
	}
};

class conversion_progress_dialog : public progress_dialog_state {
	std::size_t m_fileCount;
public:
	conversion_progress_dialog()
		:	m_fileCount( 1 )
	{
		m_title = "Frost - Converting Particle Files";
	}

	void set_total_files( std::size_t fileCount ) {
		m_fileCount = fileCount;
	}
	
	void set_current_file( std::size_t currentFile ) {
		set_line( 0, "Converting file " + boost::lexical_cast<std::string>( currentFile + 1 ) + " of " + boost::lexical_cast<std::string>( m_fileCount ) + "." );
		set_line( 1, "Starting..." );
	}

	void set_progress_particles( boost::int64_t doneParticles ) {
		set_line( 1, "Done " + to_human_readable( doneParticles ) + " particles..." );
	}
};

class convert_files_progress_dlg_proc {
	progress_dialog_state * progress;
	LPTHREAD_START_ROUTINE lpStartAddress;
	LPVOID lpParameter;
	DWORD threadId;
	HANDLE hThread;
	convert_files_progress_dlg_proc();
public:
	convert_files_progress_dlg_proc( progress_dialog_state & progress, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter )
		:	hThread( 0 ),
			threadId( 0 ),
			progress( & progress ),
			lpStartAddress( lpStartAddress ),
			lpParameter( lpParameter )
	{
	}
	~convert_files_progress_dlg_proc() {
		if( hThread ) {
			CloseHandle( hThread );
		}
	}
	void join() {
		if( hThread ) {
			bool done = false;
			while( ! done ) {
				DWORD result = WaitForMultipleObjectsEx( 1, & hThread, FALSE, 100, FALSE );

				if( result == WAIT_OBJECT_0 || result == WAIT_ABANDONED_0 ) {
					done = true;
				} else if( result == WAIT_TIMEOUT ) {
				} else {
					throw std::runtime_error( "Internal Error: Got unexpected thread polling result: " + boost::lexical_cast<std::string>( result ) );
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
				throw std::runtime_error( "Internal Error: Got unexpected return code from thread: " + boost::lexical_cast<std::string>( exitCode ) );
			}
		} else {
			throw std::runtime_error( "Internal Error: Could not get exit code from thread." );
		}
		return exitCode;
		*/
	}
	static INT_PTR CALLBACK dlg_proc(
	  __in  HWND hwndDlg,
	  __in  UINT uMsg,
	  __in  WPARAM wParam,
	  __in  LPARAM lParam )
	{
		try {
			convert_files_progress_dlg_proc * pState = reinterpret_cast<convert_files_progress_dlg_proc *>( GetWindowLongPtr( hwndDlg, DWLP_USER ) );
			switch( uMsg ) {
				case WM_INITDIALOG:
					{
						pState = reinterpret_cast<convert_files_progress_dlg_proc*>( lParam );
						SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR) pState );

						// default dialog
						if( pState ) {
							SetWindowText( hwndDlg, pState->progress->get_title().c_str() );
							SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE0 ), pState->progress->get_line( 0 ).c_str() );
							SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE1 ), pState->progress->get_line( 1 ).c_str() );
							SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE2 ), pState->progress->get_line( 2 ).c_str() );
						}

						pState->progress->set_hwnd( hwndDlg );

						// launch worker thread
						pState->hThread = CreateThread( 0, 0, pState->lpStartAddress, pState->lpParameter, CREATE_SUSPENDED, & (pState->threadId) );
						if( ! pState->hThread ) {
							throw std::runtime_error( "Internal Error: Unable to create worker thread." );
						}

						DWORD result = ResumeThread( pState->hThread );
						if( result == -1 ) {
							throw std::runtime_error( "Internal Error: Unable to resume worker thread." );
						} 
					}
					break;
				case WM_COMMAND:
					{
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
					}
					break;
				case WM_USER_DONE:
					{
						if( pState ) {
							pState->join();
						}
						WPARAM result = wParam;
						EndDialog( hwndDlg, result );
						return TRUE;
					}
					break;
				case WM_USER_SET_LINE:
					{
						if( pState ) {
							SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE0 ), pState->progress->get_line( 0 ).c_str() );
							SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE1 ), pState->progress->get_line( 1 ).c_str() );
							SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_LINE2 ), pState->progress->get_line( 2 ).c_str() );
						}
					}
					break;
			}
		} catch( std::exception & e ) {
			MessageBox( hwndDlg, ( std::string( "Error: " ) + e.what() ).c_str(), "Frost", MB_OK | MB_ICONEXCLAMATION );
		}
		return FALSE;
	}
};

class delete_file_on_scope_exit {
	std::string m_filename;
public:
	delete_file_on_scope_exit( TCHAR * filename )
		:	m_filename( filename )
	{
	}
	~delete_file_on_scope_exit() {
		DeleteFile( m_filename.c_str() );
	}
};

void convert_file( const std::string & infile, const std::string & outfile, conversion_progress_dialog * progress ) {
	boost::filesystem::path filePath( outfile );
	const std::string prefixString = filePath.filename().string();
	const std::string remoteDirectory = filePath.parent_path().string();
	
	TCHAR tempFileName[MAX_PATH];
	UINT tempFileNameNumber = GetTempFileName( remoteDirectory.c_str(), prefixString.c_str(), 0, tempFileName );
	if( tempFileNameNumber == 0 ) {
		std::stringstream ss;
		ss << "Could not get temporary file for writing to \'" << outfile << "\'." << std::endl;
		ss << "Error " << boost::lexical_cast<std::string>( GetLastError() ) << ": " << frantic::win32::GetLastErrorMessage() << std::endl;
		throw std::runtime_error( ss.str() );
	}

	delete_file_on_scope_exit deleteFileOnScopeExit( tempFileName );

	{ // scope for in/out streams and their file handles
		boost::shared_ptr<frantic::particles::streams::particle_istream> in = frantic::particles::particle_file_istream_factory( infile );
		boost::shared_ptr<frantic::particles::streams::particle_ostream> out( new frantic::particles::streams::prt_particle_ostream( tempFileName, in->get_channel_map(), in->get_channel_map() ) );
		frantic::particles::particle_array pa( in->get_channel_map() );
		const std::size_t BUFFER_SIZE = 50000;
		pa.resize( 50000 );
		std::size_t particleCount = BUFFER_SIZE;
		boost::int64_t cumulativeParticleCount = 0;
		while( in->get_particles( pa.at( 0 ), particleCount ) ) {
			for( std::size_t i = 0; i < particleCount; ++i ) {
				out->put_particle( pa.at( i ) );
			}
			cumulativeParticleCount += particleCount;
			particleCount = BUFFER_SIZE;
			if( progress && progress->is_cancelled() ) {
				break;
			}
			if( progress ) {
				progress->set_progress_particles( cumulativeParticleCount );
			}
		}
		progress->set_cancelled();
	}

	if( ! progress || ! progress->is_cancelled() ) {
		BOOL moveResult = MoveFileEx( tempFileName, outfile.c_str(), MOVEFILE_REPLACE_EXISTING );
		if( moveResult == 0 ) {
			std::stringstream ss;
			ss << "The temporary file \'" << tempFileName << "\' could not be moved" << std::endl;
			ss << "to destination file \'" << outfile << "\'" << std::endl;
			ss << "Error " << boost::lexical_cast<std::string>( moveResult ) << ": " << frantic::win32::GetLastErrorMessage() << std::endl;
			throw std::runtime_error( ss.str() );
		}
	}
}

//post progress messages to the progress window ?
//WM_USER with new information ?

class convert_particle_files_threadproc_params {
	convert_particle_files_threadproc_params();
public:
	conversion_progress_dialog * progress;
	std::string infile;
	std::string outfile;
	int frameRangeMode;
	int firstFrame;
	int lastFrame;
	std::string errorMessage;
	bool ignoreMissingInputFiles;
	bool replaceOutputFiles;

	convert_particle_files_threadproc_params( conversion_progress_dialog & progress, const std::string & infile, const std::string & outfile, int frameRangeMode, int firstFrame, int lastFrame, bool ignoreMissingInputFiles, bool replaceOutputFiles )
		:	progress( & progress ),
			infile( infile ),
			outfile( outfile ),
			frameRangeMode( frameRangeMode ),
			firstFrame( firstFrame ),
			lastFrame( lastFrame ),
			ignoreMissingInputFiles( ignoreMissingInputFiles ),
			replaceOutputFiles( replaceOutputFiles )
	{
	}
};

// copy_if was dropped from the standard library by accident.
template<typename In, typename Out, typename Pred>
Out copy_if( In first, In last, Out res, Pred Pr ) {
	while (first != last) {
		if (Pr(*first))
			*res++ = *first;
		++first;
	}
	return res;
} 

bool in_closed_interval( int i, int first, int last ) {
	return i >= first && i <= last;
}

bool accept_conversion_parameters( HWND hwnd, const convert_particle_files_dlg_proc & state, bool & ignoreMissingInputFiles, bool & replaceOutputFiles ) {
	try {
		ignoreMissingInputFiles = false;
		replaceOutputFiles = false;
		if( state.frameRangeMode == FRAME_RANGE_MODE::SINGLE ) {
			if( ! frantic::files::file_exists( state.infile ) ) {
				throw std::runtime_error( "Missing input file: \'" + state.infile + "\'" );
			}
			if( state.outfile.empty() ) {
				throw std::runtime_error( "No output file specified." );
			}
			if( state.infile == state.outfile ) {
				throw std::runtime_error( "Input file and output file must be different." );
			}
			if( frantic::files::file_exists( state.outfile ) ) {
				//prompt
				const std::string msg = state.outfile + " already exists.\nDo you want to replace it?";
				int result = MessageBox( hwnd, msg.c_str(), "Confirm Save As", MB_YESNO | MB_ICONEXCLAMATION );
				if( result != IDYES ) {
					return false;
				}
				replaceOutputFiles = ( result == IDYES );
			}
		} else if( state.frameRangeMode == FRAME_RANGE_MODE::RANGE ) {
			if( state.firstFrame > state.lastFrame ) {
				throw std::runtime_error( "First frame number in range cannot be greater than last frame number in range." );
			}
			frantic::files::filename_pattern inPattern( state.infile );
			if( inPattern.get_directory( false ).length() == 0 ) {
				throw std::runtime_error( "Input sequence does not have a directory." );
			}
			if( ! frantic::files::directory_exists( inPattern.get_directory( false ) ) ) {
				throw std::runtime_error( "Input directory \'" + inPattern.get_directory( false ) + "\' does not exist." );
			}
			
			if( state.outfile.empty() ) {
				throw std::runtime_error( "No output file specified." );
			}
			frantic::files::filename_pattern outPattern( state.outfile );
			if( outPattern.get_directory( false ).length() == 0 ) {
				throw std::runtime_error( "Output sequence path is missing a directory." );
			}
			if( ! frantic::files::directory_exists( outPattern.get_directory( false ) ) ) {
				throw std::runtime_error( "Output directory \'" + outPattern.get_directory( false ) + "\' does not exist." );
			}
			
			frantic::files::filename_sequence inSequence( state.infile );
			inSequence.sync_frame_set();
			const frantic::files::frame_set & frameSet = inSequence.get_frame_set();
			std::vector<int> allPresentWholeFrames;
			inSequence.wholeframe_numbers( allPresentWholeFrames );
			std::vector<int> presentFrameNumbers;
			copy_if( allPresentWholeFrames.begin(), allPresentWholeFrames.end(), std::back_inserter( presentFrameNumbers ), boost::bind( in_closed_interval, _1, state.firstFrame, state.lastFrame ) );
			if( presentFrameNumbers.size() == 0 ) {
				throw std::runtime_error( "No input files found." );
			}
			std::vector<int> missingWholeFrames;
			frameSet.missing_wholeframes( state.firstFrame, state.lastFrame, missingWholeFrames );
			if( missingWholeFrames.size() ) {
				std::string msg = "Missing input frames:\n";
				const std::size_t displayCount = 5;
				for( std::size_t i = 0; i < missingWholeFrames.size() && i < displayCount; ++i ) {
					msg += boost::lexical_cast<std::string>( missingWholeFrames[i] );
					if( i + 1 < displayCount || missingWholeFrames.size() > displayCount ) {
						msg += ", ";
					}
				}
				if( displayCount < missingWholeFrames.size() ) {
					msg += "plus " + boost::lexical_cast<std::string>( missingWholeFrames.size() - displayCount ) + " others.";
				}
				msg += "\nDo you want to proceed?";
				int result = MessageBox( hwnd, msg.c_str(), "Confirm Missing Frames", MB_YESNO | MB_ICONEXCLAMATION );
				if( result != IDYES ) {
					return false;
				}
				ignoreMissingInputFiles = ( result == IDYES );
			}
			
			bool gotOutSequence = false;
			frantic::files::filename_sequence outSequence( state.outfile );
			if( inSequence[0] == outSequence[0] ) {
				throw std::runtime_error( "Input file and output file must be different." );
			}
			try {
				outSequence.sync_frame_set();
				gotOutSequence = true;
			} catch( std::exception & ) {
			}
			if( gotOutSequence ) {
				std::vector<int> allWholeFrameNumbers;
				outSequence.wholeframe_numbers( allWholeFrameNumbers );
				std::vector<int> wholeFrameNumbers;
				copy_if( allWholeFrameNumbers.begin(), allWholeFrameNumbers.end(), std::back_inserter( wholeFrameNumbers ), boost::bind( in_closed_interval, _1, state.firstFrame, state.lastFrame ) );
				if( wholeFrameNumbers.size() ) {
					std::string msg = "The following output files already exist:\n";
					const std::size_t displayCount = 5;
					for( std::size_t i = 0; i < wholeFrameNumbers.size() && i < displayCount; ++i ) {
						msg += outSequence[wholeFrameNumbers[i]] + "\n";
					}
					if( displayCount < wholeFrameNumbers.size() ) {
						msg += "Plus " + boost::lexical_cast<std::string>( wholeFrameNumbers.size() - displayCount ) + " others.";
					}
					msg += "\nDo you want to replace them?";
					int result = MessageBox( hwnd, msg.c_str(), "Confirm Save As", MB_YESNO | MB_ICONEXCLAMATION );
					if( result != IDYES && result != IDNO ) {
						return false;
					}
					replaceOutputFiles = ( result == IDYES );
				}
			}
		} else {
			throw std::runtime_error( "Internal Error: Unrecognized frame range mode: " + boost::lexical_cast<std::string>( state.frameRangeMode ) );
		}
		return true;
	} catch( std::exception & e ) {
		const std::string msg = std::string( e.what() );
		MessageBox( hwnd, msg.c_str(), "Error", MB_OK | MB_ICONEXCLAMATION );
	}
	return false;

	// exception if:
	// m_pattern.get_directory( false ).length() == 0
	// ! files::directory_exists( m_pattern.get_directory( false ) )
}

DWORD WINAPI convert_particle_files( LPVOID inState ) {
	convert_particle_files_threadproc_params * state = reinterpret_cast<convert_particle_files_threadproc_params*>( inState );
	// these may be modified by proceed()
	bool ignoreMissingInputFiles = state->ignoreMissingInputFiles;
	bool replaceOutputFiles = state->replaceOutputFiles;
	DWORD returnResult = 2;

	try {
		if( state->frameRangeMode == FRAME_RANGE_MODE::SINGLE ) {
			state->progress->set_total_files( 1 );
			state->progress->set_current_file( 0 );
			if( ! replaceOutputFiles && frantic::files::file_exists( state->outfile ) ) {
				throw std::runtime_error( state->outfile + " already exists." );
			}
			convert_file( state->infile, state->outfile, state->progress );
		} else if( state->frameRangeMode == FRAME_RANGE_MODE::RANGE ) {
			if( state->firstFrame > state->lastFrame ) {
				throw std::runtime_error( "First frame number in range cannot be greater than last frame number in range." );
			}
			frantic::files::filename_sequence inSequence( state->infile );
			frantic::files::filename_sequence outSequence( state->outfile );
			state->progress->set_total_files( state->lastFrame - state->firstFrame + 1 );
			for( int frameNumber = state->firstFrame; frameNumber <= state->lastFrame; ++frameNumber ) {
				const std::string infile = inSequence[frameNumber];
				const std::string outfile = outSequence[frameNumber];
				state->progress->set_current_file( frameNumber - state->firstFrame );
				if( frantic::files::file_exists( infile ) ) {
					if( ! replaceOutputFiles && frantic::files::file_exists( outfile ) ) {
						throw std::runtime_error( outfile + " already exists." );
					}
					convert_file( infile, outfile, state->progress );
				} else {
					if( ! ignoreMissingInputFiles ) {
						throw std::runtime_error( "Input file \'" + infile + "\' could not be found." );
					}
				}
				if( state->progress->is_cancelled() ) {
					break;
				}
			}
		} else {
			throw std::runtime_error( "Internal Error: Unrecognized frame range mode: " + boost::lexical_cast<std::string>( state->frameRangeMode ) );
		}
		returnResult = 0;
	} catch( std::exception & e ) {
		state->errorMessage = e.what();
	}
	if( state->progress->is_cancelled() ) {
		returnResult = 1;
	}
	state->progress->set_done( returnResult );
	return returnResult;
}
#endif
// return output file or empty string
frantic::tstring show_convert_particle_files_dialog( const frantic::tstring& inputFile ) {
    convert_particle_files_dlg_proc state;
    state.infile = inputFile;
    if( !state.infile.empty() ) {
        state.lockInfile = true;
        state.notification = _T("You must convert this file to PRT format before opening.");
        state.outfile = frantic::files::replace_extension( state.infile, _T(".prt") );
    }
    INT_PTR result =
        DialogBoxParam( ghInstance, MAKEINTRESOURCE( IDD_CONVERT_PARTICLE_FILES ), GetCOREInterface()->GetMAXHWnd(),
                        &convert_particle_files_dlg_proc::dlg_proc, (LPARAM)( &state ) );
    if( result == 0 ) {
        return state.outfile;
    } else {
        return _T("");
    }
}

void convert_particle_files_dlg_proc::update_infile_display( convert_particle_files_dlg_proc* pState, HWND hwndDlg ) {
    if( pState && hwndDlg ) {
        frantic::max3d::SetCustEditText( GetDlgItem( hwndDlg, IDC_INPUT_EDIT ), pState->infile );
    }
}
void convert_particle_files_dlg_proc::update_outfile_display( convert_particle_files_dlg_proc* pState, HWND hwndDlg ) {
    if( pState && hwndDlg ) {
        frantic::max3d::SetCustEditText( GetDlgItem( hwndDlg, IDC_OUTPUT_EDIT ), pState->outfile );
    }
}
void convert_particle_files_dlg_proc::update_frame_range_mode_display( convert_particle_files_dlg_proc* pState,
                                                                       HWND hwndDlg ) {
    if( pState && hwndDlg ) {
        bool enable = true;
        if( pState->frameRangeMode == 0 ) {
            Button_SetCheck( GetDlgItem( hwndDlg, IDC_SINGLE_FILE ), BST_CHECKED );
            Button_SetCheck( GetDlgItem( hwndDlg, IDC_FRAME_RANGE ), BST_UNCHECKED );
            enable = false;
        } else {
            pState->frameRangeMode = 1;
            Button_SetCheck( GetDlgItem( hwndDlg, IDC_SINGLE_FILE ), BST_UNCHECKED );
            Button_SetCheck( GetDlgItem( hwndDlg, IDC_FRAME_RANGE ), BST_CHECKED );
            enable = true;
        }
        EnableWindow( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_START ), enable );
        EnableWindow( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_START_SPIN ), enable );
        EnableWindow( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_TO_STATIC ), enable );
        EnableWindow( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_END ), enable );
        EnableWindow( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_END_SPIN ), enable );
    }
}
void convert_particle_files_dlg_proc::update_frame_range_display( convert_particle_files_dlg_proc* pState,
                                                                  HWND hwndDlg ) {
    if( pState && hwndDlg ) {
        frantic::max3d::SetSpinnerValue( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_START_SPIN ), pState->firstFrame );
        frantic::max3d::SetSpinnerValue( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_END_SPIN ), pState->lastFrame );
        frantic::max3d::SetCustEditText( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_START ),
                                         boost::lexical_cast<frantic::tstring>( pState->firstFrame ).c_str() );
        frantic::max3d::SetCustEditText( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_END ),
                                         boost::lexical_cast<frantic::tstring>( pState->lastFrame ).c_str() );
    }
}
void convert_particle_files_dlg_proc::update_scale_color_byte_to_float_display( convert_particle_files_dlg_proc* pState,
                                                                                HWND hwndDlg ) {
    if( pState && hwndDlg ) {
        CheckDlgButton( hwndDlg, IDC_ENABLE_SCALE_COLOR_BYTE_TO_FLOAT, pState->scaleColorByteToFloat );
    }
}

void convert_particle_files_dlg_proc::update_frame_range_from_infile( convert_particle_files_dlg_proc* pState,
                                                                      HWND hwndDlg, bool showError ) {
    if( pState && hwndDlg ) {
        try {
            const bool fileExists = frantic::files::file_exists( pState->infile );
            bool sequenceExists = false;
            frantic::files::filename_sequence filenameSequence;
            try {
                filenameSequence = frantic::files::filename_sequence( pState->infile );
                filenameSequence.sync_frame_set();
                if( !filenameSequence.get_frame_set().empty() ) {
                    sequenceExists = true;
                }
            } catch( std::exception& ) {
                // pass
            }
            // A. set to single file
            // B. set to range
            // C. error dialog
            if( sequenceExists ) {
                const std::pair<int, int> range = filenameSequence.get_frame_set().wholeframe_range();
                pState->frameRangeMode = 1;
                pState->firstFrame = range.first;
                pState->lastFrame = range.second;
            } else if( fileExists ) {
                pState->frameRangeMode = 0;
            } else {
                if( showError ) {
                    MessageBox(
                        hwndDlg,
                        ( _T("Cannot determine range from missing file: \"") + pState->infile + _T("\"") ).c_str(),
                        _T("Frost"), MB_OK | MB_ICONEXCLAMATION );
                }
            }
            update_frame_range_mode_display( pState, hwndDlg );
            update_frame_range_display( pState, hwndDlg );
        } catch( std::exception& e ) {
            if( showError ) {
                const std::string& msg = std::string( "Error attempting to determine range:\n\n" ) + e.what();
                MessageBoxA( hwndDlg, msg.c_str(), "Frost", MB_OK | MB_ICONEXCLAMATION );
            }
        }
    }
}
void convert_particle_files_dlg_proc::set_infile( const frantic::tstring& infile, bool updateOutputFile,
                                                  HWND hwndDlg ) {
    this->infile = infile;
    update_infile_display( this, hwndDlg );
    update_frame_range_from_infile( this, hwndDlg );
    update_frame_range_display( this, hwndDlg );
    if( updateOutputFile || this->outfile.empty() ) {
        const frantic::tstring outfile = frantic::files::replace_extension( infile, _T(".prt") );
        this->outfile = outfile;
        update_outfile_display( this, hwndDlg );
    }
}

convert_particle_files_dlg_proc::convert_particle_files_dlg_proc()
    : frameRangeMode( 0 )
    , firstFrame( 0 )
    , lastFrame( 100 )
    , ignoreMissingInputFiles( false )
    , replaceOutputFiles( false )
    , autoUpdateOutputFile( true )
    , lockInfile( false )
    , scaleColorByteToFloat( false ) {}

int GetSpinnerIntValue( HWND hwndSpinner ) {
    int value = 0;
    if( hwndSpinner ) {
        ISpinnerControl* spinner = GetISpinner( hwndSpinner );
        if( spinner ) {
            value = spinner->GetIVal();
            ReleaseISpinner( spinner );
        }
    }
    return value;
}

INT_PTR CALLBACK convert_particle_files_dlg_proc::dlg_proc( __in HWND hwndDlg, __in UINT uMsg, __in WPARAM wParam,
                                                            __in LPARAM lParam ) {
    try {
        convert_particle_files_dlg_proc* pState =
            reinterpret_cast<convert_particle_files_dlg_proc*>( GetWindowLongPtr( hwndDlg, DWLP_USER ) );
        switch( uMsg ) {
        case WM_INITDIALOG:
            pState = reinterpret_cast<convert_particle_files_dlg_proc*>( lParam );
            SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)pState );
            {
                update_infile_display( pState, hwndDlg );
                update_outfile_display( pState, hwndDlg );
                ISpinnerControl* spinner = GetISpinner( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_START_SPIN ) );
                if( spinner ) {
                    spinner->LinkToEdit( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_START ), EDITTYPE_INT );
                    spinner->SetLimits( -100000, 100000 );
                    ReleaseISpinner( spinner );
                }
                spinner = GetISpinner( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_END_SPIN ) );
                if( spinner ) {
                    spinner->LinkToEdit( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_END ), EDITTYPE_INT );
                    spinner->SetLimits( -100000, 100000 );
                    ReleaseISpinner( spinner );
                }
                update_frame_range_from_infile( pState, hwndDlg, false );
                update_frame_range_display( pState, hwndDlg );
                update_frame_range_mode_display( pState, hwndDlg );
                update_scale_color_byte_to_float_display( pState, hwndDlg );
                if( !pState->notification.empty() ) {
                    SetWindowText( GetDlgItem( hwndDlg, IDC_NOTIFICATION ), pState->notification.c_str() );
                }
                frantic::max3d::EnableCustEdit( GetDlgItem( hwndDlg, IDC_INPUT_EDIT ), !pState->lockInfile );
                frantic::max3d::EnableCustButton( GetDlgItem( hwndDlg, IDC_INPUT_SELECT ), !pState->lockInfile );
            }
            return TRUE;
        case WM_DESTROY:
            SetWindowLongPtr( hwndDlg, DWLP_USER, 0 );
            break;
        case WM_CUSTEDIT_ENTER: {
            int id = LOWORD( wParam );
            switch( id ) {
            case IDC_INPUT_EDIT:
                if( pState ) {
                    const frantic::tstring infile =
                        frantic::max3d::GetCustEditText( GetDlgItem( hwndDlg, IDC_INPUT_EDIT ) );
                    if( infile != pState->infile ) {
                        pState->set_infile( infile, pState->autoUpdateOutputFile, hwndDlg );
                    }
                }
                break;
            case IDC_OUTPUT_EDIT:
                if( pState ) {
                    const frantic::tstring outfile =
                        frantic::max3d::GetCustEditText( GetDlgItem( hwndDlg, IDC_OUTPUT_EDIT ) );
                    if( outfile != pState->outfile ) {
                        pState->outfile = outfile;
                        pState->autoUpdateOutputFile = false;
                    }
                }
                break;
            }
        } break;
        case CC_SPINNER_CHANGE:
        case CC_SPINNER_BUTTONDOWN:
        case CC_SPINNER_BUTTONUP: {
            const int id = LOWORD( wParam );
            switch( id ) {
            case IDC_FRAME_RANGE_START_SPIN:
                if( pState ) {
                    pState->firstFrame = GetSpinnerIntValue( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_START_SPIN ) );
                    frantic::max3d::SetCustEditText(
                        GetDlgItem( hwndDlg, IDC_FRAME_RANGE_START ),
                        boost::lexical_cast<frantic::tstring>( pState->firstFrame ).c_str() );
                }
                break;
            case IDC_FRAME_RANGE_END_SPIN:
                if( pState ) {
                    pState->lastFrame = GetSpinnerIntValue( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_END_SPIN ) );
                    frantic::max3d::SetCustEditText(
                        GetDlgItem( hwndDlg, IDC_FRAME_RANGE_END ),
                        boost::lexical_cast<frantic::tstring>( pState->lastFrame ).c_str() );
                }
                break;
            }
        } break;
        case WM_COMMAND: {
            const int id = LOWORD( wParam );

            typedef std::pair<frantic::tstring, frantic::tstring> string_pair;
            std::vector<string_pair> outputFilterStrings;
            outputFilterStrings.push_back(
                std::pair<frantic::tstring, frantic::tstring>( _T("Thinkbox PRT Files (*.prt)"), _T("*.prt") ) );
            std::vector<string_pair> inputFilterStrings;
            inputFilterStrings.push_back( string_pair( _T("All Particle Files"), _T("*.csv;*.pts;*.ptx;*.xyz") ) );
            inputFilterStrings.push_back( string_pair( _T("CSV Files (*.csv)"), _T("*.csv") ) );
            inputFilterStrings.push_back( string_pair( _T("PTS Files (*.pts)"), _T("*.pts") ) );
            inputFilterStrings.push_back( string_pair( _T("PTX Files (*.ptx)"), _T("*.ptx") ) );
            inputFilterStrings.push_back( string_pair( _T("XYZ Files (*.xyz)"), _T("*.xyz") ) );
            inputFilterStrings.push_back( string_pair( _T("All Files (*.*)"), _T("*.*") ) );

            switch( id ) {
            case IDC_SINGLE_FILE:
                if( pState ) {
                    pState->frameRangeMode = 0;
                }
                update_frame_range_mode_display( pState, hwndDlg );
                break;
            case IDC_FRAME_RANGE:
                if( pState ) {
                    pState->frameRangeMode = 1;
                }
                update_frame_range_mode_display( pState, hwndDlg );
                break;
            case IDC_ENABLE_SCALE_COLOR_BYTE_TO_FLOAT:
                if( pState ) {
                    pState->scaleColorByteToFloat = IsDlgButtonChecked( hwndDlg, id ) == BST_CHECKED;
                }
                update_scale_color_byte_to_float_display( pState, hwndDlg );
                break;
            case IDOK:
                // do the conversion ?
                if( pState ) {
                    convert_particle_files_params params;
                    params.infile = pState->infile;
                    params.outfile = pState->outfile;
                    params.frameRangeMode = pState->frameRangeMode;
                    params.firstFrame = pState->firstFrame;
                    params.lastFrame = pState->lastFrame;
                    params.ignoreMissingInputFiles = false;
                    params.replaceOutputFiles = true;
                    params.colorScale = pState->scaleColorByteToFloat ? ( 1.f / 255 ) : 1.f;

                    if( accept_convert_particle_files_params( GetCOREInterface()->GetMAXHWnd(), params ) ) {
                        try {
                            bool done = convert_particle_files( params );
                            if( done ) {
                                EndDialog( hwndDlg, 0 );
                            }
                        } catch( std::exception& e ) {
                            const std::string errMsg = e.what();
                            MessageBoxA( hwndDlg, errMsg.c_str(), "Frost Error", MB_OK | MB_ICONEXCLAMATION );
                        }
                    }
                } else {
                    const std::string errMsg = "Frost Internal Error: State is NULL";
                    MessageBoxA( hwndDlg, errMsg.c_str(), "Frost", MB_OK | MB_ICONEXCLAMATION );
                }
                return TRUE;
            case IDCANCEL:
                EndDialog( hwndDlg, 1 );
                return TRUE;
            case IDC_INPUT_SELECT:
                if( pState ) {
                    const frantic::tstring infile = frantic::win32::ffLoadDialog( hwndDlg, _T("Select File to Convert"),
                                                                                  pState->infile, inputFilterStrings );
                    if( !infile.empty() ) {
                        pState->set_infile( infile, true, hwndDlg );
                    }
                }
                break;
            case IDC_OUTPUT_SELECT:
                if( pState ) {
                    const frantic::tstring outfile =
                        frantic::win32::ffSaveDialog( hwndDlg, _T("Select Save Location for PRT Files"),
                                                      pState->outfile, _T("prt"), outputFilterStrings );
                    if( !outfile.empty() ) { // && outfile != pState->outfile
                        pState->outfile = outfile;
                        update_outfile_display( pState, hwndDlg );
                        pState->autoUpdateOutputFile = false;
                    }
                }
                break;
            case IDC_SET_RANGE_FROM_FILES:
                update_frame_range_from_infile( pState, hwndDlg, true );
                update_frame_range_display( pState, hwndDlg );
                break;
            case IDC_DETECT_SCALE_COLOR_BYTE_TO_FLOAT:
                if( pState ) {
                    convert_particle_files_params params;
                    params.infile = pState->infile;
                    params.outfile = _T("");
                    params.frameRangeMode = pState->frameRangeMode;
                    params.firstFrame = pState->firstFrame;
                    params.lastFrame = pState->lastFrame;
                    params.ignoreMissingInputFiles = true;
                    params.replaceOutputFiles = false;

                    boost::logic::tribool result;
                    frantic::tstring indeterminateReason;
                    try {
                        bool done = detect_particle_files_color_scale( params, result, indeterminateReason );
                        if( done ) {
                            if( result ) {
                                pState->scaleColorByteToFloat = true;
                            } else if( !result ) {
                                pState->scaleColorByteToFloat = false;
                            } else {
                                const frantic::tstring msg = _T("Unable to detect.\n\n") + indeterminateReason;
                                MessageBox( hwndDlg, msg.c_str(), _T("Frost"), MB_OK | MB_ICONEXCLAMATION );
                            }
                            update_scale_color_byte_to_float_display( pState, hwndDlg );
                        }
                    } catch( std::exception& e ) {
                        const std::string errMsg = e.what();
                        MessageBoxA( hwndDlg, errMsg.c_str(), "Frost Error", MB_OK | MB_ICONEXCLAMATION );
                    }
                } else {
                    const std::string errMsg = "Frost Internal Error: State is NULL";
                    MessageBoxA( hwndDlg, errMsg.c_str(), "Frost", MB_OK | MB_ICONEXCLAMATION );
                }
                return TRUE;
                break;
                /*
        case IDC_FRAME_RANGE_START:
                switch( notifyCode ) {
                        case EN_UPDATE:
                                if( pState ) {
                                        BOOL success;
                                        UINT result = GetDlgItemInt( hwndDlg, IDC_FRAME_RANGE_START, &success, TRUE );
                                        if( success ) {
                                                pState->firstFrame = result;
                                        }
                                        frantic::max3d::SetSpinnerValue( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_START_SPIN
        ), pState->firstFrame );
                                }
                                break;
                }
                break;
        case IDC_FRAME_RANGE_END:
                switch( notifyCode ) {
                        case EN_UPDATE:
                                if( pState ) {
                                        BOOL success;
                                        UINT result = GetDlgItemInt( hwndDlg, IDC_FRAME_RANGE_END, &success, TRUE );
                                        if( success ) {
                                                pState->lastFrame = result;
                                        }
                                        frantic::max3d::SetSpinnerValue( GetDlgItem( hwndDlg, IDC_FRAME_RANGE_END_SPIN
        ), pState->lastFrame );
                                }
                                break;
                }
                break;
                */
            }
        } break;
        }
    } catch( std::exception& e ) {
        MessageBoxA( hwndDlg, ( std::string( "Error: " ) + e.what() ).c_str(), "Frost", MB_OK | MB_ICONEXCLAMATION );
    }
    return FALSE;
}
