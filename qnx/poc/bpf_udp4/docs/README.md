# Design for Memory Based UDP over IPv4 sender <!-- omit in toc -->

The goal is to implement IPv4 UDP packets without using a BSD socket API. Such a
mechanism is useful for very small and lightweight network stacks that do not
require anything than IPv4 UDP packets (fragmented or not).

This Proof-of-Concept code covers:

- A C++ interface that can be used for BSD-like socket API or raw sockets
  - For example, sending data on Windows (using Cygwin) or Linux without RAW
    sockets; or
  - A raw socket API, like the BPF interface on BSD like unix implementations,
    such as FreeBSD, NetBSD or QNX.
- Sample code that helps show the algorithms needed to create a full IPv4 packet
  and sending it over a BPF interface. Such code could be adapted to, instead of
  sending to `/dev/bpf` or so, write to shared memory or another interface and
  another component (such as a network driver) for sending data directly.

Other example code in this repository (`udp_load`) shows the performance of
writing raw sockets is surprisingly not much worse than using the BSD socket API
directly.

- [1. The C++ Socket API](#1-the-c-socket-api)
- [2. Implementation Variants](#2-implementation-variants)
  - [2.1. Using the BSD Socket API](#21-using-the-bsd-socket-api)
  - [2.2. Using the BPF Socket API](#22-using-the-bpf-socket-api)
  - [2.3. Memory based Socket API](#23-memory-based-socket-api)
- [3. Testing](#3-testing)
  - [3.1. QNX 7.1](#31-qnx-71)
  - [3.2. QNX 8.0](#32-qnx-80)
- [4. Appendix](#4-appendix)
  - [4.1. Alignment on Fragmentation](#41-alignment-on-fragmentation)
  - [4.2. Minimum Size of an Ethernet Packet](#42-minimum-size-of-an-ethernet-packet)

## 1. The C++ Socket API

We want to have an API that mimics BSD-like Socket API, that can be implemented
using mechanisms native to the OS on hand.

```cpp
class udp_socket {
 public:
  udp_socket() = default;
  udp_socket(const udp_socket&) = delete;
  auto operator=(const udp_socket&) -> udp_socket& = delete;
  udp_socket(udp_socket&&) = default;
  auto operator=(udp_socket&&) -> udp_socket& = default;
  virtual ~udp_socket() = default;

  virtual operator bool() const noexcept = 0;
  virtual auto send(const std::vector<std::byte>& data) noexcept -> std::expected<void, int> = 0;
};

class bsd_socket : public udp_socket {
 public:
  bsd_socket() = delete;
  bsd_socket(const sockaddr_in& src, const sockaddr_in& dest);
  bsd_socket(const bsd_socket&) = delete;
  auto operator=(const bsd_socket&) -> bsd_socket& = delete;
  bsd_socket(bsd_socket&&) = default;
  auto operator=(bsd_socket&&) -> bsd_socket& = default;
  ~bsd_socket() override = default;

  operator bool() const noexcept override;
  auto send(const std::vector<std::byte>& data) noexcept -> std::expected<void, int> override;
};

class bpf_socket : public udp_socket {
 public:
  bpf_socket() = delete;
  bpf_socket(const sockaddr_in& src, const sockaddr_in& dest);
  bpf_socket(const bpf_socket&) = delete;
  auto operator=(const bpf_socket&) -> bpf_socket& = delete;
  bpf_socket(bpf_socket&&) = default;
  auto operator=(bpf_socket&&) -> bpf_socket& = default;
  ~bpf_socket() override = default;

  operator bool() const noexcept override;
  auto send(const std::vector<std::byte>& data) noexcept -> std::expected<void, int> override;
};
```

One could also define if wanted a `bsd_winsock` implementation that uses Windows
API, or a `mem_socket` for writing to shared memory regions (given that the
mechanism for writing to shared memory, and the notification of another
component that there is data available is separately provided).

## 2. Implementation Variants

### 2.1. Using the BSD Socket API

Using the BSD socket API uses the complete network stack provided by the
Operating System. That implies all standards needed for address resolution,
routing tables, etc. are available. The identification of the interfaces
depending on the address being bound is automatically selected. Firewall filters
are applied on incoming and outgoing traffic.

Sending out a packet that is greater than the configured MTU will cause the
network stack to fragment the packet.

This is typically how a user sends traffic.

It sets the TTL to one, but doesn't set any special options other than to enable
sending to multicast (and enabling in a loopback mode that other processes on
the machine can listen to the messages). Large messages are likely to fail.

### 2.2. Using the BPF Socket API

BPF sockets are usually only available for privileged users. Sending data via
BPF evades the firewall, does not support address resolution, putting the onus
of the implementation on the user. It also does not implement details on
hardware acceleration of checksums on the packet.

In this regard, a high performance implementation would actually choose to use
BSD socket API where-ever possible, and then using a BPF like interface when not
available.

### 2.3. Memory based Socket API

Not implemented in this PoC, but one could write the complete L2 Ethernet packet
(as demonstrated as part of the BPF socket API) and then adapt the code to
notify a separate process that has access to shared memory and then send the
data.

## 3. Testing

### 3.1. QNX 7.1

Testing on QNX 7.1, on a Raspberry Pi4B (genet0), using raw and VLANs.

The default interface is `genet0` and has an MTU of 1500, allowing a size
without fragmentation of 1472 bytes.

```sh
# bpf_udp4 -S192.168.1.109:3500 -D239.0.0.1:3500 -Bbpf -s1472
```

Create the VLAN interface with:

```sh
# ifconfig vlan73 create vlan 73 vlanif genet0
# ifconfig vlan73 inet 10.0.0.1/24 up
```

Interestingly, on QNX 7.1 using `io-pkt`, the MTU of the VLAN interface `vlan73`
is 1496. This can also conveniently test the alignment requirements for
fragmentation.

```sh
# bpf_udp4 -S10.0.0.1:3500 -D239.0.0.1:3500 -Bbpf -s1468
```

causes a single packet of the correct size sent out in a single Ethernet packet.
On the interface with the VLAN, the VLAN header is added automatically by the
Operating System, using the default VLAN priority field (usually 0).

Sending out a packet of 1472 bytes results in two packets, the first of 1510
bytes (4 bytes not used due to alignment), and the second packet of 68 bytes
(the extra 4 bytes due to the VLAN header, and appears QNX unnecessarily adds
more padding of 4 bytes than is needed).

When using BPF over the VLAN interface, the PCP field is not affected by the
command:

```c
setsockopt(fd, SOL_SOCKET, SO_VLANPRIO, &vlan, sizeof(vlan))
```

There is no specific command on QNX 7.1 to set the VLAN for the BPF interface at
a packet level. One could modify the software to provide the 802.1q header
explicitly, but then the MTU is effectively reduced by 4 bytes (an MTU of 1500
is reduced when the 802.1q header is applied).

### 3.2. QNX 8.0

Testing on QNX 8.0, on a Raspberry Pi4B (genet0), using raw and VLANs.

```sh
# ifconfig vlan73 create vlan 73 vlandev genet0
# ifconfig vlan73 inet 10.0.0.2/24 up
```

QNX 8.0 using `io-sock`, the MTU of the main interface `genet0`
is 1500, and for the VLAN interface `vlan73` is also 1500.

```sh
# bpf_udp4 -S10.0.0.2:3500 -D239.0.0.1:3500 -Bbpf -s1472
# bpf_udp4 -S192.168.1.110:3500 -D239.0.0.1:3500 -Bbpf -s1472
```

QNX8 uses the FreeBSD network stack, is the PCP field can be set with:

```c
auto bpf_vlan(int fd, uint8_t vlan) -> stdext::expected<void, int> {
  if (fd < 0) return stdext::unexpected{EINVAL};

  unsigned int vlanpcp = vlan;
  if (ioctl(fd, BIOCSETVLANPCP, &vlanpcp, sizeof(vlanpcp)) == -1)
    return stdext::unexpected{errno};
  return {};
}
```

One must be sure to enable the option with (see the
[change](https://reviews.freebsd.org/D31263)):

```sh
# sysctl net.link.vlan.mtag_pcp=1
net.link.vlan.mtag_pcp: 0 -> 1
```

## 4. Appendix

### 4.1. Alignment on Fragmentation

When studying the header for IPv4, the fragmentation offset is 13 bits and is
the number of 8-bit octets offset. What this implies, when testing
fragmentation, the length of packets that are fragmented need to be divisible by
8.

With an MTU of 1500, the payload of an IPv4 packet is 1480 bytes (a UDP payload
of 1472 bytes) that is divisible by 8. With an MTU of 1496, the payload is now
1476 bytes (a UDP payload of 1468) which is no longer divisible by 8.

### 4.2. Minimum Size of an Ethernet Packet

The Ethernet standard specifies a minimum packet size of 64 bytes. Using the
IPv4 stack, as well as BPF stack on FreeBSD, NetBSD and QNX, the sizes of the
Ethernet frame is increased automatically to 64 bytes (the Ethernet header, IPv4
header, UDP header and then the payload). This situation occurs for a payload
that is less than 14 bytes.
