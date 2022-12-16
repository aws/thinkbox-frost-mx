// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/particles/streams/apply_function_particle_istream.hpp>

namespace frantic {
namespace particles {
namespace streams {

/**
 * This class allows an arbitrary function to be applied to every particle in a stream
 * by taking an output channel and boost::array of input channels. The supplied channels must
 * must match the signature of the function. It will also handle modifying its channel map to request
 * additional channels that are not in its output.
 */
template <class F>
class apply_function_particle_istream_nothreads : public delegated_particle_istream {
  private:
    typedef typename ParticleDispatcher<F>::result_type destination_type;
    typedef typename ParticleDispatcher<F>::function_type function_type;
    typedef typename ParticleDispatcher<F>::args_type args_type;
    enum { NUM_ARGS = ParticleDispatcher<F>::NUM_ARGS };

  private:
    frantic::tstring m_destChannel;
    frantic::channels::channel_cvt_accessor<destination_type> m_destAccessor;

    args_type m_srcAccessors;
    function_type m_function;
    boost::array<frantic::tstring, NUM_ARGS> m_srcChannels;

    // When an argument references a channel not specified in the user-supplied channel
    // map, it will request a modified channel map from the delegate with the extra channels.
    channels::channel_map m_outMap, m_nativeMap;
    channels::channel_map_adaptor m_adaptor;
    boost::scoped_array<char> m_tempBuffer;

  private:
    /**
     * This function will traverse the typelist of the arguments to the template function F
     * For each arg, it will get the relevant cvt_accessor from the supplied map and store it in
     * m_srcAccessors[Index]. The template allows for a compile-time loop over the arguments.
     * For example: If F == void(int,float), NUM_ARGS will be 2, so calling init_arg_accessors<0>()
     *              will loop over elements 0 and 1 of the typelist.
     */
    template <int Index>
    void init_arg_accessors( const channels::channel_map& pcm, const frantic::tstring* channelArray ) {
        typedef boost::tuples::element<Index, args_type>::type::value_type T;

        if( !pcm.has_channel( channelArray[Index] ) )
            m_srcAccessors.get<Index>().reset( T() );
        else
            m_srcAccessors.get<Index>() = pcm.get_const_cvt_accessor<T>( channelArray[Index] );
        init_arg_accessors<Index + 1>( pcm, channelArray );
    }

    /**
     * Template overload, stops the iteration once all argument accessors have been collected
     */
    template <>
    void init_arg_accessors<NUM_ARGS>( const channels::channel_map&, const frantic::tstring* ) {}

    /**
     * This function will traverse the typelist of the arguments to the template function F
     * For each arg, it will ensure the supplied map has a channel with the given name, otherwise
     * it will create a new channel with the relevant type.
     */
    template <int Index>
    void ensure_arg_channels( channels::channel_map& pcm, const channels::channel_map& nativeMap,
                              const frantic::tstring* channelArray ) {
        typedef boost::tuples::element<Index, args_type>::type::value_type T;

        if( !pcm.has_channel( channelArray[Index] ) && nativeMap.has_channel( channelArray[Index] ) )
            pcm.append_channel<T>( channelArray[Index] );
        ensure_arg_channels<Index + 1>( pcm, nativeMap, channelArray );
    }

    /**
     * Template overload, stops the iteration once all argument channels have been ensured
     */
    template <>
    void ensure_arg_channels<NUM_ARGS>( channels::channel_map& /*pcm*/, const channels::channel_map& /*nativeMap*/,
                                        const frantic::tstring* /*channelArray*/ ) {}

  public:
    apply_function_particle_istream_nothreads( boost::shared_ptr<particle_istream> pin, const function_type& function,
                                               const frantic::tstring& destChannel,
                                               const boost::array<frantic::tstring, NUM_ARGS>& srcChannels )
        : delegated_particle_istream( pin )
        , m_function( function )
        , m_destChannel( destChannel )
        , m_srcChannels( srcChannels ) {
        set_channel_map( m_delegate->get_channel_map() );

        // Add the output of this operation to the native channel map if it isn't already there
        m_nativeMap = m_delegate->get_native_channel_map();
        if( !m_nativeMap.has_channel( m_destChannel ) )
            m_nativeMap.append_channel<destination_type>( m_destChannel );
    }

    virtual ~apply_function_particle_istream_nothreads() {}

    void set_channel_map( const channel_map& pcm ) {
        channels::channel_map requested = m_outMap = pcm;
        const channels::channel_map& nativeMap = m_delegate->get_native_channel_map();

        // Append any missing channels to the delegate's channel map
        if( !requested.has_channel( m_destChannel ) )
            requested.append_channel<destination_type>( m_destChannel );
        ensure_arg_channels<0>( requested, nativeMap, m_srcChannels.data() );

        // Initialize the accessors.
        m_destAccessor = requested.get_cvt_accessor<destination_type>( m_destChannel );
        init_arg_accessors<0>( requested, m_srcChannels.data() );

        m_delegate->set_channel_map( requested );
        m_adaptor.set( m_outMap, requested );
        m_tempBuffer.reset( m_adaptor.is_identity() ? NULL : new char[requested.structure_size()] );
    }

    std::size_t particle_size() const { return m_outMap.structure_size(); }

    const channels::channel_map& get_channel_map() const { return m_outMap; }
    const channels::channel_map& get_native_channel_map() const { return m_nativeMap; }

    void set_default_particle( char* buffer ) {
        if( m_adaptor.is_identity() )
            m_delegate->set_default_particle( buffer );
        else {
            channels::channel_map_adaptor tempAdaptor( m_delegate->get_channel_map(), m_outMap );

            boost::scoped_ptr<char> pDefault( new char[tempAdaptor.dest_size()] );
            memset( pDefault.get(), 0, tempAdaptor.dest_size() );
            tempAdaptor.copy_structure( pDefault.get(), buffer );

            m_delegate->set_default_particle( pDefault.get() );
        }
    }

    bool get_particle( char* outBuffer ) {
        char* inBuffer = ( m_adaptor.is_identity() ) ? outBuffer : m_tempBuffer.get();

        if( !m_delegate->get_particle( inBuffer ) )
            return false;

        // It is imperative that the function be dispatched on the input buffer, because there are
        // no guarantees that the required channels exist in the output buffer
        ParticleDispatcher<F>::dispatch( m_function, m_destAccessor, m_srcAccessors, inBuffer );

        if( inBuffer != outBuffer )
            m_adaptor.copy_structure( outBuffer, inBuffer );

        return true;
    }

    bool get_particles( char* outBuffer, std::size_t& numParticles ) {
        boost::scoped_array<char> tempBuffer;
        char* target;

        if( !m_adaptor.is_identity() ) {
            tempBuffer.reset( new char[numParticles * m_adaptor.source_size()] );
            target = tempBuffer.get();
        } else
            target = outBuffer;

        bool notEos = m_delegate->get_particles( target, numParticles );

        apply_function_impl<F> f( m_destAccessor, m_srcAccessors, m_function, m_adaptor, outBuffer );
        f( particle_range( 0, numParticles, target, m_adaptor.source_size(), 1000 ) );

        return notEos;
    }
};

} // namespace streams
} // namespace particles
} // namespace frantic
