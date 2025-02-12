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
  if (mappings_.find(next_hop.ipv4_numeric()) != mappings_.end()) {
    // 能够在映射中找到目标以太网地址，就即刻发送报文。
    // 将数据报序列化，即统一格式化，让格式适用不同平台，便于在网络上传输。
    EthernetHeader h {mappings_.at(next_hop.ipv4_numeric()).first, ethernet_address_, EthernetHeader::TYPE_IPv4};
    transmit({h,serialize(dgram)});
  } else {
    // 找不到以太网地址
    // 1. 如果是没有相同IP,加入等待队列中，就ARP请求广播
    // 2. 如果是有相同ip，加入到队列中，原来等待时间不小于5s就继续广播，否则立即退出。
    if(datagrams_waiting_.find(next_hop.ipv4_numeric()) == datagrams_waiting_.end())
    {
      datagrams_waiting_.emplace(next_hop.ipv4_numeric(),make_pair(vector{dgram},0));
    } else {
      auto& res = datagrams_waiting_.at(next_hop.ipv4_numeric());
      res.first.push_back(dgram);
      if(res.second < 5000) { return; }
    }
    EthernetHeader h {ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP};
    ARPMessage broadcast_m { 
      .opcode = ARPMessage::OPCODE_REQUEST,
      .sender_ethernet_address = ethernet_address_,
      .sender_ip_address = ip_address_.ipv4_numeric(), 
      .target_ip_address = next_hop.ipv4_numeric()};
    transmit({h,serialize(broadcast_m)});
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // 目标地址不是该网络接口的帧应该忽略
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST) {
    return;
  }

  switch( frame.header.type )
  {
    case EthernetHeader::TYPE_IPv4:
    {
      // 解析，写入datagram_received
      InternetDatagram datagram {};
      if( parse( datagram, frame.payload) ) {
        datagrams_received_.emplace(move(datagram));
      }
      break;
    }
    case EthernetHeader::TYPE_ARP:
    {
      // 解析，并记住映射关系，如果是ARP请求还需要给予回复。
      ARPMessage arpm {};
      // 有可能是广播地址，必须要求目标地址是该网络接口的地址
      if( parse( arpm, frame.payload) && arpm.target_ip_address == ip_address_.ipv4_numeric() ) {
        if( arpm.opcode == ARPMessage::OPCODE_REQUEST ) {
          EthernetHeader h{ arpm.sender_ethernet_address , ethernet_address_, EthernetHeader::TYPE_ARP};
          ARPMessage reply{
            .opcode = ARPMessage::OPCODE_REPLY,
            .sender_ethernet_address = ethernet_address_,
            .sender_ip_address = ip_address_.ipv4_numeric(),
            .target_ethernet_address = arpm.sender_ethernet_address,
            .target_ip_address = arpm.sender_ip_address};
          transmit( {h, serialize(reply)} );
        }
        // 映射关系重新计时或者新元素
        if( mappings_.find(arpm.sender_ip_address) == mappings_.end())
        {
          // 查看 datagrams_waiting 数组，是否有可以发送的报文
          if(datagrams_waiting_.find(arpm.sender_ip_address) != datagrams_waiting_.end()) {
            EthernetHeader h{ arpm.sender_ethernet_address , ethernet_address_, EthernetHeader::TYPE_IPv4};
            for(auto& a: datagrams_waiting_.at(arpm.sender_ip_address).first) {
              transmit({h, serialize(a)});
            }
            datagrams_waiting_.erase(arpm.sender_ip_address);
          }
          mappings_.emplace(move(arpm.sender_ip_address), make_pair(move(arpm.sender_ethernet_address),0));
        } else {
          mappings_.at(arpm.sender_ip_address).second = 0;
        }
      }
      break;
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // 遍历映射关系，增加保存时间，剔除过期关系
  for( auto it = mappings_.begin(); it != mappings_.end(); ) 
  {
    it->second.second += ms_since_last_tick;
    if( it->second.second > 30 ) {
      it = mappings_.erase(it);
    } else {
      it ++;
    }
  }
  // 遍历未发送报文段, 增加等待时间
  for( auto& datagram: datagrams_waiting_)
  {
    datagram.second.second += ms_since_last_tick;
  }
}
