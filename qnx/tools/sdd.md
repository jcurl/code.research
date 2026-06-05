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
