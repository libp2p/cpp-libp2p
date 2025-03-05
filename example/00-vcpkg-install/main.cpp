#include <iostream>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>

int main() {
  auto ma_res = libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/8080");
  if (ma_res.has_value()) {
    std::cout << "Created multiaddress: "
              << ma_res.value().getStringAddress() << std::endl;
  }
  return 0;
}