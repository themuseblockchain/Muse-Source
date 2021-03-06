/*
 * Copyright (c) 2018 Peertracks, Inc., and contributors.
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

#include <muse/app/plugin.hpp>
#include <muse/chain/database.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef CUSTOM_TAGS_SPACE_ID
#define CUSTOM_TAGS_SPACE_ID 8
#endif

namespace muse { namespace custom_tags {

using namespace graphene::db;

namespace detail {
   class custom_tags_impl;
}

class custom_tags_plugin : public muse::app::plugin {
   public:
      custom_tags_plugin();
      ~custom_tags_plugin();

      std::string plugin_name()const override;

      virtual void plugin_set_program_options(
         boost::program_options::options_description &command_line_options,
         boost::program_options::options_description &config_file_options
      ) override;

      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

   private:
      std::unique_ptr<detail::custom_tags_impl> my;
};

struct tag_object : public graphene::db::abstract_object< tag_object > {
   static const uint8_t space_id = CUSTOM_TAGS_SPACE_ID;
   static const uint8_t type_id = 1;

   std::string tagger;
   std::string tag;
   std::string taggee;
};

struct by_tagger;
struct by_taggee;
typedef boost::multi_index_container<
   tag_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_tagger >,
         composite_key< tag_object,
                        member< tag_object, std::string, &tag_object::tagger >,
                        member< tag_object, std::string, &tag_object::tag >,
                        member< tag_object, std::string, &tag_object::taggee > > >,
      ordered_unique< tag< by_taggee >,
         composite_key< tag_object,
                        member< tag_object, std::string, &tag_object::taggee >,
                        member< tag_object, std::string, &tag_object::tag >,
                        member< tag_object, std::string, &tag_object::tagger > > >
   >
> custom_tags_multi_index_container;
typedef generic_index<tag_object, custom_tags_multi_index_container> custom_tags_index;

} } // muse::custom_tags

FC_REFLECT_DERIVED( muse::custom_tags::tag_object, (graphene::db::object),
                    (tagger)(tag)(taggee) )
