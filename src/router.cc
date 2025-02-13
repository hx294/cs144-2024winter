#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  routing_table_.emplace_back(
    move( route_prefix ), move( prefix_length ), move( next_hop ), move( interface_num ) );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // 遍历每个网路接口
  for ( auto& network_interface : _interfaces ) {
    auto& datagrams = network_interface->datagrams_received();
    // 遍历每个数据报, 寻找最匹配的路由
    while ( !datagrams.empty() ) {
      InternetDatagram& datagram = datagrams.front();
      // TTL == 0 或 1的报文丢弃, 其他的进行最长前缀匹配，找到需要路由的接口。
      if ( datagram.header.ttl != 0 && --datagram.header.ttl != 0 ) {
        // 如果interface_num 为空， 说明数据报没有匹配到网段,直接丢弃
        // 如果next_hop 为空，说明数据报到达目标网络，否则前往下一跳。
        auto [interface_num, next_hop] = longest_prefix_match( datagram.header.dst );
        if ( next_hop && interface_num ) {
          interface( interface_num.value() )->send_datagram( datagram, next_hop.value() );
        } else if ( interface_num ) {
          interface( interface_num.value() )
            ->send_datagram( datagram, Address::from_ipv4_numeric( datagram.header.dst ) );
        }
      }
      datagrams.pop();
    }
  }
}

pair<optional<size_t>, optional<Address>> Router::longest_prefix_match( const uint32_t& dst ) const
{
  std::vector<shared_ptr<route_>> prefix_match;
  // 遍历路由表，找到所有前缀匹配路由
  for ( const auto& route : routing_table_ ) {
    // 特例：如果路由prefix_len是0，那么直接匹配。不能对usigned int进行右移32位，将产生未定义行为。
    uint32_t route_prefix = route.route_prefix;
    uint32_t dst_prefix = dst;
    // 通过右移而忽略prefix_len 以后的比特
    if ( route.prefix_length > 0 ) {
      route_prefix >>= ( 32 - route.prefix_length );
      dst_prefix >>= ( 32 - route.prefix_length );
    } else {
      route_prefix = dst_prefix = 0;
    }
    if ( route_prefix == dst_prefix ) {
      prefix_match.push_back( make_shared<route_>( route ) );
    }
  }

  // 遍历已匹配的路由，找到prefix_len 值最大的路由
  uint8_t max_prefix = 0;
  shared_ptr<route_> longest_prefix;
  for ( auto& route : prefix_match ) {
    if ( route->prefix_length >= max_prefix ) {
      max_prefix = route->prefix_length;
      longest_prefix = route;
    }
  }

  std::pair<optional<size_t>, optional<Address>> res;
  if ( longest_prefix != nullptr ) {
    res.first = optional<size_t>( longest_prefix->interface_num );
    res.second = optional<Address>( longest_prefix->next_hop );
  }

  return res;
}
