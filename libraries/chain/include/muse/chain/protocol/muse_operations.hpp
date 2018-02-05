/*
 * Copyright (c) 2017 MUSE, and contributors.
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
#include <muse/chain/protocol/base.hpp>
#include <muse/chain/protocol/asset.hpp>
#include <muse/chain/protocol/address.hpp>
#include <fc/optional.hpp>

namespace muse { namespace chain {

struct distribution{
   string payee; //<Account having right to receive royalties
   uint32_t bp=0; //Share to receive, in base points
};

struct management_vote{
   string voter; //<Account having voting right
   uint32_t percentage=0; //<Voting power
};

/*
 * Virtual operation issued by the system when payout for content playing happens
 */
struct content_reward_operation : public base_operation
{
   content_reward_operation(){} //<Constructor
   content_reward_operation( const string& a, const string& p, const asset& s, const asset& v )
         :payee(a),url(p),mbd_payout(s),vesting_payout(v){} //<Constructor
   string payee; //<Account receiving payment
   string url; //<URL of the content for which payment is made
   asset  mbd_payout; //<MBD payout
   asset  vesting_payout; //<Vesting payout
   void  validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
};

/*
 * Virtual operation issued by the system when payout for curration happens
 */
struct curate_reward_operation : public base_operation
{
   curate_reward_operation(){} //<Constructor
   curate_reward_operation( const string& c, const string& u, const asset& s, const asset& v )
         :curator(c),url(u),mbd_payout(s),vesting_payout(v){} //<Constructor

   string curator;//<Account receiving payment
   string url;//<URL of the content for which payment is made
   asset  mbd_payout;//<MBD payout
   asset  vesting_payout;//<Vesting payout
   void   validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
};

/*
 * Virtual operation issued by the system when streaming platform is paid for playing
 */
struct playing_reward_operation : public base_operation
{
   playing_reward_operation(){};//<Constructor
   playing_reward_operation( const string& p, const string& u, const asset& m, const asset &v )
         :platform(p),url(u),mbd_payout(m), vesting_payout(v) {};//<Constructor

   string platform;//<Streaming platform receiving payment
   string url;//<URL of the content for which payment is made
   asset  mbd_payout;//<MBD payout
   asset  vesting_payout;//<Vesting payout
   void   validate()const { FC_ASSERT( false, "this is a virtual operation" ); }
};

/**
 * Submit a new content to the network.
 */
enum genre{
   rock,
   pop,
   metal, //to be added
   GENRE_LAST_ENTRY
};

struct content_metadata_album_master{
   bool part_of_album; //<Part of an Album or a Single?
   string album_title; //<Album title
   vector<string> album_artist; //<List of artists
   uint32_t genre_1; //<from enum genre
   optional<uint32_t> genre_2; //<from enum genre
   string country_of_origin;
   enum explicit_e{yes, no, clean};
   uint32_t explicit_;
   string p_line;
   string c_line;
   uint64_t upc_or_ean;
   uint32_t release_date;
   uint32_t release_year;
   uint32_t sales_start_date;
   optional<string> album_producer;
   optional<string> album_type;
   string master_label_name;
   string display_label_name;
   void validate_meta() const;
};

struct content_metadata_track_master{
   string track_title; //<Track title
   string ISRC;
   struct track_artist {
      string artist;
      optional<vector<string>> aliases;
      optional<uint64_t> ISNI;
   };
   vector <track_artist> track_artists;
   optional<string> featured_artist;
   optional<uint64_t> featured_artist_ISNI;
   optional<string> track_producer;
   uint32_t genre_1; //<from enum genre
   optional<uint32_t> genre_2; //<from enum genre
   string p_line;
   uint32_t track_no;
   uint32_t track_volume;
   optional<string> json_metadata;
   uint32_t track_duration;
   bool samples;
   void validate_meta()const;
};

struct content_metadata_publisher{
   string composition_title; //<Publishing title
   optional<string> alternate_composition_title;
   optional<string> ISWC;
   bool third_party_publishers;
   struct publisher{
      string publisher;
      optional<string> IPI_CAE;
      optional<uint64_t> ISNI;
   };
   vector<publisher> publishers;
   enum writer_role{
      composer,
      lyrics,
      music_lyrics,
      arranger
   };
   struct writer{
      string writer;
      optional<string> IPI_CAE;
      optional<uint64_t> ISNI;
      optional<uint32_t> role; //<from writer_role
      string publisher; //<must refer to the one from publishers
   };
   vector<writer> writers;
   string PRO;
   void validate_meta()const {};
};

/*
 * Operation which creates content entry
 */
