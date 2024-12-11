#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ),mem( capacity ),pushed_cnt{0},poped_cnt{0} {}

bool Writer::is_closed() const
{
  // Your code here.
  return closed;
}

void Writer::push( string data )
{
  // Your code here.
  for(auto &c: data)
    if((pushed_cnt - poped_cnt) < capacity_){
      mem[(pushed_cnt++)%capacity_] = c;
    }else {
      break;
    }
}

void Writer::close()
{
  // Your code here.
  closed = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_- (pushed_cnt-poped_cnt);
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return pushed_cnt;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed && (pushed_cnt-poped_cnt) == 0;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return poped_cnt;
}

string_view Reader::peek() const
{
  // Your code here.
  string_view v;
  if( (pushed_cnt-poped_cnt) > 0) {
    uint64_t i = pushed_cnt % capacity_;
    uint64_t j = poped_cnt % capacity_;
    if( j < i ) {
      v = string_view(j+mem.begin(),i+mem.begin());
    }else{
      v = string_view(j+mem.begin(),mem.end());
    }
  }
  return v;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  while(len--) {
    if((pushed_cnt-poped_cnt) > 0){
      poped_cnt++;
    }else{
      break;
    }
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return pushed_cnt-poped_cnt;
}
