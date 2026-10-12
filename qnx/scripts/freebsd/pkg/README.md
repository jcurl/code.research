# pkgdownload <!-- omit in toc -->

Downloads and extracts packages, including their dependencies, from a FreeBSD
(default) or NetBSD package repository. NetBSD support is partial.

- [1. Setup](#1-setup)
- [2. Usage](#2-usage)
  - [2.1. Common Options](#21-common-options)
  - [2.2. System Files Download and Extraction (FreeBSD only)](#22-system-files-download-and-extraction-freebsd-only)
    - [2.2.1. NetBSD System Packages](#221-netbsd-system-packages)
  - [2.3. Package Files Download and Extraction](#23-package-files-download-and-extraction)
    - [2.3.1. FreeBSD Packages](#231-freebsd-packages)
    - [2.3.2. NetBSD Packages](#232-netbsd-packages)
      - [2.3.2.1. NetBSD Packages Further Work](#2321-netbsd-packages-further-work)
    - [2.3.3. Common Behaviour](#233-common-behaviour)
- [3. Example](#3-example)
  - [3.1. FreeBSD Base System and Packages](#31-freebsd-base-system-and-packages)
  - [3.2. NetBSD Packages](#32-netbsd-packages)
  - [3.3. Making a Local Mirror](#33-making-a-local-mirror)
    - [3.3.1. FreeBSD](#331-freebsd)
      - [3.3.1.1. Download the FreeBSD Base Binary](#3311-download-the-freebsd-base-binary)
      - [3.3.1.2. Download the FreeBSD Packages](#3312-download-the-freebsd-packages)
    - [3.3.2. NetBSD](#332-netbsd)
      - [3.3.2.1. Download a NetBSD Package and Dependencies](#3321-download-a-netbsd-package-and-dependencies)

## 1. Setup

Requires Python 3.12. Ubuntu 24.04 includes it as `python3`:

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

The requirements listed were those tested, they may not be hard requirements.
For example, the packages provided by Ubuntu 24.04, which are older, also work.

Ubuntu 22.04 only has Python 3.10. Install 3.12 with
[uv](https://docs.astral.sh/uv/) instead:

```sh
curl -LsSf https://astral.sh/uv/install.sh | sh
uv python install 3.12
uv venv --python 3.12 .venv
uv pip install -r requirements.txt
```

The commands below use `.venv/bin/python`.

## 2. Usage

```sh
.venv/bin/python pkgdownload.py [options] [package...]
```

### 2.1. Common Options

| Option            | Default                                                | Description                                                                              |
| ----------------- | ------------------------------------------------------ | ---------------------------------------------------------------------------------------- |
| `--os`            | `freebsd`                                              | Operating system: `freebsd` or `netbsd`, ignoring case.                                  |
| `--baseurl`       | `https://ftp.freebsd.org/pub/FreeBSD/releases`         | Repository URL. Use `file:///path` for a local mirror.                                   |
| `--version`       | `15` (FreeBSD)                                         | OS version.                                                                              |
| `--arch`          | `aarch64` (FreeBSD)                                    | OS architecture.                                                                         |
| `--branch`        | `latest` (FreeBSD packages), `RELEASE` with `--system` | Branch, e.g. `RELEASE` for system, FreeBSD `latest` or NetBSD `11.0_2026Q2` as examples. |
| `--ignore-ca`     | off                                                    | Skip HTTPS certificate checks.                                                           |
| `--ignore-verify` | off                                                    | Skip the catalogue signature and public key checks.                                      |
| `--sysroot`       | `{os}-{major}-{arch}/sysroot/{release}/{branch}/`      | Folder to extract the tarball into. `{release}` is the full version (`15` gives `15.0`). |
| `--no-extract`    | off                                                    | Download packages but do not extract them into the sysroot.                              |
| `--clean`         | off                                                    | Delete the folder first                                                                  |
| `--log-level`     | `INFO`                                                 | One of `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`.                                        |

### 2.2. System Files Download and Extraction (FreeBSD only)

```sh
.venv/bin/python pkgdownload.py [options] --system [package...]
```

Options:

| Option     | Default                   | Description                          |
| ---------- | ------------------------- | ------------------------------------ |
| `--system` |                           | Download the system tarball packages |
| `package`  | required unless `--clean` | The tarball name.                    |

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

Architectures not in this table are used unchanged.

#### 2.2.1. NetBSD System Packages

Downloading the system packages for NetBSD are not supported. That is because
this repository downloads the sources and builds the toolchain.

If we should decide to implement (such that we decide not to compile the
toolchain, e.g. to use Clang, as done similar to FreeBSD), we can observe the
system packages are at
https://ftp.netbsd.org/pub/NetBSD/NetBSD-11.0/evbarm-aarch64/binary/sets/.

### 2.3. Package Files Download and Extraction

```sh
.venv/bin/python pkgdownload.py [options] [package...]
```

Options:

| Option    | Default                    | Description                                            |
| --------- | -------------------------- | ------------------------------------------------------ |
| `package` | required unless `--clean`; | Package name(s). Dependencies are added automatically. |

#### 2.3.1. FreeBSD Packages

Files are downloaded to `freebsd-{major}-{arch}/pkg/{branch}/`. For FreeBSD, the
version is the "major" version, as minor versions are not tracked for packages.

For FreeBSD, the catalogue is first downloaded, and checked before use:
`packagesite.yaml` must match `packagesite.yaml.sig` using the key in
`packagesite.yaml.pub` (RSA, SHA-256 over the hex digest of the file). The key
details are logged at `INFO`. A bad signature is reported as an error.

The key itself must also be trusted. The `--conf` (default
`{sysroot}/etc/pkg/FreeBSD.conf`) gives the `fingerprints` folder to find the
trust. If the config is missing or has no `fingerprints` setting, a warning is
logged and only the signature is checked. A missing `trusted/` folder or an
untrusted key is an error.

Use `--ignore-verify` to skip both checks.

#### 2.3.2. NetBSD Packages

For NetBSD, the folder is `netbsd-{release}-{arch}/pkg/{branch}/`, as each
release has its own repository. Files are extracted to the sysroot given above.

The repository is `{baseurl}/{arch}/{branch}/`, such as
`https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/aarch64/11.0_2026Q2/`.

The branch is used as given in the repository URL, so `--branch 11.0_2026Q2` is
needed for `aarch64/11.0_2026Q2/`. Without `--branch`, the branch is the
release, which gives `aarch64/11.0/`. The branch is also the folder name used
when storing files locally, so `--version 11.0 --branch 11.0_2026Q2` is saved
under `11.0/11.0_2026Q2`.

The catalogue is `All/pkg_summary.bz2`, saved under the branch folder and
decompressed to `catalogue/pkg_summary`. `SHA512.bz2` is in the branch folder,
beside `All/`, and is saved there and decompressed to `catalogue/SHA512`.

The repository URL is
`https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/{arch}/{version}` (or
`{branch}` when given). The URL rule in 2.3.3 applies, with the archive
`https://archive.netbsd.org/pub/pkgsrc-archive/packages/NetBSD/`. A missing
`SHA512.bz2` is skipped as described below.

Dependencies are the `DEPENDS` lines of each package, one per pattern. Versions
are compared with pkgsrc's dewey rules, so `1.0alpha1` < `1.0` and `1.0nb2` >
`1.0nb1`. A `PKGNAME` is split at its last `-` into the name and version.

A pattern that matches one package is used. A pattern that matches nothing is a
**warning** and the run continues, as the package may not be in this branch. A
pattern that matches several packages is resolved as follows:

- If one of them is already in the sysroot, it is logged at `INFO` and the
  pattern is satisfied. A package is in the sysroot when every path in its
  `PROVIDES` exists there, so a package with no `PROVIDES` is never counted.
- Otherwise, if one of them is in the list of packages for this run, the pattern
  is satisfied.
- Otherwise the run stops with an error listing them, and you add one of them to
  the command line. Nothing is downloaded or extracted in that case.

A package can be named by its full `PKGNAME`, including the version, such as
`libxml2-2.15.3` or `curl-8.21.0`. Otherwise the name must equal the package
name, which is the `PKGNAME` without its version, so `libxml2` finds
`libxml2-2.15.3`. If the package name matches several versions, such as
`libxml2` with `libxml2-2.15.1` and `libxml2-2.15.3`, the latest version is used
by Dewey version comparison, and a warning lists the others. Give the full
`PKGNAME` to choose another. Dependency matching is described below.

Each package is downloaded from `{repo}All/{FILE_NAME}` to `All/` under the
branch folder, such as `All/libxml2-2.15.3.tgz`. The download is checked against
`FILE_SIZE` and, when `catalogue/SHA512` exists, its SHA512. The run only stops
for a wrong SHA512. A package with no entry in `catalogue/SHA512` is logged as a
warning and its checksum is not checked. The packages are gzip-compressed tar
files. They are extracted directly into the sysroot, as FreeBSD packages are, so
no `usr/pkg` folder is added.

The sysroot default uses the same version and branch folders as the repository,
so `--version 11.0 --branch 11.0_2026Q2` is stored under
`sysroot/11.0/11.0_2026Q2`.

##### 2.3.2.1. NetBSD Packages Further Work

- Signature checks. There are no signature checks for the NetBSD catalogue or
  `SHA512` file, unlike FreeBSD, so they rely on HTTPS only.

#### 2.3.3. Common Behaviour

You can pass a local mirror with `--baseurl file:///path/to/mirror`. The mirror
must have the same layout as the remote one, for example
`file:///path/to/mirror/FreeBSD:15:aarch64/latest/`. A local mirror has no archive
to fall back to.

Downloads of the package catalogue, packages and the `--system` release tarball
use the same URL rule for FreeBSD and NetBSD. The first download in a run is
tried on the mirror (`pkg_baseurl`, or `release_baseurl` for `--system`). If the
mirror serves it, the mirror is used for every later download in the run. If the
mirror fails, the file is tried once from the archive (`pkg_baseurl_archive`, or
`release_baseurl_archive` for `--system`). If the archive serves it, the archive
is used for every later download in the run. If both fail, the URL is not fixed,
so the next download tries the mirror again. A missing optional file, such as
NetBSD's `SHA512.bz2`, counts as a failure, so it can switch to the archive. The
URL is not kept between runs, so each run starts with the mirror. A `--baseurl`
given on the command line never uses the archive.

A `402 Payment Required` response is treated as `404 Not Found`, with a warning
in the log, so the file counts as missing and the usual fallback applies.

## 3. Example

### 3.1. FreeBSD Base System and Packages

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
- Package downloads: `{os}-{versionmajor}-{arch}/sysroot/{major}/{branch}`,
  where `{major}` is the major version (`15.1` gives `15`).

Because the branches differ necessarily, the extract to different folders, so
normally the `--sysroot` path should be given for correctly buiding a root. This
behaviour is useful to keep separated by default, so that one sees additions to
the OS installation.

### 3.2. NetBSD Packages

After compiling the toolchain, we have the sysroot. We'd like to install
`libxml2` into the sysroot for cross-compilation:

```sh
pkgdownload.py --os netbsd --version 11.0 --arch aarch64 --sysroot /opt/netbsd/11.0/aarch64/sysroot libxml2
```

### 3.3. Making a Local Mirror

You might want to download the packages but not extract them

#### 3.3.1. FreeBSD

Variables:

- `{version}`: 13.5, 14.2, 14.3, 14.4, 14.5, 15.0, 15.1, etc.
  - `{major}` is then 13, 14, 15, etc.
- `{arch}`: `amd64`, `aarch64`
- `{branch}` is default `latest` for packages, and `RELEASE` for system.

##### 3.3.1.1. Download the FreeBSD Base Binary

```sh
pkgdownload.py --os freebsd --version {version} --arch {arch} --no-extract --system base.txz
```

Then copy:

- from: `freebsd-{major}-{arch}/releases/{farch}/{version}-{branch}/`
  - The `{arch}` of `aarch64` equal to `{farch}` of `{arm64}`. For Intel it is
    `{amd64}`
    - FreeBSD uses `arm64` for system images, and `aarch64` for packages.
  - i.e. `freebsd-{major}-{arch}/releases/arm64/{version}-RELEASE`
  - i.e. one can use `--baseurl file://./freebsd-{major}-{arch}/releases`
- to: `{baseurl}/{farch}/{version}-{branch}/`

##### 3.3.1.2. Download the FreeBSD Packages

```sh
pkgdownload.py --os freebsd --version {version} --arch {arch} --no-extract --system base.txz
```

Then copy:

- from: `freebsd-{major}-{arch}/packages/FreeBSD:{major}:{arch}/{branch}`
  - e.g. `freebsd-14-aarch64/packages/FreeBSD:14:aarch64/latest`
  - i.e. one case use `--baseurl file://./freebsd-{major}-{arch}/packages`
- to: `{baseurl}/FreeBSD:{major}:{arch}/{branch}` where the packages are kept

#### 3.3.2. NetBSD

Variables:

- `{version}`: 10.0, 10.1, 10.2, 11.0, etc.
- `{arch}`: `amd64`, `aarch64`, `i386`, `aarch64eb`
- `{branch}` is default `latest` for packages.

##### 3.3.2.1. Download a NetBSD Package and Dependencies

```sh
pkgdownload.py --os netbsd --version {version} --arch {arch} aarch64eb --no-extract libxml2
```

Then copy

- from: `netbsd-{version}-{arch}/packages/{arch}/{version}`
- to: `{baseurl}/{arch}/{version}`
  - `SHA512.bz2` for the package digest
  - `All` folder containing the packages
