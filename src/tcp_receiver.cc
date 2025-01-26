#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if( (!ISN_ && !message.SYN) || (ISN_ && message.SYN) ) return;
  bool flag = false;
  if ( message.FIN ) { flag = true; }
  if ( message.SYN ) {
    ISN_ = optional<Wrap32>{message.seqno};
    absolute_index_++;
  }
  uint64_t current_index = message.seqno.unwrap(ISN_.value(),absolute_index_);
  reassembler_.insert(current_index >= 1 ? current_index - 1 : 0 ,message.payload,flag);
  absolute_index_ = reassembler_.writer().bytes_pushed()+1;
  if( message.FIN ) { absolute_index_++; }
}

TCPReceiverMessage TCPReceiver::send() const
{
  return {ISN_ ? Wrap32::wrap(absolute_index_,ISN_.value()):ISN_,
  static_cast<uint16_t>(min(writer().available_capacity() -  reassembler_.bytes_pending(), (1UL << 16) - 1)),
  false};
}
