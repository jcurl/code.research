#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>

#include <cerrno>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "ubench/net.h"

#include "config.h"

namespace ubench::net {

namespace {
struct ifaddr_deleter {
  void operator()(ifaddrs* ptr) {
    if (ptr) freeifaddrs(ptr);
  }
};

auto make_ifaddrs(ifaddrs* addr) -> std::shared_ptr<ifaddrs> {
  return std::shared_ptr<ifaddrs>(addr, ifaddr_deleter{});
}

class ifaddr_ptr final {
 public:
  ifaddr_ptr() noexcept {
    if (getifaddrs(&addr_)) {
      errno_ = errno;
    }

    // This allows us to keep the lifetime of the object alive for as long as
    // needed.
    root_ = make_ifaddrs(addr_);
  }

  ifaddr_ptr(const ifaddr_ptr& other, ifaddrs* next)
      : errno_{other.errno_}, root_{other.root_}, addr_{next} {}

  ifaddr_ptr(const ifaddr_ptr& other) = default;
  auto operator=(const ifaddr_ptr& other) -> ifaddr_ptr& = default;
  ifaddr_ptr(ifaddr_ptr&& other) = default;
  auto operator=(ifaddr_ptr&& other) -> ifaddr_ptr& = default;

  [[nodiscard]] auto os_errno() const -> int { return errno_; }

  [[nodiscard]] auto operator->() const -> ifaddrs* { return addr_; }

  [[nodiscard]] explicit operator bool() const { return addr_ != nullptr; }

  /// @brief Get the next pointer in the linked list.
  ///
  /// Use this method instead of dereferencing the field this->ifa_next, so that
  /// reference counting of the head node is made for automatic deletion.
  /// Usually, the current pointer is updated with the pointer to the next, that
  /// if there was no reference counting, it would be deleted (when the old
  /// variable is overwritten) rendering the lifetime of the next pointer now
  /// invalid.
  ///
  /// An alternative would be to implement an iterator.
  ///
  /// @return a new ifaddr_ptr with a reference count increment, and the next
  /// element.
  [[nodiscard]] auto next() const -> ifaddr_ptr {
    return ifaddr_ptr{*this, addr_->ifa_next};
  }

  ~ifaddr_ptr() = default;

 private:
  int errno_{0};
  std::shared_ptr<ifaddrs> root_{};
  ifaddrs* addr_{};
};

}  // namespace

auto query_net_interfaces_getifaddrs(std::function<bool(
        std::string, unsigned int, sockaddr*, sockaddr*, sockaddr*, sockaddr*)>
        cb) -> void {
  ifaddr_ptr ifaddr{};
  if (!ifaddr) return;

  while (ifaddr) {
    std::string name{ifaddr->ifa_name};

    std::cout << "Name: " << name << std::endl;
    std::cout << " Flags: " << std::hex << ifaddr->ifa_flags << std::dec
              << std::endl;

    if (ifaddr->ifa_addr && ifaddr->ifa_addr->sa_family == AF_INET) {
      in_addr ipv4addr = ((sockaddr_in*)(ifaddr->ifa_addr))->sin_addr;
      std::cout << " IPv4: " << ipv4addr << std::endl;
    }
    if (ifaddr->ifa_netmask && ifaddr->ifa_netmask->sa_family == AF_INET) {
      in_addr ipv4addr = ((sockaddr_in*)(ifaddr->ifa_netmask))->sin_addr;
      std::cout << " NetMask: " << ipv4addr << std::endl;
    }

#if HAVE_NET_IFF_BROADCAST
    if ((ifaddr->ifa_flags & IFF_BROADCAST) && ifaddr->ifa_broadaddr &&
        ifaddr->ifa_broadaddr->sa_family == AF_INET) {
      in_addr ipv4addr = ((sockaddr_in*)(ifaddr->ifa_broadaddr))->sin_addr;
      std::cout << " Broadcast: " << ipv4addr << std::endl;
    }
#endif

#if HAVE_NET_IFF_POINTOPOINT
    if ((ifaddr->ifa_flags & IFF_POINTOPOINT) && ifaddr->ifa_dstaddr &&
        ifaddr->ifa_dstaddr->sa_family == AF_INET) {
      in_addr ipv4addr = ((sockaddr_in*)(ifaddr->ifa_dstaddr))->sin_addr;
      std::cout << " Dest: " << ipv4addr << std::endl;
    }
#endif

    bool update = false;
    bool result;
#if HAVE_NET_IFF_BROADCAST
    if (!update && ifaddr->ifa_flags & IFF_BROADCAST) {
      result = cb(name, ifaddr->ifa_flags, ifaddr->ifa_addr,
          ifaddr->ifa_netmask, ifaddr->ifa_broadaddr, nullptr);
      update = true;
    }
#endif
#if HAVE_NET_IFF_POINTOPOINT
    if (!update && ifaddr->ifa_flags & IFF_POINTOPOINT) {
      result = cb(name, ifaddr->ifa_flags, ifaddr->ifa_addr,
          ifaddr->ifa_netmask, nullptr, ifaddr->ifa_dstaddr);
      update = true;
    }
#endif
    if (!update) {
      result = cb(name, ifaddr->ifa_flags, ifaddr->ifa_addr,
          ifaddr->ifa_netmask, nullptr, nullptr);
    }

    if (!result) return;
    ifaddr = ifaddr.next();
  }
}

}  // namespace ubench::net