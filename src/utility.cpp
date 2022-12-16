// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/math/constants/constants.hpp>

#include <frantic/graphics/cubeface.hpp>
#include <frantic/particles/streams/apply_function_particle_istream.hpp>

#include <frantic/max3d/particles/streams/max_legacy_particle_istream.hpp>
#include <frantic/max3d/particles/streams/seconds_to_ticks_particle_istream.hpp>
#include <frantic/max3d/particles/streams/ticks_to_seconds_particle_istream.hpp>
#include <frantic/max3d/particles/tp_interface.hpp>

#include "Frost.hpp"
#include "convert_byte_color_to_float_particle_istream.hpp"
#include "utility.hpp"

using namespace frantic::channels;
using namespace frantic::geometry;
using namespace frantic::graphics;

#define VRAY_RENDER_CLASS_ID Class_ID( 0x73bab286, 0x77f8fd0c )
#define FINALRENDER_RENDERER_CLASS_ID Class_ID( 0x0cf12ab7, 0x03762873 )

void extract_connected_particle_groups( INode* node, std::set<INode*>& groups ) {
    if( !node ) {
        throw std::runtime_error( "extract_connected_particle_groups: got a NULL INode." );
    }

    Object* nodeObj = node->GetObjectRef();
    // first, make sure this inode has something to do with particles.
    if( nodeObj && GetParticleObjectExtInterface( nodeObj ) ) {
        std::set<IPFActionList*> connectedActionLists;

        const int numSubs = nodeObj->NumSubs();
        for( int i = 0; i < numSubs; ++i ) {
            if( Animatable* subanim = nodeObj->SubAnim( i ) ) {
                IPFActionList* actionList =
                    dynamic_cast<IPFActionList*>( subanim->GetInterface( PFACTIONLIST_INTERFACE ) );
                if( actionList ) {
                    connectedActionLists.insert( actionList );
                }
            }
        }

        // try to get the groups from the node.
        std::vector<INode*> inodes;
        frantic::max3d::get_referring_inodes( inodes, node );

        std::vector<INode*>::iterator i = inodes.begin();
        for( ; i != inodes.end(); i++ ) {
            if( *i ) {
                Object* obj = ( *i )->GetObjectRef();
                if( obj ) {
                    if( IParticleGroup* pGroup = ParticleGroupInterface( obj ) ) {
                        INode* actionListNode = pGroup->GetActionList();
                        IPFActionList* actionList = GetPFActionListInterface( actionListNode->GetObjectRef() );
                        if( pGroup->GetParticleContainer() &&
                            connectedActionLists.find( actionList ) != connectedActionLists.end() ) {
                            groups.insert( *i );
                        }
                    }
                }
            }
        }
    }
}

bool is_node_particle_flow( INode* node ) {
    if( !node ) {
        return false;
    }
    Object* obj = node->GetObjectRef();
    if( obj ) {
        IParticleGroup* group = GetParticleGroupInterface( obj );
        if( group ) {
            IPFSystem* particleSystem = PFSystemInterface( group->GetParticleSystem() );
            if( particleSystem ) {
                return true;
            }
        }
    }
    return false;
}

void extract_tp_groups( INode* node, std::set<ReferenceTarget*>& outGroups ) {
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
    if( !node ) {
        throw std::runtime_error( "extract_tp_groups: got a NULL INode." );
    }

    if( frantic::max3d::particles::tp_interface::get_instance().is_node_thinking_particles( node ) ) {
        typedef std::pair<std::string, ReferenceTarget*> named_tp_group;
        std::vector<ReferenceTarget*> groups;
        frantic::max3d::particles::tp_interface::get_instance().get_groups( node, groups );
        BOOST_FOREACH( ReferenceTarget* group, groups ) {
            outGroups.insert( group );
        }
    }
#endif
}

void extract_particle_groups( INode* node, std::set<INode*>& groups ) {
    if( !node ) {
        throw std::runtime_error( "extract_particle_groups: got a NULL INode." );
    }

    Object* nodeObj = node->GetObjectRef();
    // first, make sure this inode has something to do with particles.
    if( nodeObj && GetParticleObjectExtInterface( nodeObj ) ) {
        // try to get the groups from the node.
        std::vector<INode*> inodes;
        frantic::max3d::get_referring_inodes( inodes, node );

        std::vector<INode*>::iterator i = inodes.begin();
        for( ; i != inodes.end(); i++ ) {
            if( *i ) {
                Object* obj = ( *i )->GetObjectRef();
                if( obj ) {
                    if( IParticleGroup* pGroup = ParticleGroupInterface( obj ) ) {
                        groups.insert( *i );
                    }
                }
            }
        }
    }
}

