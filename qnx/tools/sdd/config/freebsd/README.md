# FreeBSD Service File

The file `sdd` is a FreeBSD rc service.

## Installation

- Create the directory structure

  ```sh
  umask 022
  cd /usr/local
  mkdir -p etc/rc.d
  mkdir -p bin
  ```

- Copy the service file in this directory to to `/usr/local/etc/rc.d/sdd`

  Ensure this service is marked as executable:

  ```sh
  chown root:wheel /usr/local/etc/rc.d/sdd
  chmod 755 /usr/local/etc/rc.d/sdd
  ```

- Copy the binary after building to `/usr/local/bin/sdd`

  Build using the instructions at the base of this repository (or use the
  `scripts` folder if you've set up podman).

- Update the `rc.conf`

  ```text
  sdd_enable="YES"
  ```
