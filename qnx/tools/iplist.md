# IP List on Interfaces

This is a useful test of the `ubench::net` interface, that is able to obtain all
interfaces for various operating systems and give information such as:

- Interface Name
- MAC address
- MTU
- VLAN identifier and parent interface
- Flags

The following usage shows the case when there are multiple interfaces, some
running, with VLANs, and interfaces having multiple IPv4 addresses. Only IPv4
and Ethernet interfaces are supported.

## Output

Sample output running on Linux:

```text
Interface  HW Address        MTU     VLAN Parent     Type     Address         Net Mask        Broadcast       Destination     UuRLMA
---------- ----------------- ------- ---- ---------- -------- --------------- --------------- --------------- --------------- ------
docker0    02:42:92:1a:b8:f3 1500                    AF_INET  172.17.0.1      255.255.0.0     172.17.255.255                  U---MA
enp0s31f6  40:8d:5c:b1:9e:bf 1500                                                                                             U---MA
enp4s0     40:8d:5c:b1:9e:bd 1500                    AF_INET  192.168.1.109   255.255.255.0   192.168.1.255                   UuR-MA
enp4s0     40:8d:5c:b1:9e:bd 1500                    AF_INET  10.0.0.1        255.255.255.0   10.0.0.1                        UuR-MA
enp4s0.100 40:8d:5c:b1:9e:bd 1500    100  enp4s0     AF_INET  10.0.3.1        255.255.255.0   10.0.3.1                        ----MA
lo                           65536                   AF_INET  127.0.0.1       255.0.0.0                                       UuRL-A
virbr0     52:54:00:02:ef:c8 1500                    AF_INET  192.168.122.1   255.255.255.0   192.168.122.255                 U---MA
wlp5s0     a4:34:d9:12:69:cf 1500                                                                                             ----MA
```

We can see in the above sample on Linux:

- The interface `enp4s0` has two IP addresses on the same interface. The order
  is that as returned by the Operating System.
- The interface `enp4s0.100` is a VLAN interface, running on `enp4s0`.

The flags show:

- `U` the interface is `UP`.
- `u` for Linux, the driver indicates the interface is `LOWER_UP`.
- `R` interface is `RUNNING`.
- `L` the interface is `LOOPBACK`.
- `M` the interface supports `MULTICAST`.
- `A` the interface has ARP, so that the `NOARP` flag is not set.

Output is available on Linux, FreeBSD, NetBSD, QNX 7.1, QNX 8.0, Cygwin.

On Cygwin, interface names are queried (the aliases) instead of the opaque GUIDs
for the interfaces.

```text
$ ./tools/iplist/iplist
Interface                     HW Address        MTU     VLAN Parent                        Type     Address         Net Mask        Broadcast       Destination     UuRLMA
----------------------------- ----------------- ------- ---- ----------------------------- -------- --------------- --------------- --------------- --------------- ------
Local Area Connection* 1      a4:34:d9:12:69:d0 1500                                       AF_INET  169.254.98.134  255.255.0.0     169.254.255.255                 Uu--MA
Ethernet                      40:8d:5c:b1:9e:bf 1500                                       AF_INET  169.254.242.149 255.255.0.0     169.254.255.255                 Uu--MA
VMware Network Adapter VMnet1 00:50:56:c0:00:01 1500                                       AF_INET  192.168.84.1    255.255.255.0   192.168.84.255                  UuR-MA
Local Area Connection* 12                       0                                                                                                                   Uu--MA
Wi-Fi                                           0                                                                                                                   Uu--MA
Local Area Connection* 2      a6:34:d9:12:69:cf 1500                                       AF_INET  169.254.217.250 255.255.0.0     169.254.255.255                 Uu--MA
Ethernet 2                                      0                                                                                                                   Uu--MA
Local Area Connection* 11                       0                                                                                                                   Uu--MA
VMware Network Adapter VMnet8 00:50:56:c0:00:08 1500                                       AF_INET  192.168.10.1    255.255.255.0   192.168.10.255                  UuR-MA
Bluetooth Network Connection  a4:34:d9:12:69:d3 1500                                       AF_INET  169.254.50.152  255.255.0.0     169.254.255.255                 Uu--MA
Wi-Fi 2                       a4:34:d9:12:69:cf 1500                                       AF_INET  169.254.19.160  255.255.0.0     169.254.255.255                 Uu--MA
Ethernet (Kernel Debugger)                      0                                                                                                                   Uu--MA
Local Area Connection         00:ff:b6:90:05:55 1500                                       AF_INET  169.254.247.163 255.255.0.0     169.254.255.255                 Uu--MA
Ethernet 4                    1c:6f:65:cd:82:02 0                                          AF_INET  192.168.1.106   255.255.255.0                                   Uu--MA
Loopback Pseudo-Interface 1                     4294967295                                 AF_INET  127.0.0.1       255.0.0.0                                       UuRLMA
Ethernet 3                    40:8d:5c:b1:9e:bd 1500                                       AF_INET  192.168.1.109   255.255.255.0   192.168.1.255                   UuR-MA
```

Here, Windows would show four interfaces active:

- `VMware Network Adapter VMnet1`
- `VMware Network Adapter VMnet8`
- `Loopback Pseudo-Interface 1`
- `Ethernet 3`
