#include <iostream>
#include <libp2p/multi/multiaddress.hpp>

using namespace libp2p;

int main() {
  auto addr = multi::Multiaddress::create("/ip4/192.168.0.1/tcp/8080");
  std::cout << "address: " << addr.value().toString() << std::endl;
  return 0;
}
