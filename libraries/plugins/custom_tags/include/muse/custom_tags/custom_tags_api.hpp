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
#pragma once

#include <muse/custom_tags/custom_tags.hpp>

#include <muse/chain/protocol/types.hpp>

#include <fc/api.hpp>

namespace muse { namespace custom_tags {

namespace detail {
   class custom_tags_api_impl;
}

class half_tagging {
public:
   half_tagging( const tag_object& t, bool use_tagger );

   std::string user;
   std::string tag;
};

class custom_tags_api
{
   public:
      explicit custom_tags_api( const muse::app::api_context& ctx );

      void on_api_startup();

      /**
       * @brief Checks if the given user has received the given tagged from the tagged_by user
       * @param user the user who might have been tagged
       * @param tagged_by the user who might have tagged "user"
       * @param the tag to check for
       * @return true iff we know that "tagged_by" has tagged "user" with "tag"
       */
      bool is_tagged( const std::string& user, const std::string& tagged_by, const std::string& tag ) const;

      /**
       * @brief Returns a list of all tags checked by this node, or null if it handles all tags
       */
      //const fc::optional<std::set<std::string>>& list_checked_tags()const;

      /**
       * @brief Returns a list of all taggers checked by this node, or null if it handles all taggers
       */
      //const fc::optional<std::set<std::string>>& list_checked_taggers()const;

      /**
       * @brief Returns a possibly filtered list of up to 100 known tags, ordered by (tag,taggee)
       * @param tagger the creator of the tags to return
       * @param filter_tag only return tags with this value, or empty string
       * @param start_tag start with tags containing this value, or empty string
       * @param start_taggee start with tags set on this user, or empty string
       */
      std::vector<half_tagging> list_tags_from( const std::string& tagger,
                                                const std::string& filter_tag,
                                                const std::string& start_tag,
                                                const std::string& start_taggee )const;

      /**
       * @brief Returns a possibly filtered list of up to 100 known tags, ordered by (tag,tagger)
       * @param taggee the recipient of the tags to return
       * @param filter_tag only return tags with this value, or empty string
       * @param start_tag start with tags containing this value, or empty string
       * @param start_tagger start with tags set by this user, or empty string
       */
      std::vector<half_tagging> list_tags_on( const std::string& taggee,
                                              const std::string& filter_tag,
                                              const std::string& start_tag,
                                              const std::string& start_tagger )const;

   private:
      std::shared_ptr< detail::custom_tags_api_impl > my;
};

} } // muse::custom_tags

FC_REFLECT( muse::custom_tags::half_tagging, (user)(tag) )

FC_API( muse::custom_tags::custom_tags_api,
        (is_tagged)
//        (list_checked_tags)
//        (list_checked_taggers)
        (list_tags_from)
        (list_tags_on)
      )
