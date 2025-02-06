#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <algorithm>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return last_out_index_next_ - first_out_index_;                                                                                                                      
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  TCPSenderMessage m;
  // 窗口大小为0
  if( window_size_ == 0 ) {
    // 伪装成窗口为1的样子
    m = {isn_,true,"",false,false};
    transmit(m);
  } else {
    Reader& r = input_.reader();
    string_view s = r.peek();
    // 剩余的窗口序列号数量
    uint64_t left_space = window_size_;
    first_out_index_ = last_out_index_next_;
  
    // 第一个并携带SYN标志的报文段
    if( r.bytes_popped() == 0 && !r.is_finished() ) {
      // 实际的序列号为 要包括SYN
      left_space --;
      m.SYN = true;
    }

    // 这部分发送的都是最大报文段
    while( left_space >= TCPConfig::MAX_PAYLOAD_SIZE - m.sequence_length() 
    && s.size() >= TCPConfig::MAX_PAYLOAD_SIZE - m.sequence_length()  ) {
      // 一个报文段的大小是剩余窗口大小，报文段最大值，外出字节流未pop量的最小值。
      uint64_t msg_size = TCPConfig::MAX_PAYLOAD_SIZE - m.sequence_length();
      left_space -= msg_size;
      s.copy(m.payload.data(),msg_size);
      m.seqno = Wrap32::wrap(last_out_index_next_,isn_);
      outstanding_segments_.push(move(m));
      transmit(outstanding_segments_.back());
      last_out_index_next_ += outstanding_segments_.back().sequence_length();
      s = s.substr(msg_size,s.size()-msg_size);
      m = {};
    }

    // 发送最后一个报文段，不是最大报文段
    uint64_t last_seg_size = min( left_space, s.size() );
    left_space -= last_seg_size;
    s.copy(m.payload.data(),last_seg_size);
    s = s.substr(last_seg_size, s.size() - last_seg_size);
    // 是否需要承载一个FIN,以及窗口是否能容纳
    if( r.is_finished() && left_space) {
        m.FIN = true;
        left_space --;
    }
    outstanding_segments_.push(move(m));
    transmit(outstanding_segments_.back());
    last_out_index_next_ += outstanding_segments_.back().sequence_length();

    r.pop(last_out_index_next_ - first_out_index_);

  }
  consecutive_++;

  if(timer_.has_started() == false) {
    timer_.start();
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return {Wrap32::wrap(first_out_index_,isn_),false,"",false,false};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if(msg.ackno) {
    if( left_ != msg.ackno.value() ) {
      timer_.reset_timeout(initial_RTO_ms_);
      timer_.start();
      consecutive_ = 0;
    }
    left_ = msg.ackno.value();
  }
  window_size_ = msg.window_size;

  // 所有未完成的数据被接收后，停止重发计时器
  if(left_ + window_size_ == Wrap32::wrap(last_out_index_next_,isn_)) {
    timer_.stop();
  }

  //窗口最大值为 2^16-1，故每次发送最大为 2^16-1,小于Wrap32 的一半:2^31，则不会发送转换错误。
  while( outstanding_segments_.size() 
  && first_out_index_ + outstanding_segments_.front().sequence_length() <= left_.unwrap(isn_,first_out_index_)){
    first_out_index_ += outstanding_segments_.front().sequence_length();
    outstanding_segments_.pop();
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if( timer_.has_started() == false ) {
    return;
  }

  timer_.add_time(ms_since_last_tick);
  if(timer_.is_expired() && outstanding_segments_.size()) {
    transmit(outstanding_segments_.front());  
  }
  
  if(window_size_ != 0) {
    consecutive_ ++;
    timer_.double_timeout();
  }

  timer_.start();
}