// logic taken from get_mesh_from_inode -- return false if get_mesh_from_inode() would throw an exception
bool can_get_mesh_from_inode( TimeValue t, INode* node ) {
    if( !node ) {
        return false;
    }

    ObjectState os = node->EvalWorldState( t );
    Object* obj = os.obj;

    if( obj == 0 ) {
        return false;
    }

    SClass_ID scid = obj->SuperClassID();

    // If the object is a derived object, follow its references to the real object
    // This is here because there were some biped objects not being saved when they should have been.
    while( scid == GEN_DERIVOB_CLASS_ID ) {
        obj = ( (IDerivedObject*)obj )->GetObjRef();
        if( obj == 0 )
            return false;
        else
            scid = obj->SuperClassID();
    }

    if( scid != SHAPE_CLASS_ID && scid != GEOMOBJECT_CLASS_ID ) {
        return false;
    }

    return true;
}

void conform_output_mesh_channel_types( frantic::geometry::trimesh3& mesh ) {
    if( mesh.has_face_channel( _T("MaterialID") ) ) {
        bool needsConversion = true;
        {
            frantic::geometry::trimesh3_face_channel_general_accessor generalAcc =
                mesh.get_face_channel_general_accessor( _T("MaterialID") );
            if( generalAcc.arity() != 1 ) {
                throw std::runtime_error( "Unexpected arity for MtlIndex.  Expected 1 but got " +
                                          boost::lexical_cast<std::string>( generalAcc.arity() ) + " instead." );
            }
            if( generalAcc.data_type() == frantic::channels::channel_data_type_traits<MtlID>::data_type() ) {
                needsConversion = false;
            }
        }
        if( needsConversion ) {
            std::vector<MtlID> newChannel( mesh.face_count() );
            {
                frantic::geometry::const_trimesh3_face_channel_cvt_accessor<boost::int64_t> acc =
                    ( (const frantic::geometry::trimesh3&)mesh )
                        .get_face_channel_cvt_accessor<boost::int64_t>( _T("MaterialID") );
                for( std::size_t i = 0; i < mesh.face_count(); ++i ) {
                    newChannel[i] = static_cast<MtlID>( acc.get( i ) );
                }
            }
            mesh.erase_face_channel( _T("MaterialID") );
            mesh.add_face_channel<MtlID>( _T("MaterialID") );
            frantic::geometry::trimesh3_face_channel_accessor<MtlID> acc =
                mesh.get_face_channel_accessor<MtlID>( _T("MaterialID") );
            for( std::size_t i = 0; i < mesh.face_count(); ++i ) {
                acc[i] = newChannel[i];
            }
        }
    }
}

void CustButton_SetRightClickNotify( HWND hwndButton, bool enable ) {
    if( hwndButton ) {
        ICustButton* custButton = GetICustButton( hwndButton );
        if( custButton ) {
            custButton->SetRightClickNotify( enable );
            ReleaseICustButton( custButton );
        }
    }
}

void CustButton_SetTooltip( HWND hwndButton, const TCHAR* tooltip, bool enable ) {
    if( hwndButton ) {
        ICustButton* custButton = GetICustButton( hwndButton );
        if( custButton ) {
            custButton->SetTooltip( enable, const_cast<TCHAR*>( tooltip ) );
            ReleaseICustButton( custButton );
        }
    }
}

void notify_if_missing_frost_mxs() {
    const bool found = !frantic::max3d::mxs::expression( _T("(FrostUI==undefined)") ).evaluate<bool>();
    if( !found ) {
        if( is_network_render_server() ) {
            throw std::runtime_error( "Missing file: FrostUI.ms" );
        } else {
            const TCHAR msg[] =
                _T("The Frost plugin is not installed correctly.\n\nMissing file: FrostUI.ms\n\nPlease reinstall.");
            const TCHAR title[] = _T("Frost");
            MessageBox( GetCOREInterface()->GetMAXHWnd(), msg, title, MB_OK | MB_ICONEXCLAMATION );
        }
    }
}

