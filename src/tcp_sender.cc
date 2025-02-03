#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  uint64_t extra = 1; // FIN 占据一个序列号。
  if( reader().is_finished() ) {
    extra ++;
  }
  return extra + reader().bytes_popped() - first_out_index_;                                                                                                                      
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  (void)transmit;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  return {};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if(msg.ackno) {
    left_ = msg.ackno.value();
  }
  right_ = left_ + msg.window_size;

  //窗口最大值为 2^16-1，故每次发送最大为 2^16-1,小于Wrap32 的一半:2^31，则不会发送转换错误。
  while( outstanding_segments_.size() 
  && first_out_index_ + outstanding_segments_.front().sequence_length() <= left_.unwrap(isn_,first_out_index_)){
    first_out_index_ += outstanding_segments_.front().sequence_length();
    outstanding_segments_.pop();
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;
}
