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

#include <muse/chain/history_object.hpp>

namespace muse { namespace custom_tags {

namespace detail {

class custom_tags_impl {
public:
   typedef void result_type;

   explicit custom_tags_impl( custom_tags_plugin& _plugin )
      : _self( _plugin ) {}

   ~custom_tags_impl() {}

   muse::chain::database& database() { return _self.database(); }

   void on_operation( const muse::chain::operation_object& op_obj );

   std::set<std::string> get_accounts( const fc::variant_object& json, const char* key );
   void set_labels( const std::string& tagger, const std::string& label, std::set<std::string>& names );
   void add_labels( const std::string& tagger, const std::string& label, const std::set<std::string>& names );
   void remove_labels( const std::string& tagger, const std::string& label, const std::set<std::string>& names );

   template<typename Op>
   void operator()( Op&& )const{ /* ignore most operations */ }

   void operator()( const muse::chain::custom_json_operation& op );

   custom_tags_plugin&                                _self;
   graphene::db::primary_index< custom_tags_index >*  cti;
};

std::set<std::string> custom_tags_impl::get_accounts( const fc::variant_object& json, const char* key )
{
   std::set<std::string> result;

   auto itr = json.find( key );
   if( itr == json.end() ) return result;

   FC_ASSERT( itr->value().is_array(), "${key} is not an array - ignoring op", ("key",std::string(key)) );

   for( const fc::variant& name : itr->value().get_array() )
   {
      database().get_account( name.get_string() ); // ensure it exists
      result.insert( name.get_string() );
   }

   return result;
}

void custom_tags_impl::set_labels( const std::string& tagger, const std::string& label, std::set<std::string>& names )
{
   auto& idx = cti->indices().get<by_tagger>();
   auto itr = idx.lower_bound( boost::make_tuple( tagger, label, "" ) );
   while( itr != idx.end() && itr->tagger == tagger && itr->tag == label )
   {
      if( names.find( itr->taggee ) != names.end() )
      {
         auto& to_erase = *itr;
         itr++;
         cti->remove( to_erase );
      }
      else
      {
         names.erase( itr->taggee );
         itr++;
      }
   }
   for( const std::string& name : names )
      database().create<tag_object>( [&tagger,&label,&name]( tag_object& new_tag ) {
         new_tag.tagger = tagger;
         new_tag.tag = label;
         new_tag.taggee = name;
      });
}

void custom_tags_impl::add_labels( const std::string& tagger, const std::string& label, const std::set<std::string>& names )
{
   auto& idx = cti->indices().get<by_tagger>();
   for( const std::string& name : names )
   {
      auto itr = idx.find( boost::make_tuple( tagger, label, name ) );
      if( itr == idx.end() )
         database().create<tag_object>( [&tagger,&label,&name]( tag_object& new_tag ) {
            new_tag.tagger = tagger;
            new_tag.tag = label;
            new_tag.taggee = name;
         });
   }
}

void custom_tags_impl::remove_labels( const std::string& tagger, const std::string& label, const std::set<std::string>& names )
{
   auto& idx = cti->indices().get<by_tagger>();
   for( const std::string& name : names )
   {
      auto itr = idx.find( boost::make_tuple( tagger, label, name ) );
      if( itr != idx.end() ) cti->remove( *itr );
   }
}

void custom_tags_impl::operator()( const muse::chain::custom_json_operation& op )
{ try {
   const std::string* tagger;
   if( op.required_auths.size() == 1 && op.required_basic_auths.empty() )
      tagger = &(*op.required_auths.begin());
   else if( op.required_auths.empty() && op.required_basic_auths.size() == 1 )
      tagger = &(*op.required_basic_auths.begin());
   else
   {
      ilog( "Ignoring custom_json_operation with more than one authority" );
      return;
   }

   const fc::variant json_data = fc::json::from_string( op.json );
   if( !json_data.is_object() )
   {
      ilog( "Ignoring custom_json_operation that contains no json object" );
      return;
   }

   auto itr = json_data.get_object().find( "label" );
   if( itr == json_data.get_object().end() || !itr->value().is_string() )
   {
      ilog( "Ignoring custom_json_operation with no string label" );
      return;
   }
   const std::string& label = itr->value().get_string();

   const std::set<std::string> to_add = get_accounts( json_data.get_object(), "add" );
   const std::set<std::string> to_remove = get_accounts( json_data.get_object(), "remove" );
   std::set<std::string> to_set = get_accounts( json_data.get_object(), "set" );

   FC_ASSERT( to_set.empty() || ( to_add.empty() && to_remove.empty() ), "custom_json should use either set or add/remove!" );

   if( !to_set.empty() )
      set_labels( *tagger, label, to_set );
   else
   {
      add_labels( *tagger, label, to_add );
      remove_labels( *tagger, label, to_remove );
   }
} FC_CAPTURE_AND_LOG( (op) ) }

void custom_tags_impl::on_operation( const muse::chain::operation_object& op_obj ) {
   op_obj.op.visit( *this );
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
   my->cti = database().add_index< graphene::db::primary_index< custom_tags_index > >();

   ilog("custom_tags plugin: plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void custom_tags_plugin::plugin_startup() {
   app().register_api_factory< custom_tags_api >( "custom_tags_api" );
}

void custom_tags_plugin::plugin_shutdown() { /* Must override pure virtual method */ }

} } // namespace muse::custom_tags

MUSE_DEFINE_PLUGIN( custom_tags, muse::custom_tags::custom_tags_plugin )
