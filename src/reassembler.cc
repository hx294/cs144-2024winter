#include "reassembler.hh"
#include <algorithm>

using namespace std;

typedef pair<string, pair<uint64_t, bool>>  Pair;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  Writer& w = output_.writer();
  uint64_t left = w.available_capacity();
  if( first_index <= expect_index ) {
    uint64_t pushed_size = min( left , data.size() - ( expect_index - first_index));
    delete_redundancy(expect_index,expect_index + pushed_size);
    data.erase(0,expect_index-first_index);
    w.push(data);
    if(is_last_substring) w.close();
    expect_index += pushed_size;
    check_Reassembler(expect_index, w);
  } else {
    add_bytes(first_index, data, is_last_substring,left);
  }
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
  if ( begin >= end) return ;
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
        r = unassembled_bytes_.erase(r);
        r--;
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
      bool mark = r->second.second;
      r = unassembled_bytes_.erase(r);
      r--;
      if(mark) {
        w.close();
        break;
      }
    }
  }
  expect_index = begin;
}

void Reassembler::add_bytes( uint64_t first_index, string& data, bool is_last_string , uint64_t left) 
{
  uint64_t begin = first_index;
  uint64_t end = begin + data.size();

  // 防止超出capacity
  end = min(end,left + expect_index);
  if(end > begin)
  {
    if(end - begin < data.size()) is_last_string = false;
    data.resize(end - begin);
    delete_redundancy(begin,end);
    unassembled_bytes_.push_back({move(data),{first_index,is_last_string}});
  }
}
