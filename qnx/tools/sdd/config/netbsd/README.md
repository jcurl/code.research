# FreeBSD Service File

The file `sdd` is a NetBSD rc service.

## Installation

### NetBSD 10.1

- Copy the service to `/etc/rc.d`. There is no `/usr/local/etc/rc.d` like in
  FreeBSD to separate our extra scripts.

  Ensure this service is marked as executable:

  ```sh
  chown root:wheel /etc/rc.d/sdd
  chmod 755 /etc/rc.d/sdd
  ```

- Copy the binary after building to `/usr/local/bin/sdd`

  Build using the instructions at the base of this repository (or use the
  `scripts` folder if you've set up podman).

  ```sh
  umask 022
  mkdir -m 755 -p /usr/local/bin
  install -m ./tools/sdd/sdd /usr/local/bin
  ```

- Update the `rc.conf`

  ```text
  sdd_enable="YES"
  ```
