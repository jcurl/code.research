# Simple Discovery Daemon <!-- omit in toc -->

I have quite a few small Raspberry Pi devices, running either Linux, NetBSD,
FreeBSD, QNX. Intent is a very small daemon, specifically for the task of
emitting periodic messages announcing themselves.

On the latest QNX8 binaries, it is non-trivial to attach to the UART to identify
the device. It would be convenient if these devices had a small binary to
announce themselves, and a small tool on Linux or Windows to receive these
announcements.

- [1. Prior Work](#1-prior-work)
  - [1.1. Avahi / Bonjour](#11-avahi--bonjour)
- [2. Goals](#2-goals)
- [3. Usage](#3-usage)
  - [3.1. Example of Running](#31-example-of-running)
    - [3.1.1. Multicast Group Membership](#311-multicast-group-membership)
    - [3.1.2. Filtering Output](#312-filtering-output)

## 1. Prior Work

### 1.1. Avahi / Bonjour

Other tools can do this, such as [avahi](https://github.com/avahi) ([main
page](https://avahi.org/)). For larger systems, and a general way of providing
services, this is great! This could be investigated further, especially for
porting to QNX and integrating into the BSP images I maintain privately.

## 2. Goals

Goal is to have a very small binary, reusing the work in this repository.
Existing work is provided in the [iplist](./iplist.md) tool,

It should not interfere with existing services.

Security is not a concern, the tool is expected to only run in a small home-lab.
The security is not expected to be worse than mDNS.

## 3. Usage

The tool should start with no configuration, default options for the command
line, and overridable network configuration for sending.

```sh
sdd -S <ipaddr:port> -D <ipaddr:port> -n <interval>
```

Options:

- `-S` option identifies where to bind to.
- `-D` option identifies where to send to.
- `-n` option identifies the frequency to send updtes.

### 3.1. Example of Running

#### 3.1.1. Multicast Group Membership

On QNX, one can run the tool simply by:

```cmd
$ sdd
Source: 0.0.0.0:52599
Source Intf:
Destination: 239.255.42.99:52599
Interval: 1000ms
Opened: 192.168.178.41:52599
```

It starts transmitting a JSON snippet to the multicast address 239.255.42.99
with port 52599.

On simple networks with unmanaged switches, it's generally possible to observe
the traffic using Wireshark. On networks where the transmitter is behind a
managed switch, the traffic may not be sent further. This is because there is no
reciever on a different port of the switch which has subscribed to the group.

To receive the traffic reliably (that sends an IGMPv2 message to subscribe to
the message) on Linux:

```sh
socat - UDP-RECVFROM:52599,ip-add-membership=239.255.42.99:0.0.0.0,reuseaddr,fork
```

The first message received may be something like:

```text
{ "host": "rpi4-710-e729ecba", "interfaces": { "genet0": { "mac": "d8:3a:dd:fb:93:51", "mtu": 1500, "ipv4": [ "192.168.178.41" ] }, "lo0": { "mtu": 33136, "ipv4": [ "127.0.0.1" ] } } }
```

#### 3.1.2. Filtering Output

There are no newlines at the end of the packet. We can use `socat` and `jq` to
receive the UDP packets and print them to the console in a nice way.

```cmd
$ socat -u UDP-RECVFROM:52599,ip-add-membership=239.255.42.99:0.0.0.0,reuseaddr,fork SYSTEM:"jq ."
{
  "host": "rpi4-710-e729ecba",
  "interfaces": {
    "genet0": {
      "mac": "d8:3a:dd:fb:93:51",
      "mtu": 1500,
      "ipv4": [
        "192.168.178.41/24"
      ]
    },
    "lo0": {
      "mtu": 33136,
      "ipv4": [
        "127.0.0.1/8"
      ]
    }
  }
}
```

If we only want the host names, we can do:

```cmd
$ socat -u UDP-RECVFROM:52599,ip-add-membership=239.255.42.99:0.0.0.0,reuseaddr,fork SYSTEM:"jq .host"
"rpi4-710-e729ecba"
```

The second form is more compact if you only want to know the names of the
devices on the network.
