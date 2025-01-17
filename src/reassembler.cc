#include "reassembler.hh"
#include <algorithm>

using namespace std;

typedef pair<string, pair<uint64_t, bool>>  Pair;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  (void)first_index;
  (void)data;
  (void)is_last_substring;
}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t res = 0;
  for(auto r = unassembled_bytes_.begin(); r  != unassembled_bytes_.end(); r ++) 
  {
    res += r->first.size();
  }

  return res;
}

void Reassembler::delete_redundancy( uint64_t begin, uint64_t end ) 
{
  for( auto  r = unassembled_bytes_.begin(); r != unassembled_bytes_.end(); r ++) 
  {
    if ( r->second.first >= end || 
    r->first.size() + r->second.first < begin ) {
      continue;
    } else {
      uint64_t new_begin = max( r->second.first, begin);
      uint64_t new_end = min(end, r->second.first + r->first.size() );
      r->second.first = new_begin;
      r->first = r->first.erase(0,new_begin - begin );
      r->first.resize(end - new_end); 
      if ( r->first.size() == 0) {
        unassembled_bytes_.erase(r);
      }
    }
  }
}

void Reassembler::check_Reassembler( uint64_t begin , Writer& w) 
{
  for(auto r = unassembled_bytes_.begin(); r != unassembled_bytes_.end(); r ++) 
  {
    if(r->second.first == begin) {
      w.push(r->first);
      begin += r->first.size();
    }
  }
}

void Reassembler::add_bytes( uint64_t first_index, string& data, bool is_last_string ) 
{
  uint64_t begin = first_index;
  uint64_t end = begin + data.size();

  delete_redundancy(begin,end);

  unassembled_bytes_.push_back({move(data),{first_index,is_last_string}});
}
