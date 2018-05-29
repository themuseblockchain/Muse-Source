#include <muse/chain/content_object.hpp>

#include <algorithm>

namespace muse { namespace chain {


set< uint32_t > content_by_genre_index::get_genres( const content_object& c )const
{
   set< uint32_t > result;
   result.insert( c.album_meta.genre_1 );
   if( c.album_meta.genre_2 )
      result.insert( *c.album_meta.genre_2 );
   result.insert( c.track_meta.genre_1 );
   if( c.track_meta.genre_2 )
      result.insert( *c.track_meta.genre_2 );
   return result;
}

void content_by_genre_index::add_content( const set< uint32_t >& genres, content_id_type id )
{
   for( const auto genre : genres )
      content_by_genre[ genre ].insert( id );
}

void content_by_genre_index::remove_content( const set< uint32_t >& genres, content_id_type id )
{
   for( const auto genre : genres )
   {
      auto by_genre = content_by_genre.find( genre );
      if( by_genre != content_by_genre.end() )
         by_genre->second.erase( id );
   }
}

static set< uint32_t > intersect( const set< uint32_t > s1, const set< uint32_t > s2 )
{
   set< uint32_t > result;
   auto right = s2.begin();
   for( auto left : s1 )
   {
      while( right != s2.end() && *right < left ) right++;
      if( right == s2.end() ) return result;
      if( left == *right )
         result.insert( left );
   }
   return result;
}

static set< uint32_t > subtract( const set< uint32_t > s1, const set< uint32_t > s2 )
{
   set< uint32_t > result;
   auto right = s2.begin();
   for( auto left : s1 )
   {
      while( right != s2.end() && *right < left ) right++;
      if( right == s2.end() || left < *right )
         result.insert( left );
   }
   return result;
}

static const set< content_id_type > EMPTY;
const set< content_id_type >& content_by_genre_index::find_by_genre( uint32_t genre )const
{
   auto by_genre = content_by_genre.find( genre );
   if( by_genre == content_by_genre.end() )
      return EMPTY;
   return by_genre->second;
}

void content_by_genre_index::object_inserted( const object& obj )
{
   const content_object& c = static_cast< const content_object& >( obj );
   add_content( get_genres(c), c.id );
}

void content_by_genre_index::object_removed( const object& obj )
{
   const content_object& c = static_cast< const content_object& >( obj );
   remove_content( get_genres(c), c.id );
}

void content_by_genre_index::about_to_modify( const object& before )
{
   const content_object& c = static_cast< const content_object& >( before );
   FC_ASSERT( in_progress.find( c.id ) == in_progress.end() );
   in_progress[c.id] = get_genres( c );
}

void content_by_genre_index::object_modified( const object& after )
{
   const content_object& c = static_cast< const content_object& >( after );
   auto prev_genres = in_progress.find( c.id );
   FC_ASSERT( prev_genres != in_progress.end() );
   const set< uint32_t > new_genres = get_genres( c );
   const set< uint32_t > common = intersect( prev_genres->second, new_genres );
   remove_content( subtract( prev_genres->second, common ), c.id );
   add_content( subtract( new_genres, common ), c.id );
   in_progress.erase( prev_genres );
}


void content_by_category_index::add_content( const optional<string>& category, content_id_type id )
{
   if( !category ) return;
   content_by_category[ *category ].insert( id );
}

void content_by_category_index::remove_content( const optional<string>& category, content_id_type cid )
{
   if( !category ) return;
   auto itr = content_by_category.find( *category );
   if( itr == content_by_category.end() ) return;
   auto itr2 = itr->second.find( cid );
   if( itr2 == itr->second.end() ) return;
   itr->second.erase( itr2 );
   if( itr->second.empty() )
      content_by_category.erase( itr );
}

void content_by_category_index::object_inserted( const object& obj )
{
   const content_object& c = static_cast< const content_object& >( obj );
   add_content( c.album_meta.album_type, c.id );
}

void content_by_category_index::object_removed( const object& obj )
{
   const content_object& c = static_cast< const content_object& >( obj );
   remove_content( c.album_meta.album_type, c.id );
}

void content_by_category_index::about_to_modify( const object& before )
{
   const content_object& c = static_cast< const content_object& >( before );
   FC_ASSERT( in_progress.find( c.id ) == in_progress.end() );
   in_progress[c.id] = c.album_meta.album_type;
}

void content_by_category_index::object_modified( const object& after )
{
   const content_object& c = static_cast< const content_object& >( after );
   auto prev_category = in_progress.find( c.id );
   FC_ASSERT( prev_category != in_progress.end() );
   const optional<string>& new_category = c.album_meta.album_type;
   if( new_category.valid() != prev_category->second.valid()
          || ( new_category.valid() && *new_category != *prev_category->second ) )
   {
      remove_content( prev_category->second, c.id );
      add_content( new_category, c.id );
   }
   in_progress.erase( prev_category );
}

const set< content_id_type >& content_by_category_index::find_by_category( const string& category )const
{
   auto by_category = content_by_category.find( category );
   if( by_category == content_by_category.end() )
      return EMPTY;
   return by_category->second;
}

} } // muse::chain
