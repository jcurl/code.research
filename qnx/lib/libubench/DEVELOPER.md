# Developer Information for libubench <!-- omit in toc -->

Documentation is maintained in the source header files directly. As such, this
document file does not repeat how to use the library.

- [1. Operating System Functionality](#1-operating-system-functionality)
  - [1.1. File Handles (`file.h`)](#11-file-handles-fileh)
  - [1.2. Buffers (`os.h`)](#12-buffers-osh)
  - [1.3. CPU Times (`clock.h`)](#13-cpu-times-clockh)
  - [1.4. Process Name for a PID (`os.h`)](#14-process-name-for-a-pid-osh)
  - [1.5. Thread Handling (`thread.h`)](#15-thread-handling-threadh)
  - [1.6. Networking (`net.h`)](#16-networking-neth)
- [2. Common Functionality](#2-common-functionality)
  - [2.1. String Parsing (arguments `string.h`)](#21-string-parsing-arguments-stringh)
- [3. Useful Information](#3-useful-information)
  - [3.1. Networking](#31-networking)
    - [3.1.1. Interfaces](#311-interfaces)
      - [3.1.1.1. Adding an IPv4 Address (IP aliasing)](#3111-adding-an-ipv4-address-ip-aliasing)
      - [3.1.1.2. Creating a new VLAN](#3112-creating-a-new-vlan)
      - [3.1.1.3. Flags](#3113-flags)

## 1. Operating System Functionality

### 1.1. File Handles (`file.h`)

Using file handles directly in C++ code poses the problem for unexpected leaks
of file handles. The `ubench::file::fdesc` class allows assignment of a file
handle, so that the lifetime of the handle itself is managed by C++. In
particular, in the case of an unexpected exception, the RAII model allows the
file handle to be closed, preventing a possible leak.

### 1.2. Buffers (`os.h`)

A common idiom in C is to have a structure of the form, often defined in the
Operating System headers itself.

```c
struct procfs_debuginfo {
  uint64_t vaddr;
  char path[1];
}
```

It's usage is common to have userspace code provide sufficient buffer, either by
using `malloc()`, or defining a new `struct` and allocating on the stack.

```c
struct {
  procfs_debuginfo info;
  char buff[1024];
} map;
```

The variable `map` now has sufficient space, that the Operating System can
safely write to the user provided memory.

C++ will perform an optimisation, especially with strings, and could possibly
cause unexpected behaviour in the following case:

```c
printf("%s %d\n", map.info.path, strlen(map.info.path));
```

The result might print:

```text
/boot/file 0
```

This is saying that the string, which is clearly printed as having length 10
characters (and thus using 11 bytes of memory) is reported by `strlen` however
to be zero bytes.

Reproduced in Godbolt for the ARM64 GCC 8.x compiler, the `strlen` is completely
optimised away and replaced with the value zero.

An explanation of why:

- The `map.info.path` decays to a pointer, and the Operating System library only
  receive a point to the array. The memory contents are shown to be correct.
- The `strlen(map.info.path)` refers to the field `char path[1];`. If a compiler
  assumes that there is no undefined behaviour, it then assumes that the string
  must be terminated with an ASCII NUL (\0), and therefore the string length of
  a 1 byte `char` must be zero bytes in length.
  - Compiling this without optimisations would result in an expected value.
  - This shows the usage of undefined behaviour.

The class `ubench::os::osbuff` works around the undefined behaviour when
interoperating with Operating System headers in C++ code:

```c++
using procfs_debuginfo_tx = osbuff<procfs_debuginfo, 1024>;
procfs_debuginfo_tx map{};
devctl(fd, DCMD_PROC_MAPDEBUG_BASE, &map(), sizeof(map), nullptr);
return std::string{map().path};
```

This creates a type `procfs_debuginfo_tx` that reserves an array of 1024 bytes.
It is then allocated on the stack. Accessing within the fields works as
expected.

### 1.3. CPU Times (`clock.h`)

Obtain the total idle clock time of the CPU since boot, and the total CPU time
of the current process since it is started.

The CPU time of the current process is standard in Posix for Linux, Cygwin,
NetBSD, FreeBSD, QNX.

The CPU idle time is complicated, each Operating System provides its own
mechanisms.

- Linux through the `/proc/cpu` virtual file
- QNX through the times of PID 1 and a thread representing the idle time of a
  specific core.
- FreeBSD, NetBSD through the `sysctl()` interface
- Cygwin through Windows specific API

### 1.4. Process Name for a PID (`os.h`)

Each Operating System provides their own implementation specific way of getting
the process name of each PID.

- Linux: Queries `/proc/<pid>/cmdline`.
- QNX: Returns the name of the process only, but if `/proc/<pid>/as` is not
  available, uses `/proc/<pid>/cmdline` which is the name as invocated on the
  command line.
- NetBSD: Queries `/proc/<pid>/cmdline`.
- FreeBSD: Returns the name of the process only via `sysctl`, abstracted by
  `libutil`.
- Cygwin: Queries `/proc/<pid>/cmdline`. Cross-process queries generally not
  available.

### 1.5. Thread Handling (`thread.h`)

Pin the current thread to a core (as represented by the Operating System).

- Linux, NetBSD, FreeBSD, Cygwin uses `pthread_setaffinity_np`.
- QNX uses Kernel services `ThreadCtl`.

### 1.6. Networking (`net.h`)

Query for a list of all interfaces. It is not as general as the services
provided by Operating Systems, but provides the most common methods for 802.3
(Ethernet) and Wireless that has identical properties of wired Ethernet.

- Interface name, and if supported on the OS, an alias.
  - Cygwin is the only system that provides an alias, to map a GUID to a human
    readable name.
- IPv4 addresses. An interface may have one or more IPv4 address.
  - IPv4 address.
  - Broadcast address (for shared medium, like Ethernet).
  - Destination address (for point-to-point, like PPP).
  - Network mask.
- VLAN associated with the interface.
  - VLAN identifier.
  - Parent interface.
  - No DEI or PCP field is provided, as this is generally part of how an
    application wants to prioritise their packet when they create the socket,
    and is not fixed for the interface.
- Ethernet MAC address.
- Status.

The Ethernet MAC address is not available on all Operating Systems. It is
abstracted by this library.

Other useful functionality:

- Check if an IPv4 address is multicast.
- Convert an IPv4 multicast address to a multi-cast MAC address.
- `ostream` overloads for writing IPv4 (`sockaddr_in` or `in_addr`).
- `ostream` overload for writing Ethernet.

## 2. Common Functionality

### 2.1. String Parsing (arguments `string.h`)

Argument parsing often needs to read strings.

- Convert a comma separated list of elements into a vector (`split_args`).
- Parse a string into an integer (`parse_int`).
- A backup implementation of `strlcpy` if not found by `CMakeLists.txt`.

## 3. Useful Information

This section covers only links and quick information, important while testing
the library and seeing how it functions.

### 3.1. Networking

#### 3.1.1. Interfaces

Some Operating Systems only allow one IPv4 address per interface. Creating a
second IPv4 address results in multiple interfaces. Others support multiple IPv4
addresses per interface.

##### 3.1.1.1. Adding an IPv4 Address (IP aliasing)

Linux allows adding a new IPv4 address to existing interface (`enp108s0`). Note,
that `ifconfig` does not show aliases, but `ip a` will.

```sh
ip addr add 10.0.0.1/24 dev enp108s0  # Add
ip addr del 10.0.0.1/24 dev enp108s0  # Remove
```

Linux allows creating aliases with new interfaces from an existing interface
(`enp108s0` with `enp108s0:0`):

```sh
ifconfig enp108s0:0 10.0.0.1 netmask 255.255.255.0  # Add
ifconfig enp108s0:0 down                            # Remove
```

FreeBSD, NetBSD, QNX 7.1, QNX 8 aliases on existing interface (`genet0`)

```sh
ifconfig genet0 inet 10.0.1.1/24 alias   # Add
ifconfig genet0 inet 10.0.1.1/24 -alias  # Remove
```

Cygwin always represents a new interface, appended with `:N` for aliases.

##### 3.1.1.2. Creating a new VLAN

Linux:

```sh
ip link add link enp108s0 name enp108s0.100 type vlan id 100
ip addr add 10.0.3.1/24 dev enp108s0.100
```

NetBSD and QNX7.1:

```sh
ifconfig vlan73 create vlan 73 vlanif genet0
```

FreeBSD and QNX8: (`genet0` is the parent interface)

```sh
ifconfig vlan73 create vlan 73 vlandev genet0
```

Cygwin doesn't support an interface to get the VLAN. VLANs are assigned in the
driver settings under Windows.

##### 3.1.1.3. Flags

Each Operating System maintains its own sets of flags.

| Flag              | Linux 6.8             | Cygwin 3.5 | QNX 7.1 (io-pkt) | QNX 8      | NetBSD 10.1 | FreeBSD 14.2 |
| ----------------- | --------------------- | ---------- | ---------------- | ---------- | ----------- | ------------ |
| IFF_UP            | 0x00001               | 0x00001    | 0x00000001       | 0x00000001 | 0x0001      | 0x00000001   |
| IFF_BROADCAST     | 0x00002               | 0x00002    | 0x00000002       | 0x00000002 | 0x0002      | 0x00000002   |
| IFF_DEBUG         | 0x00004               |            | 0x00000004       | 0x00000004 | 0x0004      | 0x00000004   |
| IFF_LOOPBACK      | 0x00008               | 0x00008    | 0x00000008       | 0x00000008 | 0x0008      | 0x00000008   |
| IFF_POINTOPOINT   | 0x00010               | 0x00010    | 0x00000010       | 0x00000010 | 0x0010      | 0x00000010   |
| IFF_NOTRAILERS    | 0x00020               | 0x00020    | 0x00000020       |            |             |              |
| IFF_KNOWSEPOCH    |                       |            |                  | 0x00000020 |             |              |
| IFF_UNNUMBERED    |                       |            |                  |            | 0x0020      |              |
| IFF_NEEDSEPOCH    |                       |            |                  |            |             | 0x00000020   |
| IFF_RUNNING       | 0x00040               | 0x00040    | 0x00000040       | 0x00000040 | 0x0040      | 0x00000040   |
| IFF_DRV_RUNNING   |                       |            |                  | 0x00000040 |             | 0x00000040   |
| IFF_NOARP         | 0x00080               | 0x00080    | 0x00000080       | 0x00000080 | 0x0080      | 0x00000080   |
| IFF_PROMISC       | 0x00100               | 0x00100    | 0x00000100       | 0x00000100 | 0x0100      | 0x00000100   |
| IFF_ALLMULTI      | 0x00200 Not supported |            | 0x00000200       | 0x00000200 | 0x0200      | 0x00000200   |
| IFF_MASTER        | 0x00400               |            |                  |            |             |              |
| IFF_SLAVE         | 0x00800               |            |                  |            |             |              |
| IFF_OACTIVE       |                       |            | 0x00000400       | 0x00000400 | 0x0400      | 0x00000400   |
| IFF_DRV_OACTIVE   |                       |            |                  | 0x00000400 |             | 0x00000400   |
| IFF_SIMPLEX       |                       |            | 0x00000800       | 0x00000800 | 0x0800      | 0x00000800   |
| IFF_MULTICAST     | 0x01000               | 0x01000    | 0x00008000       | 0x00008000 | 0x8000      | 0x00008000   |
| IFF_PORTSEL       | 0x02000               |            |                  |            |             |              |
| IFF_AUTOMEDIA     | 0x04000               |            |                  |            |             |              |
| IFF_DYNAMIC       | 0x08000               |            |                  |            |             |              |
| IFF_LINK0         |                       |            | 0x00001000       | 0x00001000 | 0x1000      | 0x00001000   |
| IFF_LINK1         |                       |            | 0x00002000       | 0x00002000 | 0x2000      | 0x00002000   |
| IFF_LINK2         |                       |            | 0x00004000       | 0x00004000 | 0x4000      | 0x00004000   |
| IFF_NAT_T         |                       |            |                  |            | 0x1000      |              |
| IFF_ECN           |                       |            |                  |            | 0x2000      |              |
| IFF_ALTPHYS       |                       |            |                  | 0x00004000 |             | 0x00004000   |
| IFF_FWD_IPV6      |                       |            |                  |            | 0x4000      |              |
| IFF_LOWER_UP      | 0x10000 linux/if.h    | 0x10000    |                  |            |             |              |
| IFF_CANTCONFIG    |                       |            |                  | 0x00010000 |             | 0x00010000   |
| IFF_DORMANT       | 0x20000 linux/if.h    | 0x20000    |                  |            |             |              |
| IFF_PPROMISC      |                       |            |                  | 0x00020000 |             | 0x00020000   |
| IFF_ECHO          | 0x40000 linux/if.h    |            |                  |            |             |              |
| IFF_MONITOR       |                       |            |                  | 0x00040000 |             | 0x00040000   |
| IFF_STATICARP     |                       |            |                  | 0x00080000 |             | 0x00080000   |
| IFF_STICKYARP     |                       |            |                  |            |             | 0x00100000   |
| IFF_DYING         |                       |            |                  | 0x00200000 |             | 0x00200000   |
| IFF_RENAMING      |                       |            |                  | 0x00400000 |             | 0x00400000   |
| IFF_NOGROUP       |                       |            |                  | 0x00800000 |             |              |
| IFF_PALLMULTI     |                       |            |                  |            |             | 0x00800000   |
| IFF_NETLINK_1     |                       |            |                  | 0x01000000 |             | 0x01000000   |
| IFF_VIRTIO        |                       |            | 0x10000000       |            |             |              |
| IFF_BRIDGELRO     |                       |            |                  | 0x10000000 |             |              |
| IFF_IP6FORWARDING |                       |            | 0x20000000       |            |             |              |
| IFF_VLANMULTI     |                       |            |                  | 0x20000000 |             |              |
| IFF_ACCEPTRTADV   |                       |            | 0x40000000       |            |             |              |
| IFF_SHIM          |                       |            | 0x80000000       |            |             |              |
