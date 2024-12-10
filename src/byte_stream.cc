#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return closed;
}

void Writer::push( string data )
{
  // Your code here.
  for(auto &c: data)
    if(mem.size() <= capacity_){
      mem.push_back(c);
      pushed_cnt++;
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
  return capacity_- mem.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return pushed_cnt;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed && mem.size()==0;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return poped_cnt;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view(mem.begin(),mem.end());
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  while(len--) {
    if(mem.size() > 0){
      poped_cnt ++;
      mem.pop_front();
    }else{
      break;
    }
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return mem.size();
}
