#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class RetransmissionTimer {

  public:
    explicit RetransmissionTimer( uint64_t t ) : RTO_{ t } 
      {}
    void double_timeout() { RTO_ *= 2; }
    void reset_timeout( uint64_t init ) { RTO_ = init; }
    void start() { passed_time_ = 0; }
    void stop() { passed_time_ = 0; }

    bool is_expired() const { return passed_time_ >= RTO_; }
    
    void add_time( const uint64_t t) { passed_time_ += t; }

  private:
    // Retransmission Timeout
    uint64_t RTO_;
    // the time passed 
    uint64_t passed_time_ {};
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), timer_{initial_RTO_ms}, first_out_index_{isn.unwrap(Wrap32(0),0)}
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  // the number of consective retransmissions 
  uint64_t consecutive_ {};
  // the number of outstanding bytes
  uint64_t outstanding_bytes_ {};
  // recevier window's left or right edge, 左闭右开
  Wrap32 left_ {0};
  Wrap32 right_ {0};
  // RetransmissionTimer initialized in constructor
  RetransmissionTimer timer_ ;

  // outstanding segments
  std::queue<TCPSenderMessage> outstanding_segments_ {};
  // first outstanding index
  uint64_t first_out_index_ ;
};

