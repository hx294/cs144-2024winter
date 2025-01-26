#include "tcp_receiver.hh"
#include "wrapping_integers.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  bool flag = false;
  if ( message.FIN ) { flag = true; finished_ = true;}
  if ( message.SYN ) {
    ISN_ = optional<Wrap32>{message.seqno};
    stream_index_ ++;
  }
  reassembler_.insert(stream_index_,message.payload,flag);
  stream_index_ += message.payload.size();
  if( message.FIN ) { stream_index_ ++;}
}

TCPReceiverMessage TCPReceiver::send() const
{
  return {ISN_ ? Wrap32::wrap(stream_index_,ISN_.value()):ISN_,
  static_cast<uint16_t>(writer().available_capacity() -  reassembler_.bytes_pending()),
  finished_};
}
