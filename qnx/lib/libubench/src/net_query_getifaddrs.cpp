#include "config.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#if HAVE_NET_SOCKADDR_DL
#include <net/if_dl.h>
#if HAVE_NET_IFT_ETHER
#include <net/if_types.h>
#endif
#endif

#include <algorithm>
#include <map>
#include <memory>
#include <string>

#include "ubench/net.h"
#include "net_common.h"

namespace ubench::net {

namespace {
class ifaddrs_iterator {
 public:
  using value_type = ifaddrs;
  using difference_type = std::ptrdiff_t;
  using pointer = const ifaddrs*;
  using reference = const ifaddrs&;
  using iterator_category = std::input_iterator_tag;

  ifaddrs_iterator() = default;
  explicit ifaddrs_iterator(ifaddrs* ptr) : ptr_{ptr} {}

  auto operator*() const -> reference { return *ptr_; }

  auto operator->() const -> pointer { return ptr_; }

  auto operator++() -> ifaddrs_iterator& {
    if (ptr_) ptr_ = ptr_->ifa_next;
    return *this;
  }

  auto operator++(int) -> ifaddrs_iterator {
    ifaddrs_iterator temp{*this};
    if (ptr_) ptr_ = ptr_->ifa_next;
    return temp;
  }

  auto operator==(const ifaddrs_iterator& rhs) -> bool {
    return ptr_ == rhs.ptr_;
  }

  auto operator!=(const ifaddrs_iterator& rhs) -> bool {
    return ptr_ != rhs.ptr_;
  }

 private:
  ifaddrs* ptr_{};
};

class ifaddrs_container {
 public:
  ifaddrs_container() {
    ifaddrs* ifap{};
    if (getifaddrs(&ifap)) return;
    ptr_.reset(ifap);
  }

  ifaddrs_container(const ifaddrs_container&) = delete;
  auto operator=(const ifaddrs_container&) -> ifaddrs_container& = delete;
  ifaddrs_container(ifaddrs_container&&) = default;
  auto operator=(ifaddrs_container&&) -> ifaddrs_container& = default;
  ~ifaddrs_container() = default;

  using iterator = ifaddrs_iterator;
  [[nodiscard]] auto begin() const -> iterator {
    return ifaddrs_iterator{ptr_.get()};
  }

  [[nodiscard]] auto end() const -> iterator { return ifaddrs_iterator{}; }

 private:
  struct ifaddr_deleter {
    void operator()(ifaddrs* ptr) {
      if (ptr) freeifaddrs(ptr);
    }
  };

  using unique_ifaddrs_t = std::unique_ptr<ifaddrs, ifaddr_deleter>;
  unique_ifaddrs_t ptr_{};
};

}  // namespace

auto query_net_interfaces_getifaddrs(
    std::map<std::string, interface>& interfaces) -> void {
  ifaddrs_container iflist{};
  for (const auto& ifaddr : iflist) {
    const std::string name{ifaddr.ifa_name};
    auto [it, ins] = interfaces.try_emplace(name, name);
    auto& interface = it->second;

    if (ifaddr.ifa_addr == nullptr) continue;

    // We see on Linux, NetBSD, FreeBSD, QNX 7.1, QNX 8.0 that the flags are the
    // same always for the same interface. On Cygwin, the AF_INET6 is missing
    // the broadcast flag.
    interface.flags(ifaddr.ifa_flags);

    // NOLINTNEXTLINE(bugprone-switch-missing-default-case)
    switch (ifaddr.ifa_addr->sa_family) {
      case AF_INET: {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        const in_addr ipv4addr = ((sockaddr_in*)(ifaddr.ifa_addr))->sin_addr;

        // Add only if this IP address doesn't already exist.
        if (std::find_if(interface.inet().begin(), interface.inet().end(),
                [&ipv4addr](if_ipv4& ip) {
                  return ip.addr().s_addr == ipv4addr.s_addr;
                }) == interface.inet().end()) {
          auto& entry = interface.inet().emplace_back(ipv4addr);

          if (ifaddr.ifa_netmask && ifaddr.ifa_netmask->sa_family == AF_INET) {
            entry.netmask(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                ((sockaddr_in*)(ifaddr.ifa_netmask))->sin_addr);
          }

          if ((ifaddr.ifa_flags & IFF_BROADCAST) && ifaddr.ifa_broadaddr &&
              ifaddr.ifa_broadaddr->sa_family == AF_INET) {
            entry.broadcast_addr(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                ((sockaddr_in*)(ifaddr.ifa_broadaddr))->sin_addr);
          }

          if ((ifaddr.ifa_flags & IFF_POINTOPOINT) && ifaddr.ifa_dstaddr &&
              ifaddr.ifa_dstaddr->sa_family == AF_INET) {
            entry.dest_addr(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                ((sockaddr_in*)(ifaddr.ifa_dstaddr))->sin_addr);
          }
        }
        break;
      }
#if HAVE_NET_AF_LINK && HAVE_NET_SOCKADDR_DL
      // Present on QNX 7.1, QNX 8.0, NetBSD, FreeBSD
      case AF_LINK: {
        if (!interface.hw_addr()) {
          // On BSD, QNX7 we could use `satocsdl()` but that is removed in QNX8.
          sockaddr_dl* dl_addr = (struct sockaddr_dl*)(ifaddr.ifa_addr);

          // NOLINTNEXTLINE(bugprone-switch-missing-default-case)
          switch (dl_addr->sdl_type) {
#if HAVE_NET_IFT_ETHER
            case IFT_ETHER: {
              unsigned char* ptr = (unsigned char*)LLADDR(dl_addr);
              ether_addr hwaddr{};
              hwaddr.ether_addr_octet[0] = ptr[0];
              hwaddr.ether_addr_octet[1] = ptr[1];
              hwaddr.ether_addr_octet[2] = ptr[2];
              hwaddr.ether_addr_octet[3] = ptr[3];
              hwaddr.ether_addr_octet[4] = ptr[4];
              hwaddr.ether_addr_octet[5] = ptr[5];
              interface.hw_addr(hwaddr);
              break;
            }
#endif
              // case IFT_L2VLAN could be interesting on FreeBSD.
          }
        }
        break;
      }
#endif
    }
  }
}

}  // namespace ubench::net
