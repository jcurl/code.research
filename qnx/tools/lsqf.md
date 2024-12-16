# List of Open QNX Files <!-- omit in toc -->

This is a small utility, which is based on Open QNX sources for
[pidin](https://github.com/vocho/openqnx/blob/master/trunk/utils/p/pidin/pidin_proc.c),
reimplemented in C++17.

The goal is to allow a user to see what files a PID has open.

- [1. Usage](#1-usage)
  - [1.1. Invocation](#11-invocation)
  - [1.2. Output](#12-output)
  - [1.3. Sample Output](#13-sample-output)
- [2. Implementation Details](#2-implementation-details)

## 1. Usage

Similar to the Unix tool `lsof`, this tool `lsqf` (list QNX files) provides a
tabulated form of processes and the file descriptors it has open. This is a
debug tool making use of debug functionality within QNX and has been tested on
QNX 7.1 and QNX 8.0.

### 1.1. Invocation

```sh
lsqf [-c] [-d] [-s] [-v] [-p <pid>]
```

It will list all files for the PIDs provided on the command line (a comma
separated list, ensure that there are no spaces in the list). If the `-p` option
is not given, all processes in the system (as observed by `/proc/<pid>`) are
enumerated (including the currently running process).

The following options provide additional information:

- `-c` show side channels. These are shown as an FD with a number, followed by
  's', like `0s`.
- `-d` show dead connections.
- `-s` show connections to self.
- `-v` add verbosity
  - In this version, this prints the full path of files obtained from the OS,
    instead of just the file name. This is not necessarily the path of the file
    from the root path name space.

The tool works best when running as `root`, but attempts to find alternative
non-root access to the names of processes. If the process name can't be
determined, then the only the PID is shown.

### 1.2. Output

The output of the tool is tabulated with the following columns:

- Process Name (PID) - Show the name of the process, with the PID of that
  process in parenthesis.
- File Descriptor and Flags
  - The file descriptor.
  - Flags include:
    - If the FD is followed by an `s`, this indicates an `_NTO_SIDE_CHANNEL` for
      a message connection, and not an FD. Only shown with the `-c` option.
    - `d` indicates a dead connection.
    - `c` indicates `CLOEXEC`, a file descriptor that is closed on `exec*()`.
    - `r` file is open for read (else `-`)
    - `w` file is open for write (else `-`)
- Resource Manager (PID) - Show the name of the process hosting the resource
  manager path, with the PID of the resource manager.
- Unix Mode, such as `Mrwxrwxrwx`, where the `x` bits include sticky bits.
  - The first character is one of:
    - `?` - Unknown
    - `-` - Regular file
    - `b` - Block file
    - `c` - Character file
    - `d` - Directory
    - `l` - Symbolic link
    - `n` - Named file (typed memory or shared memory)
    - `p` - FIFO / Pipe
    - `s` - Socket
  - The following characters are the mode of the file, for the user, group and
    other. The executable bit may also be `x`, `s`, `S` (user/group) or `x`,
    `t`, `T` (other).
- Path - The connector, or socket description (if available).

The mode and path are not printed for side channels or dead descriptors.

### 1.3. Sample Output

```sh
$ ./lsqf -cds
Process (PID)             FD (flags) RsrcMgr (PID)             Mode       Path
------------------------- ---------- ------------------------- ---------- ----------
procnto-smp-instr (1)     0rw        devc-serminiuart (7)      crw-rw-rw- /dev/ser1
procnto-smp-instr (1)     1rw        devc-pty (368654)         crw------- /dev/ttyp0
procnto-smp-instr (1)     2r-        devb-ram (4109)           -rwxr-xr-x /tmp/lsqf
procnto-smp-instr (1)     0s         procnto-smp-instr:1 (1)
procnto-smp-instr (1)     3s         procnto-smp-instr:2 (1)
procnto-smp-instr (1)     5s         procnto-smp-instr:3 (1)
slogger2 (2)              0rw        procnto-smp-instr (1)     crw------- /dev/text
slogger2 (2)              1rw        procnto-smp-instr (1)     crw------- /dev/text
slogger2 (2)              2rw        procnto-smp-instr (1)     crw------- /dev/text
slogger2 (2)              0s         procnto-smp-instr (1)
slogger2 (2)              2s         slogger2:1 (2)
slogger2 (2)              3sc        procnto-smp-instr (1)
slogger2 (2)              6sc        procnto-smp-instr (1)
slogger2 (2)              7sc        procnto-smp-instr (1)
slogger2 (2)              8sc        procnto-smp-instr (1)
slogger2 (2)              9sc        procnto-smp-instr (1)
slogger2 (2)              10sc       slogger2:1 (2)
dhclient (344081)         0rw        procnto-smp-instr (1)     crw-rw-rw- /dev/null
dhclient (344081)         1rw        procnto-smp-instr (1)     crw-rw-rw- /dev/null
dhclient (344081)         2rw        procnto-smp-instr (1)     crw-rw-rw- /dev/null
dhclient (344081)         3-w        devb-sdmmc-bcm2711 (12)   -rw-rw-rw- /fs/qnx6/var/db/dhclient.leases
dhclient (344081)         4crw       io-pkt-v6-hc (303120)     crw-rw---- /dev/bpf
dhclient (344081)         5crw       io-pkt-v6-hc (303120)     srw-rw-rw- I4UDP  *.68                  *.*
dhclient (344081)         0s         procnto-smp-instr (1)
```

## 2. Implementation Details

The program retrieves the name of the process using
[`DCMD_PROC_MAPDEBUG_BASE`](https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.prog/topic/process_DCMD_PROC_MAPDEBUG_BASE.html)
which requires read access to `/proc/<pid>/as`. If this fails, it attempts to
read the name of the process via `/proc/<pid>/cmdline` (which is typical when
running as non-root).

When querying for a process, the
[`ConnectServerInfo`](https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.lib_ref/topic/c/connectserverinfo.html)
returns the matched _coid_, which is the same as the [file
descriptor](https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.lib_ref/topic/c/connectattach.html).

To get details of the file descriptor, the resource manager providing the
service, must support duplicating the file descriptor (the message type
`_IO_DUP`) and then the resource manager callback
[`iofdinfo`](https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.lib_ref/topic/i/iofdinfo.html).
See also [I/O
messages](https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.resmgr/topic/fleshing_out_IO_messages.html).
