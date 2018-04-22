/*
 * Copyright (c) 2018 PeerTracks, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <muse/custom_tags/custom_tags_api.hpp>

namespace muse { namespace custom_tags {

namespace detail {

class custom_tags_impl {
public:
   custom_tags_impl( custom_tags_plugin& _plugin )
      : _self( _plugin ) {}

   ~custom_tags_impl() {}

   muse::chain::database& database() { return _self.database(); }

   void on_operation( const muse::chain::operation_object& op_obj );

   custom_tags_plugin& _self;
   custom_tags_index*  cti;
};

void custom_tags_impl::on_operation( const muse::chain::operation_object& op_obj ) {
    // FIXME
}

} // detail

custom_tags_plugin::custom_tags_plugin()
   : my( new detail::custom_tags_impl(*this) ) {}

custom_tags_plugin::~custom_tags_plugin() {}

void custom_tags_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   command_line_options.add_options()
         ;
   config_file_options.add(command_line_options);
}

std::string custom_tags_plugin::plugin_name()const
{
   return "custom_tags";
}

void custom_tags_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   ilog("custom_tags plugin: plugin_initialize() begin");

   database().post_apply_operation.connect( [this]( const muse::chain::operation_object& op ){ my->on_operation(op); } );
   my->cti = database().add_index< primary_index< custom_tags_index > >();

   ilog("custom_tags plugin: plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void custom_tags_plugin::plugin_startup() {
   app().register_api_factory< custom_tags_api >( "custom_tags_api" );
}

void custom_tags_plugin::plugin_shutdown() { /* Must override pure virtual method */ }

} } // namespace muse::custom_tags

MUSE_DEFINE_PLUGIN( custom_tags, muse::custom_tags::custom_tags_plugin )