struct content_operation : public base_operation
{
   string uploader; //<Account which created the record
   string url; //<URL of the content. Must be unique in the network

   content_metadata_album_master album_meta; //<Album metadata
   content_metadata_track_master track_meta; //<Track metadata
   content_metadata_publisher comp_meta; //<Publishing metadata
   vector <distribution> distributions; //<List of royalties receivers. Either empty or sum of shares must match 10000BPs
   vector <management_vote>  management; //<List of accounts managing this piece, its master side
   uint32_t management_threshold; //<Threshold needed to achieve consensus on management vote
   optional <vector<management_vote> > management_comp; //<List of royalties receivers. Either empty or sum of shares must match 10000BPs
   optional <vector<distribution> > distributions_comp; //<List of accounts managing publishing side of this piece
   optional <uint32_t> management_threshold_comp; //<Threshold needed to achieve consensus on management vote

   uint32_t playing_reward=1000; //< Share reward of the playing platform, in bp
   uint32_t publishers_share=5000; //< Share of royalties dedicateed for the publishers, in bp

   void validate()const;
   void get_required_active_authorities( flat_set<string>& a )const{ a.insert(uploader); }
};

/**
 * Change content parameters
 */
struct content_update_operation : public base_operation
{
   enum side_t { master=0, publisher=1 };
   uint32_t side = master; //< Side which has to be changed
   string url; //<URL of the content to be changed

   optional<content_metadata_album_master> album_meta; //<New album metadata
   optional<content_metadata_track_master> track_meta; //<New track metadata
   optional<content_metadata_publisher> comp_meta; //<New publishing metadata

   vector <distribution> new_distributions; //New royalties distribution split
   vector <management_vote>  new_management; //New list of accounts managing this piece.
   uint32_t new_threshold; //<Threshold needed to achieve consensus on management vote
   uint32_t new_playing_reward;//< Share reward of the playing platform, in bp
   uint32_t new_publishers_share; //< Share of royalties dedicateed for the publishers, in bp

   void validate()const;
   void get_required_master_content_authorities( flat_set<string>& a )const{ if(side == master || new_playing_reward||new_publishers_share)a.insert(url); }
   void get_required_comp_content_authorities( flat_set<string>& a )const{ if(side == publisher || new_playing_reward||new_publishers_share)a.insert(url); }
};


/**
 * Remove content from MUSE
 */
struct content_disable_operation : public base_operation
{
   string url; //<URL of the content to be removed

   void validate()const;
   void get_required_master_content_authorities( flat_set<string>& a )const{ a.insert(url); }
};

/**
 * Approve content submitted to the network
 */
struct content_approve_operation : public base_operation
{
   string approver; //<Account approving the content
   string url;//<URL of the content

   void validate()const;
   void get_required_basic_authorities( flat_set<string>& a )const{ a.insert(approver); }
};

/**
 *Enable building friendship.
 */
struct friendship_operation : public base_operation
{
   string who; //<Requestor for the "friendship"
   string whom; //<Receiver of the "friendship" request

   void validate()const;
   void get_required_basic_authorities( flat_set<string>& a )const{ a.insert(who); }
};

/*
 * Cancel friendship relationship
 */
struct unfriend_operation : public base_operation
{
   string who; //<The account canceling the request
   string whom; //<The account specifying relationship to be canceled. The relationship who-whom must exist

   void validate()const;
   void get_required_basic_authorities( flat_set<string>& a )const{ a.insert(who); }
};

/**
 *  If the owner isn't a streaming platform they will become a steaming platform.
 *
 *  The network will pick the top N platforms for
 *  streaming.
 *
 */
struct streaming_platform_update_operation : public base_operation
{
   string            owner; //< Owner of the account being updated
   string            url; //< Identifier of the new streaming patform
   asset             fee; ///< The fee paid to register a new streaming platfrom

   void validate()const;
   void get_required_active_authorities( flat_set<string>& a )const{ a.insert(owner); }

};


/**
 * All accounts with a VFS can vote for or against any streaming platform.
 *
 */
struct account_streaming_platform_vote_operation : public base_operation
{
   string account; //<Voter account
   string streaming_platform; //<Platform voted for
   bool approve = true; //<Approve or disapprove

   void validate() const;
   void get_required_basic_authorities(  flat_set<string>& a )const{ a.insert(account); }
};

/**
 * Report information who listened to what. Only streaming platform can submit the streaming reports.
 *
 */
