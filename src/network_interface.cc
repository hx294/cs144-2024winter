#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetHeader h {mappings_.at(next_hop).first, ethernet_address_, EthernetHeader::TYPE_IPv4};
  Serializer s {};
  if (mappings_.find(next_hop) != mappings_.end()) {
    // 能够在映射中找到目标以太网地址，就即刻发送报文。
    // 将数据报序列化，即统一格式化，让格式适用不同平台，便于在网络上传输。
    dgram.serialize(s);
    transmit({h,s.output()});
  } else {
    // 找不到以太网地址
    // 1. 如果是没有相同IP,加入等待队列中，就ARP请求广播
    // 2. 如果是有相同ip，加入到队列中，原来等待时间不小于5s就继续广播，否则立即退出。
    if(datagrams_waiting_.find(next_hop) == datagrams_waiting_.end())
    {
      datagrams_waiting_.emplace(next_hop,make_pair(vector{dgram},0));
    } else {
      auto& res = datagrams_waiting_.at(next_hop);
      res.first.push_back(dgram);
      if(res.second < 5000) { return; }
    }
    
    ARPMessage broadcast_m { 
      .opcode = ARPMessage::OPCODE_REQUEST,
      .sender_ethernet_address = ethernet_address_,
      .sender_ip_address = ip_address_.ipv4_numeric(), 
      .target_ethernet_address = ETHERNET_BROADCAST, 
      .target_ip_address = next_hop.ipv4_numeric()};
    broadcast_m.serialize(s);
    transmit({h,s.output()});
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  (void)frame;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  (void)ms_since_last_tick;
}