void get_list_box_selection_mxs( HWND hwndListBox, std::vector<int>& out ) {
    out.clear();
    if( hwndListBox ) {
        LRESULT selCount = SendMessage( hwndListBox, LB_GETSELCOUNT, 0, 0 );
        if( selCount != LB_ERR ) {
            std::vector<int> sel( selCount + 1 );
            LRESULT lRet = SendMessage( hwndListBox, LB_GETSELITEMS, selCount, LPARAM( &sel[0] ) );
            if( lRet != LB_ERR ) {
                sel.resize( lRet );
                for( std::size_t i = 0; i < sel.size(); ++i ) {
                    ++sel[i];
                }
                sel.swap( out );
            }
        }
    }
}

void set_list_box_selection_mxs( HWND hwndListBox, const std::vector<int>& selection ) {
    if( !hwndListBox ) {
        return;
    }
    const LRESULT itemCount = SendMessage( hwndListBox, LB_GETCOUNT, 0, 0 );
    if( itemCount == LB_ERR ) {
        return;
    }
    SendMessage( hwndListBox, LB_SETSEL, FALSE, -1 );
    BOOST_FOREACH( int selectedIndex1, selection ) {
        const int selectedIndex = selectedIndex1 - 1;
        if( selectedIndex >= 0 && selectedIndex < itemCount ) {
            SendMessage( hwndListBox, LB_SETSEL, TRUE, (LPARAM)selectedIndex );
        }
    }
}

frantic::graphics::camera<float> get_default_camera() {
    frantic::graphics::camera<float> camera;
    return camera;
}

frantic::graphics::camera<float> get_camera_from_view( View& view ) {
    frantic::graphics::camera<float> camera;

    camera.set_transform( frantic::max3d::from_max_t( Inverse( view.affineTM ) ) );
    camera.set_horizontal_fov( view.fov );
    camera.set_projection_mode( view.projType == 0 ? frantic::graphics::projection_mode::perspective
                                                   : frantic::graphics::projection_mode::orthographic );

    return camera;
}

boost::shared_ptr<frantic::particles::streams::particle_istream>
prepare_file_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin ) {
    // Some older files may store Age & LifeSpan as ticks so we want to convert those to float time. This is only
    // strictly necessary if there are multiple sequences with incompatible time types but I think it behooves us to
    // treat time channels uniformly.
    pin = frantic::max3d::particles::streams::convert_time_channels_to_seconds( pin );

    // If a stream was converted, it doesn't change the existing channel_map (since we only allow that to change as a
    // result of set_channel_map()), so I may need to modify the "current" channel_map to account for the change in data
    // type.
    frantic::channels::channel_map modifiedChannels;
    frantic::max3d::particles::streams::convert_time_channels_to_seconds( pin->get_channel_map(), modifiedChannels );

    pin->set_channel_map( modifiedChannels );

    if( pin->get_native_channel_map().has_channel( _T("Color") ) ) {
        frantic::channels::channel_general_accessor colorAcc =
            pin->get_native_channel_map().get_general_accessor( _T("Color") );
        if( colorAcc.arity() == 3 && colorAcc.data_type() == frantic::channels::data_type_uint8 ) {
            pin.reset( new convert_byte_color_to_float_particle_istream( pin ) );
        }
    }
    return pin;
}

inline float get_thinking_particles_radius_from_scale( const float scale ) { return 0.5f * scale; }

inline float get_thinking_particles_radius_from_scaleXYZ( const frantic::graphics::vector3f& scaleXYZ ) {
    return 0.5f * scaleXYZ.max_abs_component();
}

inline float get_thinking_particles_radius_from_scale_and_scaleXYZ( const float scale,
                                                                    const frantic::graphics::vector3f& scaleXYZ ) {
    return 0.5f * scale * scaleXYZ.max_abs_component();
}

