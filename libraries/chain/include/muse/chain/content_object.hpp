#pragma once

#include <muse/chain/protocol/authority.hpp>
#include <muse/chain/protocol/types.hpp>
#include <muse/chain/protocol/base_operations.hpp>
#include <muse/chain/witness_objects.hpp>
#include <muse/chain/streaming_platform_objects.hpp>
#include <muse/chain/protocol/muse_operations.hpp>
 
#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace muse { namespace chain {

   using namespace graphene::db;

   class content_object : public abstract_object<content_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_content_object_type;

         string uploader;
         
         string            url;
         asset             accumulated_balance_master;
         asset             accumulated_balance_comp;

         content_metadata_album_master album_meta;
         content_metadata_track_master track_meta;
         content_metadata_publisher comp_meta;

         string track_title;  //this is copy of track_meta.track_title; we need it as C++ does not allow pointers to nested class members, which are used in indices.

         time_point_sec    last_update;
         time_point_sec    created;
         time_point_sec    last_played;
         
         vector <distribution> distributions_master;
         vector <distribution> distributions_comp;

         uint32_t playing_reward=1000;
         uint32_t publishers_share=5000;

         authority manage_master;
         authority manage_comp;

         uint64_t times_played=0;
         uint32_t times_played_24=0;

         bool           curation_rewards=true;
         time_point_sec curation_reward_expiration;

         bool allow_votes   = true;      /// allows a post to receive votes;

         bool disabled = false;

         friend bool operator<(const content_object& a, const content_object& b) {
            return a.id<b.id;
         }
   };

   class content_approve_object : public abstract_object<content_approve_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_content_approve_object_type;

         string content; //url
         account_id_type approver;
   };

   /**
    * This index maintains the set of voter/content pairs that have been used, voters cannot
    * vote on the same content more than once per payout period.
    */
   class content_vote_object : public abstract_object<content_vote_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_content_vote_object_type;
         account_id_type voter;
         content_id_type content;
         int16_t weight;
         int8_t num_changes = 0;
         bool marked_for_curation_reward = false;
         time_point_sec  last_update; ///< The time of the last update of the vote
   };

   struct by_content;
   struct by_approver;
   struct by_content_approver;
   struct by_approver_content;
   typedef multi_index_container<
      content_approve_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_content >, member< content_approve_object, string, &content_approve_object::content > >,
         ordered_unique< tag< by_approver >, member< content_approve_object, account_id_type, &content_approve_object::approver > >,
         ordered_unique< tag< by_content_approver >,
            composite_key< content_approve_object,
               member< content_approve_object, string, &content_approve_object::content >,
               member< content_approve_object, account_id_type, &content_approve_object::approver >
            >
         >,
         ordered_unique< tag< by_approver_content >,
            composite_key< content_approve_object,
                member< content_approve_object, account_id_type, &content_approve_object::approver >,
                member< content_approve_object, string, &content_approve_object::content >
            >
         >
      >
   > content_approve_multi_index_type;

   struct by_reward_flag_update;

   struct by_content_voter;
   struct by_voter_content;
   typedef multi_index_container<
      content_vote_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_content_voter >,
            composite_key< content_vote_object,
               member< content_vote_object, content_id_type, &content_vote_object::content>,
               member< content_vote_object, account_id_type, &content_vote_object::voter>
            >
         >,
         ordered_unique< tag< by_voter_content >,
            composite_key< content_vote_object,
               member< content_vote_object, account_id_type, &content_vote_object::voter>,
               member< content_vote_object, content_id_type, &content_vote_object::content>
            >
         >,
         ordered_non_unique< tag< by_reward_flag_update >,
            composite_key< content_vote_object,
               member< content_vote_object, bool, &content_vote_object::marked_for_curation_reward >,
               member< content_vote_object, time_point_sec, &content_vote_object::last_update >
            >
         >
      >
   > content_vote_multi_index_type;

   struct by_url; 
   struct by_title;
   struct by_uploader;
   struct by_popularity;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      content_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_url >, member< content_object, string, &content_object::url> >,
         ordered_non_unique< tag< by_title >, member< content_object, string, &content_object::track_title > >,
         ordered_unique< tag< by_uploader >,
            composite_key< content_object, 
               member< content_object, string, &content_object::uploader >,
               member< object, object_id_type, &object::id >
            >,
           composite_key_compare< std::less< string >, std::greater< object_id_type > >
         >,
         ordered_non_unique< tag< by_popularity >, member< content_object, uint32_t, &content_object::times_played_24 > >
      >
   > content_multi_index_type;

   typedef generic_index< content_object,      content_multi_index_type >       content_index;
   typedef generic_index< content_vote_object, content_vote_multi_index_type >  content_vote_index;
   typedef generic_index< content_approve_object, content_approve_multi_index_type > content_approve_index;

   /**
    *  @brief This secondary index will allow a looking up content by genre.
    */
   class content_by_genre_index : public secondary_index
   {
      public:
         virtual void object_inserted( const object& obj ) override;
         virtual void object_removed( const object& obj ) override;
         virtual void about_to_modify( const object& before ) override;
         virtual void object_modified( const object& after  ) override;

         const set< content_id_type >& find_by_genre( uint32_t genre )const;

      private:
         set< uint32_t > get_genres( const content_object& c )const;
         void add_content( const set< uint32_t >& genres, content_id_type id );
         void remove_content( const set< uint32_t >& genres, content_id_type id );
         map< content_id_type, set<uint32_t> > in_progress;
         map< uint32_t, set<content_id_type> > content_by_genre;
   };

   /**
    *  @brief This secondary index will allow a looking up content by category.
    */
   class content_by_category_index : public secondary_index
   {
      public:
         virtual void object_inserted( const object& obj ) override;
         virtual void object_removed( const object& obj ) override;
         virtual void about_to_modify( const object& before ) override;
         virtual void object_modified( const object& after  ) override;

         const set< content_id_type >& find_by_category( const string& category )const;

      private:
         void add_content( const optional<string>& category, content_id_type cid );
         void remove_content( const optional<string>& category, content_id_type cid );
         map< content_id_type, optional<string> > in_progress;
         map< string, set<content_id_type> > content_by_category;
   };

} } // muse::chain

FC_REFLECT_DERIVED( muse::chain::content_object, (graphene::db::object),
                    (album_meta)(track_meta)(comp_meta)
                    (uploader)(url)(accumulated_balance_master)(accumulated_balance_comp)
                    (last_update)(created)(last_played)(times_played)(times_played_24)
                    (allow_votes)(playing_reward) (publishers_share)
                    (manage_master)(manage_comp)(distributions_master)(distributions_comp)
                    (curation_rewards)(curation_reward_expiration)
                    )

FC_REFLECT_DERIVED( muse::chain::content_approve_object, (graphene::db::object),
                    (approver)(content) )

FC_REFLECT_DERIVED( muse::chain::content_vote_object, (graphene::db::object),
                    (voter)(content)(marked_for_curation_reward)(weight)(num_changes)(last_update) )