struct streaming_platform_report_operation : public base_operation
{
   string streaming_platform; //<Platform submiting the report
   string consumer; //<Consumer of the art piece
   string content; //<URL of the art piece
   uint64_t play_time; //<How long the consumer listened to it (in seconds))
   string playlist_creator;//<Not used

   void validate() const;
   void get_required_active_authorities(  flat_set<string>& a )const{ a.insert(streaming_platform); }
};

/*
 * All accounts with a VFS can vote for any content piece.
 */
struct vote_operation : public base_operation
{
   string    voter; //<Voter account
   string    url; //<Content voted for
   int16_t   weight = 0; //<Vote weight

   void validate()const;
   void get_required_basic_authorities( flat_set<string>& a )const{ a.insert(voter); }
};

/**
 * @brief Claim a balance in a @ref balanc_object
 *
 * This operation is used to claim the balance in a given @ref balance_object. If the balance object contains a
 * vesting balance, @ref total_claimed must not exceed @ref balance_object::available at the time of evaluation. If
 * the object contains a non-vesting balance, @ref total_claimed must be the full balance of the object.
 */
struct balance_claim_operation : public base_operation
{
   struct fee_parameters_type {};

   string            deposit_to_account;
   balance_id_type   balance_to_claim;
   public_key_type   balance_owner_key;
   asset             total_claimed;

   void            validate()const;
   void            get_required_authorities( vector<authority>& a  )const
   {
      authority key(100, balance_owner_key, 100);
      a.push_back( key );
   }
};


}}

FC_REFLECT(muse::chain::content_metadata_album_master,(part_of_album)(album_title)(album_artist)(genre_1)(genre_2)
      (country_of_origin)(explicit_)(p_line)(c_line)(upc_or_ean)(release_date)(release_year)(sales_start_date)
      (album_producer)(album_type)(master_label_name)(display_label_name))
FC_REFLECT(muse::chain::content_metadata_publisher::publisher,(publisher)(IPI_CAE)(ISNI))
FC_REFLECT(muse::chain::content_metadata_publisher::writer,(writer)(IPI_CAE)(ISNI)(role)(publisher))
FC_REFLECT(muse::chain::content_metadata_track_master::track_artist,(artist)(aliases)(ISNI))
FC_REFLECT(muse::chain::content_metadata_track_master,(track_title)(ISRC)(track_artists)(featured_artist)(featured_artist_ISNI)
      (track_producer)(genre_1)(genre_2)(p_line)(track_no)(track_volume)(json_metadata)(track_duration)(samples) )
FC_REFLECT(muse::chain::content_metadata_publisher,(composition_title)(alternate_composition_title)(ISWC)(third_party_publishers)
      (publishers)(writers)(PRO))

FC_REFLECT( muse::chain::distribution, (payee)(bp) )
FC_REFLECT( muse::chain::management_vote, (voter)(percentage) )
FC_REFLECT( muse::chain::content_operation, (uploader)(url)(album_meta)(track_meta)(comp_meta)(distributions)
      (management)(management_threshold)(distributions_comp)(management_comp)(management_threshold_comp)
      (playing_reward)(publishers_share) )
FC_REFLECT_ENUM( muse::chain::content_update_operation::side_t,(master)(publisher) )
FC_REFLECT( muse::chain::content_update_operation, (url)(side)(album_meta)(track_meta)(comp_meta)(new_distributions)(new_management)(new_threshold)(new_playing_reward)(new_publishers_share) )
FC_REFLECT( muse::chain::content_approve_operation, (approver)(url) )
FC_REFLECT( muse::chain::content_disable_operation, (url) )
FC_REFLECT( muse::chain::friendship_operation, (who)(whom) )
FC_REFLECT( muse::chain::unfriend_operation, (who)(whom) )
FC_REFLECT( muse::chain::content_reward_operation, (payee)(url)(mbd_payout)(vesting_payout) )
FC_REFLECT( muse::chain::curate_reward_operation, (curator)(url)(mbd_payout)(vesting_payout) )
FC_REFLECT( muse::chain::playing_reward_operation, (platform)(url)(mbd_payout)(vesting_payout) )
FC_REFLECT( muse::chain::streaming_platform_report_operation, (streaming_platform)(consumer)(content)(playlist_creator)(play_time))
FC_REFLECT( muse::chain::account_streaming_platform_vote_operation, (account)(streaming_platform)(approve))
FC_REFLECT( muse::chain::streaming_platform_update_operation, (owner)(url)(fee))
FC_REFLECT( muse::chain::vote_operation, (voter)(url)(weight) )
FC_REFLECT( muse::chain::balance_claim_operation,
            (deposit_to_account)(balance_to_claim)(balance_owner_key)(total_claimed) )