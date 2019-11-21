#include <generated/protocol/kademlia/protobuf/kad.pb.h>
#include <libp2p/common/logger.hpp>
#include <libp2p/common/hexutil.hpp>
#include <boost/asio.hpp>
int main() {
  auto log = libp2p::common::createLogger("log");
  log->set_pattern("%L %T.%e %v");
  log->set_level(spdlog::level::debug);
  kad::pb::Record record;
  record.set_key("xxx");
  record.set_value("yyy");
  record.set_timereceived("202020");
  uint8_t buffer[4000];
  record.SerializeToArray(buffer, 4000);
  size_t sz = record.ByteSizeLong();
  log->info("sz = {}, msg = {}", sz, libp2p::common::hex_lower({buffer, buffer + 18}));
  kad::pb::Record record2;
  log->info("parsing #1: {}", record2.ParseFromArray(buffer, sz - 10));
  log->debug("{} {} {} {}", record.key(), record2.key(), record.value(), record2.value());
  log->info("{}", record.key() == record2.key() && record.value() == record2.value());
  sz = record.ByteSizeLong();
  log->info("serializing #2: {}", record.SerializeToArray(buffer, sz));

  log->info("sizeof(boost::posix_time::ptime) = {}", sizeof(boost::posix_time::ptime));
}