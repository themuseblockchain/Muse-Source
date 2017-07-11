#pragma once

#include <muse/chain/protocol/authority.hpp>
#include <muse/chain/protocol/types.hpp>
#include <muse/chain/protocol/base_operations.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace muse { namespace chain {

   using namespace graphene::db;


   /**
    *  All witnesses with at least 1% net positive approval and
    *  at least 2 weeks old are able to participate in block
    *  production.
    */
   class streaming_platform_object : public abstract_object<streaming_platform_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_streaming_platform_object_type;

         /** the account that has authority over this straming platform */
         string          owner;
         time_point_sec  created;
         string          url;
         //uint32_t        total_missed = 0;
         //uint64_t        last_aslot = 0;
         //uint64_t        last_confirmed_block_num = 0;

         /**
          *  The total votes for this streaming platform. 
          */
         share_type      votes;

         /**
          * This field represents the Muse blockchain version the witness is running.
          */

         streaming_platform_id_type get_id()const { return id; }
   };


   class streaming_platform_vote_object : public abstract_object<streaming_platform_vote_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_streaming_platform_vote_object_type;

         streaming_platform_id_type streaming_platform;
         account_id_type account;
   };

   class report_object : public abstract_object<report_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_report_object_type;

         streaming_platform_id_type streaming_platform;
         account_id_type consumer;
         content_id_type content;
         time_point_sec created;
         uint32_t play_time;
         optional<account_id_type> playlist_creator;
         //TODO_MUSE - content_id_type content
   };

  /**
    * @ingroup object_index
    */
   struct by_name;
   typedef multi_index_container<
      streaming_platform_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_name>, member<streaming_platform_object, string, &streaming_platform_object::owner> >,
         ordered_unique< tag<by_vote_name>,
            composite_key< streaming_platform_object,
               member<streaming_platform_object, share_type, &streaming_platform_object::votes >,
               member<streaming_platform_object, string, &streaming_platform_object::owner >
            >,
            composite_key_compare< std::greater< share_type >, std::less< string > >
         >
      >
   > streaming_platform_multi_index_type;

   struct by_account_streaming_platform;
   struct by_streaming_platform_account;
   typedef multi_index_container<
      streaming_platform_vote_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_account_streaming_platform>,
            composite_key< streaming_platform_vote_object,
               member<streaming_platform_vote_object, account_id_type, &streaming_platform_vote_object::account >,
               member<streaming_platform_vote_object, streaming_platform_id_type, &streaming_platform_vote_object::streaming_platform >
            >,
            composite_key_compare< std::less< account_id_type >, std::less< streaming_platform_id_type > >
         >,
         ordered_unique< tag<by_streaming_platform_account>,
            composite_key< streaming_platform_vote_object,
               member<streaming_platform_vote_object, streaming_platform_id_type, &streaming_platform_vote_object::streaming_platform >,
               member<streaming_platform_vote_object, account_id_type, &streaming_platform_vote_object::account >
            >,
            composite_key_compare< std::less< streaming_platform_id_type >, std::less< account_id_type > >
         >
      > // indexed_by
   > streaming_platform_vote_multi_index_type;


   typedef generic_index< streaming_platform_object,         streaming_platform_multi_index_type>             streaming_platform_index;
   typedef generic_index< streaming_platform_vote_object,    streaming_platform_vote_multi_index_type >       streaming_platform_vote_index;
   
   struct by_consumer;
   struct by_content;
   struct by_streaming_platform;
   struct by_created;
   typedef multi_index_container<
      report_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_consumer>, 
            composite_key< report_object, 
               member< report_object, account_id_type, &report_object::consumer >,
               member< object, object_id_type,  &object::id >
            >
         >,
         ordered_unique< tag<by_streaming_platform>, 
            composite_key< report_object,   
               member<report_object, streaming_platform_id_type, &report_object::streaming_platform>,
               member<object, object_id_type, &object::id >
            >
         >,
         ordered_unique< tag<by_created>, member<report_object, time_point_sec,  &report_object::created> >
      >
   > report_object_multi_index_type;
   typedef generic_index< report_object, report_object_multi_index_type > report_index;
} }

FC_REFLECT_DERIVED( muse::chain::streaming_platform_object, (graphene::db::object),
                    (owner)
                    (created)
                    (url)(votes)
                  )
FC_REFLECT_DERIVED( muse::chain::streaming_platform_vote_object, (graphene::db::object), (streaming_platform)(account) )

FC_REFLECT_DERIVED( muse::chain::report_object, (graphene::db::object), 
                    (streaming_platform)
                    (consumer)
                    (created)
                    (play_time)
                    (playlist_creator)
                  )

