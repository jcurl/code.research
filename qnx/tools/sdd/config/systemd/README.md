# systemd Service File

The file `sdd.service` is a systemd unit, equivalent to the FreeBSD rc service
in `../freebsd/sdd`.

## Installation

- Copy the binary after building to `/usr/local/bin/sdd`

  Build using the instructions at the base of this repository (or use the
  `scripts` folder if you've set up podman).

  If you build locally, then you can install also with:

  ```sh
  make sdd
  sudo install -m 755 ./tools/sdd/sdd /usr/local/bin
  ```

- Copy the unit file to `/etc/systemd/system/sdd.service`

  ```sh
  sudo install -m 644 sdd.service /etc/systemd/system
  ```

- Reload systemd, then enable and start the service

  ```sh
  sudo systemctl daemon-reload
  sudo systemctl enable --now sdd
  ```
