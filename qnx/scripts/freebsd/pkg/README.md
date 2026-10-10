# pkgdownload

Downloads and extracts packages, including their dependencies, from a FreeBSD
(default) or NetBSD package repository. NetBSD is a placeholder until its layout
is tested.

> Status: downloads the catalogue, the packages and their dependencies, and
> checks the catalogue's RSA signature, its public key against FreeBSD.conf, and
> each file's size and checksum.

## Setup

Requires Python 3.12. Ubuntu 24.04 includes it as `python3`:

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

Ubuntu 22.04 only has Python 3.10. Install 3.12 with
[uv](https://docs.astral.sh/uv/) instead:

```sh
curl -LsSf https://astral.sh/uv/install.sh | sh
uv python install 3.12
uv venv --python 3.12 .venv
uv pip install -r requirements.txt
```

The commands below use `.venv/bin/python`.

## Usage

```sh
.venv/bin/python pkgdownload.py -[options] [package...]
```

### System Files Download and Extraction

```sh
.venv/bin/python pkgdownload.py -[options] --system [package...]
```

Options:

| Option            | Default                                           | Description                                                                              |
| ----------------- | ------------------------------------------------- | ---------------------------------------------------------------------------------------- |
| `package`         | required unless `--clean`                         | The tarball name.                                                                        |
| `--os`            | `freebsd`                                         | Operating system: `freebsd` or `netbsd`, ignoring case.                                  |
| `--baseurl`       | `https://ftp.freebsd.org/pub/FreeBSD/releases`    | Repository URL. Use `file:///path` for a local mirror.                                   |
| `--version`       | `15` (FreeBSD)                                    | OS version.                                                                              |
| `--arch`          | `aarch64` (FreeBSD)                               | OS architecture.                                                                         |
| `--branch`        | `RELEASE`                                         | Branch, e.g. `RELEASE` or `STABLE`.                                                      |
| `--ignore-ca`     | off                                               | Skip HTTPS certificate checks.                                                           |
| `--ignore-verify` | off                                               | Skip the catalogue signature and public key checks.                                      |
| `--sysroot`       | `{os}-{major}-{arch}/sysroot/{release}/{branch}/` | Folder to extract the tarball into. `{release}` is the full version (`15` gives `15.0`). |
| `--no-extract`    | off                                               | Download packages but do not extract them into the sysroot.                              |
| `--clean`         | off                                               | Delete the folder first                                                                  |
| `--log-level`     | `INFO`                                            | One of `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`.                                        |

Files are downloaded to `{os}-{major}-{arch}/system/{release}/{branch}/`. Files
are extracted to the sysroot given above.

The URL is `{baseurl}/{arch}/{version}-{branch}/{tarball}`. The defaults are
`http://ftp.freebsd.org/pub/FreeBSD/releases`, `arm64`, `15.0` and `RELEASE`. A
bare major version such as `15` means `15.0`, and a full version such as `15.1`
is used as given. If that download fails, the same file is tried once from the
archive, `http://ftp-archive.freebsd.org/pub/FreeBSD-Archive/old-releases/`,
where FreeBSD moves old releases such as 13.5.

| FreeBSD `--arch` | Release folder |
| ---------------- | -------------- |
| `aarch64`        | `arm64`        |
| `amd64`          | `amd64`        |

Architectures not in this table are used unchanged. This table is FreeBSD's; the
NetBSD table is empty until it is tested.

### Package Files Download and Extraction

```sh
.venv/bin/python pkgdownload.py -[options] [package...]
```

Options:

| Option            | Default                                      | Description                                                                                                                     |
| ----------------- | -------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| `package`         | required unless `--clean`;                   | Package name(s). Dependencies are added automatically.                                                                          |
| `--os`            | `freebsd`                                    | Operating system: `freebsd` or `netbsd`, ignoring case.                                                                         |
| `--baseurl`       | `https://pkg.freebsd.org/`                   | Repository URL. Use `file:///path` for a local mirror.                                                                          |
| `--version`       | `15` (FreeBSD)                               | OS major version.                                                                                                               |
| `--arch`          | `aarch64` (FreeBSD)                          | OS architecture.                                                                                                                |
| `--branch`        | `latest`;                                    | Package branch, e.g. `latest` or `quarterly`.                                                                                   |
| `--ignore-ca`     | off                                          | Skip HTTPS certificate checks.                                                                                                  |
| `--ignore-verify` | off                                          | Skip the catalogue signature and public key checks.                                                                             |
| `--conf`          | `{sysroot}/etc/pkg/FreeBSD.conf`             | OS configuration file naming the trusted keys folder.                                                                           |
| `--sysroot`       | `{major}-{arch}/sysroot/{version}/{branch}/` | Folder to extract packages into. `{version}` is as given (`15.1` gives `15.1`).                                                 |
| `--no-extract`    | off                                          | Download packages but do not extract them into the sysroot.                                                                     |
| `--clean`         | off                                          | Delete the `pkg/{branch}` folder first (with `--system`, its release folder). The sysroot is kept. Without packages, then exit. |
| `--log-level`     | `INFO`                                       | One of `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`.                                                                               |

Files are downloaded to `{os}-{major}-{arch}/pkg/{branch}/`. The version is the
"major" version, as minor versions are not tracked for packages. Files are
extracted to the sysroot given above. You can pass the downloaded folder for
local archives with `--baseurl file:///path/to/freebsd-15-aarch64/pkg/`. If a
repository download fails, it is retried once from the archive mirror,
`https://mirror.sg.gs/freebsd-pkg/`.

The catalogue is checked before use: `packagesite.yaml` must match
`packagesite.yaml.sig` using the key in `packagesite.yaml.pub` (RSA, SHA-256
over the hex digest of the file). The key details are logged at `INFO`. A bad
signature is reported as an error.

The key itself must also be trusted. The `--conf` (default
`{sysroot}/etc/pkg/FreeBSD.conf`) gives the `fingerprints` folder to find the
trust. If the config is missing or has no `fingerprints` setting, a warning is
logged and only the signature is checked. A missing `trusted/` folder or an
untrusted key is an error.

Use `--ignore-verify` to skip both checks.

## Example

Download FreeBSD 15.1 base

```sh
pkgdownload.py --os freebsd --version 15.1 --arch aarch64 --branch RELEASE --sysroot ./freebsd-15-aarch64/sysroot --system base.txz
```

Download the packages libxml2 and dependencies

```sh
pkgdownload.py --os freebsd --version 15 --arch aarch64 --branch latest --sysroot ./freebsd-15-aarch64/sysroot libxml2
```

The `--sysroot`, if not provided, defaults to:

- System downloads: `{os}-{versionmajor}-{arch}/sysroot/{release}/{branch}`,
  where `{release}` is the full version (`15` gives `15.0`)
- Package downloads: `{os}-{versionmajor}-{arch}/sysroot/{version}/{branch}`

Because the branches differ necessarily, the extract to different folders, so
normally the `--sysroot` path should be given for correctly buiding a root. This
behaviour is useful to keep separated by default, so that one sees additions to
the OS installation.
