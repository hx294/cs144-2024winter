#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reader().set_error();
    return;
  }
  // 在syn之前的消息忽略，重复的syn忽略。
  if ( ( !ISN_ && !message.SYN ) || ( ISN_ && message.SYN ) )
    return;
  bool flag = false;
  if ( message.FIN ) {
    flag = true;
  }
  if ( message.SYN ) {
    ISN_ = optional<Wrap32> { message.seqno };
    absolute_index_++;
    // fin 后面可能带有消息
    message.seqno = message.seqno + 1;
  }
  uint64_t current_index = message.seqno.unwrap( ISN_.value(), absolute_index_ );
  // 不能出现使用syn序列号的其他报文
  if ( message.seqno == ISN_.value() || current_index + message.payload.size() < absolute_index_ )
    return;
  reassembler_.insert( current_index - 1, message.payload, flag );
  absolute_index_ = writer().bytes_pushed() + 1;
  // 消息可能失序，所以不能用fin判断结束
  if ( reassembler_.writer().is_closed() ) {
    absolute_index_++;
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  return {
    ISN_ ? Wrap32::wrap( absolute_index_, ISN_.value() ) : ISN_,
    static_cast<uint16_t>( min( writer().available_capacity(), ( 1UL << 16 ) - 1 ) ), // 空间可能超过变量表示范围。
    writer().has_error() };
}
