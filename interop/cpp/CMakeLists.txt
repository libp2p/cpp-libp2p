
add_executable(interop main.cpp interop.pb.cc)
target_link_libraries(interop
  Boost::boost
  protobuf::libprotobuf
  p2p_basic_host
  p2p_default_network
  p2p_dnsaddr_resolver
  p2p_gossip
  p2p_identify
  p2p_inmem_address_repository
  p2p_inmem_key_repository
  p2p_inmem_protocol_repository
  p2p_kademlia
  p2p_peer_repository
)
set_target_properties(interop PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
