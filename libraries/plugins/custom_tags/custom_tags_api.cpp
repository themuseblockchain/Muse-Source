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
#include <muse/app/api_context.hpp>
#include <muse/app/application.hpp>

#include <muse/custom_tags/custom_tags_api.hpp>

namespace muse { namespace custom_tags {

namespace detail {

class custom_tags_api_impl
{
public:
   custom_tags_api_impl( muse::app::application& _app ) : app( _app ) {}

   std::shared_ptr< muse::custom_tags::custom_tags_plugin > get_plugin()
   {
      return app.get_plugin< custom_tags_plugin >( "custom_tags" );
   }

   bool is_tagged( const std::string& user, const std::string& tagged_by, const std::string& tag ) const;
//   const fc::optional<std::set<std::string>>& list_checked_tags()const;
//   const fc::optional<std::set<std::string>>& list_checked_taggers()const;
   std::vector<half_tagging> list_tags_from( const std::string& tagger,
                                             const std::string& filter_tag,
                                             const std::string& start_tag,
                                             const std::string& start_taggee )const;
   std::vector<half_tagging> list_tags_on( const std::string& taggee,
                                           const std::string& filter_tag,
                                           const std::string& start_tag,
                                           const std::string& start_tagger )const;

private:
   muse::app::application& app;
};

bool custom_tags_api_impl::is_tagged( const std::string& user, const std::string& tagged_by, const std::string& tag ) const
{
   const auto& idx = app.chain_database()->get_index_type<custom_tags_index>().indices().get<by_tagger>();
   return idx.find( boost::make_tuple( tagged_by, tag, user ) ) != idx.end();
}

//static const fc::optional<std::set<std::string>> EMPTY;

//const fc::optional<std::set<std::string>>& custom_tags_api_impl::list_checked_tags()const
//{
//   return EMPTY; // FIXME
//}
//
//const fc::optional<std::set<std::string>>& custom_tags_api_impl::list_checked_taggers()const
//{
//   return EMPTY; // FIXME
//}

std::vector<half_tagging> custom_tags_api_impl::list_tags_from( const std::string& tagger,
                                                                const std::string& filter_tag,
                                                                const std::string& start_tag,
                                                                const std::string& start_taggee )const
{
   FC_ASSERT( filter_tag.empty() || start_tag.empty(), "Use either filter_tag or start_tag but not both!" );

   std::vector<half_tagging> result;
   result.reserve(100);
   const auto& idx = app.chain_database()->get_index_type<custom_tags_index>().indices().get<by_tagger>();
   auto itr = idx.lower_bound( boost::make_tuple( tagger, filter_tag.empty() ? start_tag : filter_tag, start_taggee ) );
   while( itr != idx.end() && itr->tagger == tagger && result.size() < 100 )
   {
       if( !filter_tag.empty() && itr->tag != filter_tag ) break;
       result.push_back( half_tagging( *itr++, false ) );
   }
   return result;
}

std::vector<half_tagging> custom_tags_api_impl::list_tags_on( const std::string& taggee,
                                                              const std::string& filter_tag,
                                                              const std::string& start_tag,
                                                              const std::string& start_tagger )const
{
   FC_ASSERT( filter_tag.empty() || start_tag.empty(), "Use either filter_tag or start_tag but not both!" );

   std::vector<half_tagging> result;
   result.reserve(100);
   const auto& idx = app.chain_database()->get_index_type<custom_tags_index>().indices().get<by_taggee>();
   auto itr = idx.lower_bound( boost::make_tuple( taggee, filter_tag.empty() ? start_tag : filter_tag, start_tagger ) );
   while( itr != idx.end() && itr->taggee == taggee && result.size() < 100 )
   {
       if( !filter_tag.empty() && itr->tag != filter_tag ) break;
       result.push_back( half_tagging( *itr++, true ) );
   }
   return result;
}

} // detail

bool custom_tags_api::is_tagged( const std::string& user, const std::string& tagged_by, const std::string& tag ) const
{
   return my->is_tagged( user, tagged_by, tag );
}

//const fc::optional<std::set<std::string>>& custom_tags_api::list_checked_tags()const
//{
//   return my->list_checked_tags();
//}
//
//const fc::optional<std::set<std::string>>& custom_tags_api::list_checked_taggers()const
//{
//   return my->list_checked_taggers();
//}

std::vector<half_tagging> custom_tags_api::list_tags_from( const std::string& tagger,
                                                           const std::string& filter_tag,
                                                           const std::string& start_tag,
                                                           const std::string& start_taggee )const
{
    return my->list_tags_from( tagger, filter_tag, start_tag, start_taggee );
}

std::vector<half_tagging> custom_tags_api::list_tags_on( const std::string& taggee,
                                                         const std::string& filter_tag,
                                                         const std::string& start_tag,
                                                         const std::string& start_tagger )const
{
    return my->list_tags_on( taggee, filter_tag, start_tag, start_tagger );
}

custom_tags_api::custom_tags_api( const muse::app::api_context& ctx )
{
   my = std::make_shared< detail::custom_tags_api_impl >( ctx.app );
}

void custom_tags_api::on_api_startup() {}

half_tagging::half_tagging( const tag_object& t, bool use_tagger )
   : user( use_tagger ? t.tagger : t.taggee ), tag(t.tag) {}

} } // muse::custom_tags
