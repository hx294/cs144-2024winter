#include "reassembler.hh"
#include <algorithm>

using namespace std;

typedef pair<string, pair<uint64_t, bool>> Pair;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  Writer& w = output_.writer();
  uint64_t left = w.available_capacity();
  if ( first_index + data.size() <= expect_index ) {
    if ( is_last_substring )
      w.close();
  } else if ( first_index <= expect_index ) {
    uint64_t pushed_size = min( left, data.size() - ( expect_index - first_index ) );
    delete_redundancy( expect_index, expect_index + pushed_size );
    data = data.erase( 0, expect_index - first_index );
    data.resize( pushed_size );
    w.push( data );
    if ( is_last_substring ) {
      w.close();
      return;
    }
    expect_index += pushed_size;
    check_Reassembler( expect_index, w );
  } else {
    add_bytes( first_index, data, is_last_substring, left );
  }
}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t res = 0;
  for ( auto r = unassembled_bytes_.begin(); r != unassembled_bytes_.end(); r++ ) {
    res += r->first.size();
  }

  return res;
}

void Reassembler::delete_redundancy( uint64_t begin, uint64_t end )
{
  if ( begin >= end )
    return;
  for ( auto r = unassembled_bytes_.begin(); r != unassembled_bytes_.end(); r++ ) {
    if ( r->second.first >= end || r->first.size() + r->second.first <= begin ) {
      continue;
    } else {
      // 字符串r的末尾位置在end右边
      if ( end < r->second.first + r->first.size() ) {
        Pair p { r->first, { end, r->second.second } };
        p.first = p.first.erase( 0, end - r->second.first );
        r = unassembled_bytes_.insert( r, move( p ) );
        r++;
      }
      // 字符串r的起始位置在begin左边
      if ( begin > r->second.first ) {
        Pair p = { r->first, { r->second.first, false } };
        p.first = p.first.erase( begin - r->second.first, r->second.first + r->first.size() - begin );
        r = unassembled_bytes_.insert( r, move( p ) );
        r++;
      }
      r = unassembled_bytes_.erase( r );
      r--;
    }
  }
}

void Reassembler::check_Reassembler( uint64_t begin, Writer& w )
{
  for ( auto r = unassembled_bytes_.begin(); r != unassembled_bytes_.end(); ) {
    if ( r->second.first == begin ) {
      w.push( r->first );
      begin += r->first.size();
      bool mark = r->second.second;
      r = unassembled_bytes_.erase( r );
      // 前面无法写入的字节可能可以写入，所以需要重新遍历。
      r = unassembled_bytes_.begin();
      if ( mark ) {
        w.close();
        break;
      }
    } else {
      r++;
    }
  }
  expect_index = begin;
}

void Reassembler::add_bytes( uint64_t first_index, string& data, bool is_last_string, uint64_t left )
{
  uint64_t begin = first_index;
  uint64_t end = begin + data.size();

  // 防止超出capacity
  end = min( end, left + expect_index );
  if ( end > begin ) {
    if ( end - begin < data.size() )
      is_last_string = false;
    data.resize( end - begin );
    delete_redundancy( begin, end );
    unassembled_bytes_.push_back( { move( data ), { first_index, is_last_string } } );
  }
}
