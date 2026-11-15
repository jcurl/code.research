#ifndef SDD_CORE_TEST_PACKET_SAMPLES_H
#define SDD_CORE_TEST_PACKET_SAMPLES_H

#include <string_view>

namespace sdd::core::test {

/// A discovery packet as sent by a computer with two interfaces.
inline constexpr std::string_view computer_packet_json =
    R"({ "host": "rotorua", "domain": "home.lan", "interfaces": { )"
    R"("enp4s0": { "mac": "02:00:00:00:00:01", "mtu": 1500, "ipv4": [ )"
    R"("192.168.1.110/24" ] }, "lo": { "mtu": 65536, "ipv4": [ )"
    R"("127.0.0.1/8" ] } } })";

}  // namespace sdd::core::test

#endif  // SDD_CORE_TEST_PACKET_SAMPLES_H
