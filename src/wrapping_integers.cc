#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // 计算第一个绝对序列号
  uint64_t init_offset = static_cast<uint64_t>( raw_value_ - zero_point.raw_value_ );
  // 计算checkpoint 关于 2^32 的倍数，向下取整 
  const uint64_t  k =  (1UL << 32);
  uint32_t n = checkpoint / k;
  
  uint64_t mid = n*k + init_offset;
  uint64_t res = 0;
  if ( checkpoint > (mid + mid + k)/2) { res = mid + k; }
  else if ( n && checkpoint <= (mid + mid - k)/2 ) { res = mid - k;}
  else { res = mid; }
  return res;
}
