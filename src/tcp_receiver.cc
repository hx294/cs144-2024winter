#include "tcp_receiver.hh"
#include "wrapping_integers.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.FIN ) {
    reassembler_.insert(stream_index_,message.payload,true);
  } else if ( message.SYN ) {
    ISN = message.seqno;
    stream_index_ = ISN.unwrap(ISN,stream_index_); // ISN == stream_index
    reassembler_.insert(stream_index_,message.payload,false);
  } else {
    reassembler_.insert(stream_index_,message.payload,false);
  }
  stream_index_ += message.payload.size();
}

TCPReceiverMessage TCPReceiver::send() const
{
  return {Wrap32::wrap(stream_index_,ISN),
  static_cast<uint16_t>(writer().available_capacity() -  reassembler_.bytes_pending()),
  false};
}
