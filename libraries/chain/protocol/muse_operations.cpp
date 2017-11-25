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

#include <muse/chain/protocol/base_operations.hpp>
#include <muse/chain/protocol/muse_operations.hpp>

#include <fc/io/json.hpp>

#include <locale>
#include <boost/algorithm/string/predicate.hpp>

namespace muse { namespace chain {

inline void validate_url(const string& url )
{
   FC_ASSERT(url.size() < MUSE_MAX_URL_LENGTH );
   FC_ASSERT(boost::starts_with( url, "ipfs://" ) );
   //TODO_MUSE - more checks...
}

void content_metadata_album_master::validate_meta() const{
   FC_ASSERT(album_title.size() > 0 && album_title.size() < 256, "Title larger than size limit");
   FC_ASSERT(fc::is_utf8(album_title), "Title not formatted in UTF8");
}


void content_metadata_track_master::validate_meta() const{
   FC_ASSERT(track_title.size() > 0 && track_title.size() < 256, "Title larger than size limit");
   FC_ASSERT(fc::is_utf8(track_title), "Title not formatted in UTF8");
}

void content_operation::validate() const {
   FC_ASSERT(is_valid_account_name(uploader));

   album_meta.validate_meta();
   track_meta.validate_meta();
   comp_meta.validate_meta();

   validate_url(url);
   uint32_t total_vote = 0;
   for( distribution d : distributions ) {
      FC_ASSERT(is_valid_account_name(d.payee));
      total_vote += d.bp;
   }
   FC_ASSERT(total_vote == 0 || total_vote == 10000, "when distributions are set, the sum must match 10000 bp");

   total_vote = 0;
   for( management_vote v : management ) {
      FC_ASSERT(is_valid_account_name(v.voter));
      total_vote += v.percentage;
   }
   FC_ASSERT(total_vote == 100, "Total managing votes must equal 100");

   if( comp_meta.third_party_publishers && distributions_comp ) {
      total_vote = 0;
      for( distribution d : *distributions_comp ) {
         FC_ASSERT(is_valid_account_name(d.payee));
         total_vote += d.bp;
      }
      FC_ASSERT(total_vote == 0 || total_vote == 10000,
                "when distributions are set, the sum must match 1000 bp");
   }

   if( comp_meta.third_party_publishers ) {
      total_vote = 0;
      FC_ASSERT(management_comp && management_threshold_comp);
      for( management_vote v : *management_comp ) {
         FC_ASSERT(is_valid_account_name(v.voter));
         total_vote += v.percentage;
      }
      FC_ASSERT(total_vote == 100, "Total managing votes must equal 100");
   }
   FC_ASSERT(playing_reward < 10000, "Split maximum is 10000 bp");
   FC_ASSERT(publishers_share<10000, "Split maximum is 10000 bp");
}

void content_update_operation::validate() const {
   validate_url(url);

   if(side == side_t::publisher ) {
      FC_ASSERT(!album_meta, "publisher cannot edit master side info");
      FC_ASSERT(!track_meta, "publisher cannot edit master side info");
   }

   if(album_meta)album_meta->validate_meta();
   if(track_meta)track_meta->validate_meta();
   if(comp_meta)comp_meta->validate_meta();

   if( new_distributions.size() > 0 ) {
      uint32_t total_vote = 0;
      for( distribution d : new_distributions ) {
         FC_ASSERT(is_valid_account_name(d.payee));
         total_vote += d.bp;
      }
      FC_ASSERT(total_vote == 10000, "New distributions must sum to 10000 bp");
   }
   if( new_management.size() > 0 ) {
      uint32_t total_vote = 0;
      for( management_vote v : new_management ) {
         total_vote += v.percentage;
         FC_ASSERT(is_valid_account_name(v.voter));
      }
      FC_ASSERT(total_vote == 100, "Total managing votes must equal 100");
   }
   FC_ASSERT(new_playing_reward < 10000, "Split maximum is 10000 bp");
   FC_ASSERT(new_publishers_share<10000, "Split maximum is 10000 bp");

}

void content_disable_operation::validate() const {
   validate_url(url);
}

void content_approve_operation::validate() const {
   FC_ASSERT(is_valid_account_name(approver));
   validate_url(url);
}

void streaming_platform_update_operation::validate() const {
   FC_ASSERT(is_valid_account_name(owner), "Owner account name invalid");
   FC_ASSERT(url.size() > 0, "URL size must be greater than 0");
   FC_ASSERT(fee >= asset(MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL));
   FC_ASSERT(fc::is_utf8(url));
}

void account_streaming_platform_vote_operation::validate() const {
   FC_ASSERT(is_valid_account_name(account), "Account ${a}", ("a", account));
   FC_ASSERT(is_valid_account_name(streaming_platform));
}

void streaming_platform_report_operation::validate() const {
   FC_ASSERT(is_valid_account_name(streaming_platform), "Invalid account");
   FC_ASSERT(is_valid_account_name(consumer), "Invalid account");
}

void friendship_operation::validate() const {
   FC_ASSERT(is_valid_account_name(who));
   FC_ASSERT(is_valid_account_name(whom));
}

void unfriend_operation::validate() const {
   FC_ASSERT(is_valid_account_name(who));
   FC_ASSERT(is_valid_account_name(whom));
}

void vote_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( voter ), "Voter account name invalid" );
      FC_ASSERT( abs(weight) <= MUSE_100_PERCENT, "Weight is not a MUSE percentage" );
   validate_url( url );
}

void balance_claim_operation::validate() const
{
   FC_ASSERT( balance_owner_key != public_key_type() );
}

}}