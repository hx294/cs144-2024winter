#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), mem_( capacity ), pushed_cnt_ { 0 }, poped_cnt_ { 0 }
{}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  if ( closed_ )
    return;
  uint64_t avai_ = ( capacity_ - ( pushed_cnt_ - poped_cnt_ ) );
  if ( data.size() > avai_ )
    data.resize( avai_ );
  for ( auto& c : data )
    mem_[( pushed_cnt_++ ) % capacity_] = std::move( c );
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - ( pushed_cnt_ - poped_cnt_ );
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return pushed_cnt_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && ( pushed_cnt_ - poped_cnt_ ) == 0;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return poped_cnt_;
}

string_view Reader::peek() const
{
  // Your code here.
  string_view v;
  if ( ( pushed_cnt_ - poped_cnt_ ) > 0 ) {
    uint64_t i = pushed_cnt_ % capacity_;
    uint64_t j = poped_cnt_ % capacity_;
    if ( j < i ) {
      v = string_view( j + mem_.begin(), i + mem_.begin() );
    } else {
      v = string_view( j + mem_.begin(), mem_.end() );
    }
  }
  return v;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  while ( len-- ) {
    if ( ( pushed_cnt_ - poped_cnt_ ) > 0 ) {
      poped_cnt_++;
    } else {
      break;
    }
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return pushed_cnt_ - poped_cnt_;
}