boost::shared_ptr<frantic::particles::streams::particle_istream>
get_thinking_particles_radius_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin ) {
    const frantic::channels::channel_map& cm = pin->get_native_channel_map();

    if( cm.has_channel( _T("Radius") ) ) {
        return pin;
    }

    bool hasScale = false;
    bool hasScaleXYZ = false;

    if( cm.has_channel( _T("Scale") ) ) {
        frantic::channels::channel_general_accessor acc = cm.get_general_accessor( _T("Scale") );
        if( acc.arity() == 3 && frantic::channels::is_channel_data_type_float( acc.data_type() ) ) {
            hasScaleXYZ = true;
        }
    }

    if( cm.has_channel( _T("Size") ) ) {
        frantic::channels::channel_general_accessor acc = cm.get_general_accessor( _T("Size") );
        if( acc.arity() == 1 && frantic::channels::is_channel_data_type_float( acc.data_type() ) ) {
            hasScale = true;
        }
    }

    if( hasScale && hasScaleXYZ ) {
        static boost::array<frantic::tstring, 2> inputChannels = { _T("Size"), _T("Scale") };
        return boost::shared_ptr<frantic::particles::streams::particle_istream>(
            new frantic::particles::streams::apply_function_particle_istream<float(
                float, const frantic::graphics::vector3f& )>(
                pin, get_thinking_particles_radius_from_scale_and_scaleXYZ, _T("Radius"), inputChannels ) );
    } else if( hasScale ) {
        static boost::array<frantic::tstring, 1> inputChannels = { _T("Size") };
        return boost::shared_ptr<frantic::particles::streams::particle_istream>(
            new frantic::particles::streams::apply_function_particle_istream<float( float )>(
                pin, get_thinking_particles_radius_from_scale, _T("Radius"), inputChannels ) );
    } else if( hasScaleXYZ ) {
        static boost::array<frantic::tstring, 1> inputChannels = { _T("Scale") };
        return boost::shared_ptr<frantic::particles::streams::particle_istream>(
            new frantic::particles::streams::apply_function_particle_istream<float(
                const frantic::graphics::vector3f& )>( pin, get_thinking_particles_radius_from_scaleXYZ, _T("Radius"),
                                                       inputChannels ) );
    } else {
        return pin;
    }
}

boost::shared_ptr<frantic::particles::streams::particle_istream>
get_max_legacy_particle_istream( INode* node, TimeValue t, const frantic::channels::channel_map& channelMap ) {
    if( node ) {
        Object* obj = node->GetObjectRef();
        if( obj ) {
            Object* baseObj = obj->FindBaseObject();
            // RealFlow Particle Loader
            if( baseObj->ClassID() == Class_ID( 0x6DD2653D, 0x37F82C03 ) ) {
                return boost::shared_ptr<frantic::particles::streams::particle_istream>(
                    new frantic::max3d::particles::streams::max_legacy_particle_istream( node, t, channelMap, false ) );
            }
        }
    }
    return boost::shared_ptr<frantic::particles::streams::particle_istream>(
        new frantic::max3d::particles::streams::max_legacy_particle_istream( node, t, channelMap ) );
}

bool is_renderer( RENDERER::renderer_id_enum rendererID ) {
    Interface* coreInterface = GetCOREInterface();
    if( !coreInterface ) {
        return false;
    }
    Renderer* renderer = coreInterface->GetCurrentRenderer();
    if( !renderer ) {
        return false;
    }

    switch( rendererID ) {
    case RENDERER::VRAY:
        return ( renderer->ClassID() == VRAY_RENDER_CLASS_ID ) != 0;
    default:
        return false;
    }
}

bool is_render_active( bool defaultValue ) {
    bool isRenderActive = defaultValue;
    Interface7* interface7 = GetCOREInterface7();
    if( interface7 ) {
        isRenderActive = ( interface7->IsRenderActive() != 0 );
    }
    return isRenderActive;
}

void set_channel_map_to_native_channel_map( boost::shared_ptr<frantic::particles::streams::particle_istream> pin ) {
    if( pin ) {
        const frantic::channels::channel_map& nativeChannelMap = pin->get_native_channel_map();
        if( nativeChannelMap.has_channel( _T("Position") ) ) {
            if( nativeChannelMap[_T("Position")].data_type() == frantic::channels::data_type_float32 ) {
                pin->set_channel_map( nativeChannelMap );
            } else {
                std::size_t primitiveSizeSum = 0;
                for( std::size_t i = 0; i < nativeChannelMap.channel_count(); ++i ) {
                    primitiveSizeSum += nativeChannelMap[i].primitive_size();
                }
                const bool pad = primitiveSizeSum < nativeChannelMap.structure_size();

                frantic::channels::channel_map cm;
                for( std::size_t i = 0; i < nativeChannelMap.channel_count(); ++i ) {
                    const frantic::channels::channel& ch = nativeChannelMap[i];
                    if( ch.name() == _T("Position") ) {
                        cm.define_channel( ch.name(), ch.arity(), frantic::channels::data_type_float32 );
                    } else {
                        cm.define_channel( ch.name(), ch.arity(), ch.data_type() );
                    }
                }
                if( pad ) {
                    cm.end_channel_definition();
                } else {
                    cm.end_channel_definition( 1 );
                }
                pin->set_channel_map( cm );
            }
        } else {
            pin->set_channel_map( nativeChannelMap );
        }
    }
}
