#!/usr/bin/env python3

'''Download packages from a FreeBSD or NetBSD package repository.

The repository for a given OS, version, architecture and branch is located at
<baseurl>/FreeBSD:<version>:<arch>/<branch>/ for FreeBSD.

For NetBSD, the repository is located at <baseurl>/<arch>/<branch>/, where the branch is used as
given, such as 11.0_2026Q2, or is the release, such as 11.0, when not given. The catalogue is
All/pkg_summary.bz2, and SHA512.bz2 in the branch folder gives the SHA512 of each package.
Dependencies are the DEPENDS patterns, matched against the PKGNAMEs with the rules of pkg_install.

With --system, the installation tarball (base.txz by default) is downloaded from the OS
release folder into releases/<release arch>/<version>-<branch>/ and extracted into the sysroot,
instead of downloading packages.
Files are saved under <os>-<version>-<arch>/ in the same layout as the remote repository and releases,
so packages/ and releases/ can be given as a file:// --baseurl.
'''

import argparse
import bisect
import bz2
import fnmatch
import functools
import gzip
import hashlib
import json
import logging
import lzma
import operator
import os
import posixpath
import re
import shutil
import sys
import tarfile
from collections import deque
from collections.abc import Callable, Collection, Iterator, Sequence
from enum import Enum
from pathlib import Path, PurePosixPath
from typing import Any
from urllib.parse import urlsplit
from urllib.request import url2pathname

import requests
import urllib3
import zstandard
from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.asymmetric.rsa import RSAPublicKey

LOG_LEVELS = {
    'DEBUG': logging.DEBUG,
    'INFO': logging.INFO,
    'WARN': logging.WARNING,
    'ERROR': logging.ERROR,
    'FATAL': logging.CRITICAL,
}

logger = logging.getLogger(__name__)

Package = dict[str, Any]


class OSType(Enum):
    '''Operating systems supported by OS'''
    FREEBSD = 'FreeBSD'
    NETBSD = 'NetBSD'

    @classmethod
    def from_name(cls, name: str) -> 'OSType':
        '''Return the operating system with this name, ignoring case'''
        return next(os_type for os_type in cls if os_type.value.lower() == name.lower())


class OS:
    '''Settings and the version, architecture and branch for one operating system'''

    SETTINGS: dict[OSType, dict[str, Any]] = {
        OSType.FREEBSD: {
            'dir_prefix': 'freebsd',
            'default_version': '15',
            'default_arch': 'aarch64',
            'pkg_default_branch': 'latest',
            'release_default_branch': 'RELEASE',
            'pkg_baseurl': 'https://www.home.lan/xbsd/freebsd/packages',
            'pkg_baseurl_archive': 'https://pkg.freebsd.org',
            #'pkg_baseurl_archive': 'https://mirror.sg.gs/freebsd-pkg',
            'release_baseurl': 'https://www.home.lan/xbsd/freebsd/releases',
            'release_baseurl_archive': 'http://ftp.freebsd.org/pub/FreeBSD/releases',
            #'release_baseurl_archive': 'http://ftp-archive.freebsd.org/pub/FreeBSD-Archive/old-releases',
            'sysroot_dir_name': 'sysroot',
            # Release folders use a different name for some architectures. Others are used unchanged.
            'arch_names': {'aarch64': 'arm64', 'amd64': 'amd64'},
        },
        OSType.NETBSD: {
            'dir_prefix': 'netbsd',
            'default_version': '11',
            'default_arch': 'aarch64',
            # None: the branch is the release version, such as 11.0
            'pkg_default_branch': None,
            # None: --system is not supported for NetBSD yet
            'release_default_branch': None,
            'pkg_baseurl': 'https://www.home.lan/xbsd/netbsd/packages/',
            'pkg_baseurl_archive': 'https://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/',
            #'pkg_baseurl_archive': 'https://archive.netbsd.org/pub/pkgsrc-archive/packages/NetBSD/',
            'release_baseurl': None,
            'release_baseurl_archive': None,
            'sysroot_dir_name': 'sysroot',
            'arch_names': {'aarch64': 'arm64', 'amd64': 'amd64'},
        },
    }

    def __init__(self, os_type: OSType, version: str, arch: str, branch: str) -> None:
        self._settings = OS.SETTINGS[os_type]
        self._values = {'version': version, 'arch': arch, 'branch': branch}

    @property
    def values(self) -> dict[str, str]:
        '''The version, arch and branch'''
        return self._values

    @property
    def settings(self) -> dict[str, Any]:
        '''The settings of the operating system'''
        return self._settings

    def release_version(self) -> str:
        '''Return the full release version, where a bare major version such as 15 means 15.0'''
        return OS.release_name(self._values['version'])

    def major_version(self) -> str:
        '''Return the major version: 15 for 15.1'''
        return self.release_version().split('.')[0]

    def os_dir(self, dir_version: str) -> Path:
        '''Return the folder for the version and arch, such as freebsd-15-aarch64, holding the mirror and the sysroot'''
        return Path(f'{self._settings["dir_prefix"]}-{dir_version}-{self._values["arch"]}')

    def default_sysroot_dir(self, dir_version: str, version: str) -> Path:
        '''Return the default sysroot, sysroot/<version>/<branch> under the os_dir'''
        return self.os_dir(dir_version) / self._settings['sysroot_dir_name'] / version / self._values['branch']

    @staticmethod
    def release_name(version: str) -> str:
        '''Return the full release version for version, where a bare major version such as 15 means 15.0'''
        return version if '.' in version else f'{version}.0'


class Crypto:
    '''Digests, signature checks and key trust, as static methods'''

    # FreeBSD pkg hash type id for blake2_base32 (BLAKE2B-512)
    SUM_TYPE_BLAKE2B_512 = '2'
    # FreeBSD 13 catalogues store a bare SHA-256 hex digest without a type prefix
    SHA256_HEX_RE = re.compile(r'[0-9a-fA-F]{64}')
    Z_BASE32_ALPHABET = 'ybndrfg8ejkmcpqxot1uwisza345h769'
    SHA256_SUFFIX = '.sha256'

    @staticmethod
    def sha256_file(path: Path) -> str:
        '''Return the lowercase hex SHA-256 digest of the file contents'''
        return Crypto._file_digest(path, hashlib.sha256()).hex()

    @staticmethod
    def sha512_file(path: Path) -> str:
        '''Return the lowercase hex SHA-512 digest of the file contents'''
        return Crypto._file_digest(path, hashlib.sha512()).hex()

    @staticmethod
    def blake2b_512_z32(path: Path) -> str:
        '''Return the BLAKE2B-512 digest of the file contents, encoded as z-base32'''
        return Crypto._z_base32(Crypto._file_digest(path, hashlib.blake2b(digest_size=64)))

    @staticmethod
    def sha256_sidecar(path: Path) -> Path:
        '''Return the path of the SHA-256 file stored next to path, such as base.txz.sha256'''
        return path.with_name(path.name + Crypto.SHA256_SUFFIX)

    @staticmethod
    def write_sha256_sidecar(path: Path) -> None:
        '''Write the SHA-256 of path to its sidecar file, in the sha256sum format'''
        digest = Crypto.sha256_file(path)
        Crypto.sha256_sidecar(path).write_text(f'{digest}  {path.name}\n', encoding='ascii')
        logger.info('Wrote SHA-256 %s', Crypto.sha256_sidecar(path))

    @staticmethod
    def sha256_sidecar_matches(path: Path) -> bool:
        '''Return True when path exists and its sidecar holds the current SHA-256 of path'''
        sidecar = Crypto.sha256_sidecar(path)
        if not path.is_file() or not sidecar.is_file():
            return False
        try:
            fields = sidecar.read_text(encoding='ascii').split()
        except (OSError, UnicodeDecodeError):
            return False
        return bool(fields) and fields[0].lower() == Crypto.sha256_file(path)

    @staticmethod
    def verify_signature(yaml_path: Path, pub_path: Path, sig_path: Path) -> None:
        '''Verify the signature of packagesite.yaml using its public key

        The signature is RSA PKCS#1 v1.5 with SHA-256 over the lowercase hex SHA-256 of the file.
        Raises ValueError if a file is missing or the signature does not match.
        '''
        for path in (yaml_path, pub_path, sig_path):
            if not path.is_file():
                raise ValueError(f'signature check needs {path}')
        public_key = serialization.load_pem_public_key(pub_path.read_bytes())
        if not isinstance(public_key, RSAPublicKey):
            raise ValueError(f'{pub_path} is not an RSA public key')
        logger.info('Public key: RSA %d-bit, exponent %d', public_key.key_size,
                    public_key.public_numbers().e)
        logger.info('Public key SHA-256: %s', Crypto.sha256_file(pub_path))
        digest = Crypto.sha256_file(yaml_path).encode('ascii')
        try:
            public_key.verify(sig_path.read_bytes(), digest, padding.PKCS1v15(), hashes.SHA256())
        except InvalidSignature as exc:
            raise ValueError(f'invalid signature for {yaml_path}') from exc
        logger.info('Verified signature of %s', yaml_path)

    @staticmethod
    def verify_key_trust(pub_path: Path, trusted_dir: Path, revoked_dir: Path) -> None:
        '''Check that the key's SHA-256 is listed in trusted_dir and not in revoked_dir

        A missing revoked_dir lists no revoked keys. Raises ValueError if the key is revoked
        or not trusted.
        '''
        fingerprint = Crypto.sha256_file(pub_path)
        if fingerprint in Crypto._read_fingerprints(revoked_dir):
            raise ValueError(f'public key {pub_path} is revoked')
        if fingerprint not in Crypto._read_fingerprints(trusted_dir):
            raise ValueError(f'public key {pub_path} is not trusted by {trusted_dir}')
        logger.info('Public key is trusted by %s', trusted_dir)

    @staticmethod
    def package_mismatch(path: Path, package: Package) -> str | None:
        '''Return why the file does not match its catalogue entry, or None if it matches

        Checks the size, then the 'sum' field. FreeBSD 15 uses '2$<hash>' (BLAKE2B-512 in
        z-base32). FreeBSD 13 uses a bare SHA-256 hex digest.
        '''
        if not path.is_file():
            return f'{path} does not exist'
        size = path.stat().st_size
        expected_size = package.get('pkgsize')
        if size != expected_size:
            return f'{path} is {size} bytes, catalogue expects {expected_size}'
        raw_sum = str(package.get('sum', ''))
        sum_type, separator, expected_hash = raw_sum.partition('$')
        if separator and sum_type == Crypto.SUM_TYPE_BLAKE2B_512:
            actual_hash = Crypto.blake2b_512_z32(path)
            if actual_hash != expected_hash:
                return f'{path} BLAKE2B-512 is {actual_hash}, catalogue expects {expected_hash}'
            return None
        if not separator and Crypto.SHA256_HEX_RE.fullmatch(raw_sum):
            actual_hash = Crypto.sha256_file(path)
            if actual_hash != raw_sum.lower():
                return f'{path} SHA256 is {actual_hash}, catalogue expects {raw_sum}'
            return None
        return f'{package.get("name")} has unsupported checksum {raw_sum!r}'

    @staticmethod
    def _file_digest(path: Path, digest: Any) -> bytes:
        '''Return the digest of the file contents, read in chunks'''
        with path.open('rb') as data:
            for chunk in iter(lambda: data.read(1024 * 1024), b''):
                digest.update(chunk)
        return digest.digest()

    @staticmethod
    def _z_base32(data: bytes) -> str:
        '''Encode bytes as z-base32, filling each byte from its least significant bit'''
        bits = int.from_bytes(data, 'little')
        count = (len(data) * 8 + 4) // 5
        return ''.join(Crypto.Z_BASE32_ALPHABET[(bits >> (5 * i)) & 0x1F] for i in range(count))

    @staticmethod
    def _read_fingerprints(directory: Path) -> set[str]:
        '''Return the sha256 fingerprints listed in the files of a pkg fingerprint directory

        Each file has a line 'function: "sha256"' and one or more 'fingerprint: "<hex>"' lines.
        '''
        fingerprints: set[str] = set()
        if not directory.is_dir():
            return fingerprints
        for path in directory.iterdir():
            if not path.is_file():
                continue
            text = path.read_text()
            if re.search(r'^\s*function\s*:\s*"?sha256"?\s*$', text, re.MULTILINE):
                fingerprints.update(value.lower() for value in re.findall(
                    r'^\s*fingerprint\s*:\s*"?([0-9a-fA-F]+)"?', text, re.MULTILINE))
        return fingerprints


class Packages:
    '''Download and extraction of files

    Most methods are static. An instance is one folder of a remote mirror, saved with the same layout
    under a local root: <baseurl>/<folder>/<name> is saved as <root>/<folder>/<name>, so the root can be
    used as a file:// base URL itself. It also holds the mirror or archive choice for one run, used by fetch.
    '''

    DOWNLOAD_TIMEOUT = 60
    ZSTD_MAGIC = b'\x28\xb5\x2f\xfd'
    XZ_MAGIC = b'\xfd7zXZ\x00'
    GZIP_MAGIC = b'\x1f\x8b'

    def __init__(self, baseurl: str, archive_baseurl: str | None, root: Path, folder: str) -> None:
        self.baseurl = baseurl
        self.archive_baseurl = archive_baseurl
        self.folder = folder
        self.dir = root / folder
        self.decided = False
        self.use_archive = False

    @staticmethod
    def is_file_url(url: str) -> bool:
        '''Return True when the URL is a file:// local path'''
        return urlsplit(url).scheme == 'file'

    @staticmethod
    def remove_tree(path: Path) -> None:
        '''Remove the folder tree at path, if it exists'''
        if not path.exists():
            logger.info('Nothing to clean, %s does not exist', path)
            return
        shutil.rmtree(path)
        logger.info('Removed %s', path)

    @staticmethod
    def remove_if_empty(path: Path) -> bool:
        '''Remove the folder tree at path if it holds no files, only folders; return True if removed'''
        for root, dirs, files in os.walk(path):
            if files or any(os.path.islink(os.path.join(root, d)) for d in dirs):
                return False
        shutil.rmtree(path)
        return True

    @staticmethod
    def _get(url: str, verify_tls: bool) -> requests.Response:
        '''Start a streamed GET, reporting a 402 Payment Required as a 404 Not Found'''
        response = requests.get(url, stream=True, timeout=Packages.DOWNLOAD_TIMEOUT, verify=verify_tls)
        if response.status_code == 402:
            logger.warning('402 Payment Required for %s, treating it as 404 Not Found', url)
            response.status_code = 404
            response.reason = 'Not Found'
        return response

    @staticmethod
    def file(url: str, dest: Path, verify_tls: bool) -> None:
        '''Stream the URL to dest, or copy it when the URL is a file:// local path'''
        logger.info('Downloading %s to %s', url, dest)
        parts = urlsplit(url)
        if parts.scheme == 'file':
            if parts.netloc not in ('', 'localhost'):
                raise ValueError(f'file:// URL must be a local path: {url}')
            source = Path(url2pathname(parts.path))
            # The download folder may be the mirror itself, so the file may already be in place
            if not (dest.exists() and os.path.samefile(source, dest)):
                shutil.copyfile(source, dest)
        else:
            with Packages._get(url, verify_tls) as response:
                response.raise_for_status()
                with dest.open('wb') as out:
                    for chunk in response.iter_content(chunk_size=64 * 1024):
                        out.write(chunk)
        logger.info('Downloaded %s', dest)

    @staticmethod
    def decompress(archive: Path, dest: Path) -> None:
        '''Decompress the bzip2 file to dest, replacing dest only once the decompression is complete'''
        logger.info('Extracting %s to %s', archive, dest)
        dest.parent.mkdir(parents=True, exist_ok=True)
        partial = dest.with_name(dest.name + '.part')
        with bz2.open(archive, 'rb') as compressed, partial.open('wb') as out:
            shutil.copyfileobj(compressed, out)
        os.replace(partial, dest)
        logger.info('Extracted %s to %s', archive, dest)

    def _url(self, base: str, name: str) -> str:
        '''Return the URL of name in the folder under the base URL'''
        return f'{base.rstrip("/")}/{self.folder}/{name}'

    def fetch(self, name: str, verify_tls: bool) -> None:
        '''Download name from the folder of the mirror to the same name in dir, or from the archive once chosen

        The folders of the destination are created. The download is tried on the mirror first. If it fails,
        it is retried once from the archive. If the archive serves it, the archive is used for the rest of the
        run. If the mirror serves it, the mirror is used for the rest of the run. If both fail, the choice is
        left open, so the next download tries the mirror again. A missing optional file, such as SHA512.bz2,
        counts as a failed download.
        '''
        dest = self.dir / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        mirror_url = self._url(self.baseurl, name)
        archive_url = None if self.archive_baseurl is None else self._url(self.archive_baseurl, name)
        if self.decided:
            Packages.file(archive_url if self.use_archive and archive_url else mirror_url, dest, verify_tls)
            return
        try:
            Packages.file(mirror_url, dest, verify_tls)
        except requests.RequestException as exc:
            if archive_url is None:
                raise
            logger.warning('Download failed (%s), trying the archive', exc)
            Packages.file(archive_url, dest, verify_tls)
            self.use_archive = True
        self.decided = True

    @staticmethod
    def allow_absolute_links(member: tarfile.TarInfo, dest_path: str) -> tarfile.TarInfo | None:
        '''Extraction filter for system tarballs, where absolute symlinks are kept as sysroot paths

        Other members use the data filter. An absolute symlink's parent directory must stay inside
        dest_path, which the data filter cannot check when a symlink already exists at the name.
        '''
        if not (member.issym() and member.linkname.startswith('/')):
            return tarfile.data_filter(member, dest_path)
        dest_real = os.path.realpath(dest_path)
        parent = os.path.realpath(os.path.join(dest_path, os.path.dirname(member.name)))
        if os.path.commonpath([dest_real, parent]) != dest_real:
            raise tarfile.OutsideDestinationError(member, parent)
        return member

    @staticmethod
    def _is_root_metadata(member: tarfile.TarInfo) -> bool:
        '''Return True for root-level package metadata such as +MANIFEST'''
        name = member.name.removeprefix('./')
        return '/' not in name and name.startswith('+')

    @staticmethod
    def extract(archive: Path, dest_dir: Path,
                extraction_filter: str | Callable[[tarfile.TarInfo, str], tarfile.TarInfo | None]
                = 'data') -> None:
        '''Extract the zstd-, xz- or gzip-compressed tar archive into dest_dir, overwriting existing files

        The compression is chosen from the file's magic bytes, as FreeBSD 13 packages use xz and
        NetBSD packages use gzip. Root-level '+' files (package metadata) are skipped.
        '''
        logger.info('Extracting %s to %s', archive, dest_dir)
        with archive.open('rb') as compressed:
            magic = compressed.read(len(Packages.XZ_MAGIC))
            compressed.seek(0)
            if magic.startswith(Packages.ZSTD_MAGIC):
                decompressed = zstandard.ZstdDecompressor().stream_reader(
                    compressed, read_across_frames=True)
            elif magic == Packages.XZ_MAGIC:
                decompressed = lzma.open(compressed, 'rb')
            elif magic.startswith(Packages.GZIP_MAGIC):
                decompressed = gzip.open(compressed, 'rb')
            else:
                raise ValueError(f'{archive} is neither zstd, xz nor gzip compressed')
            dest_dir.mkdir(parents=True, exist_ok=True)
            with decompressed as reader, tarfile.open(fileobj=reader, mode='r|') as tar:
                members = (member for member in tar if not Packages._is_root_metadata(member))
                tar.extractall(dest_dir, members=members, filter=extraction_filter)
        logger.info('Extracted %s to %s', archive, dest_dir)


class Dewey:
    '''Compare package versions as pkgsrc's dewey does, so that 1.0alpha1 < 1.0 and 1.0nb2 > 1.0nb1'''

    # Modifiers, which sort before the release (0) when negative
    MODIFIERS = {'alpha': -3, 'beta': -2, 'pre': -1, 'rc': -1, 'pl': 0}
    TOKEN_RE = re.compile(r'\d+|alpha|beta|pre|rc|pl|nb\d*|[._]|[a-z]', re.IGNORECASE)

    @classmethod
    def key(cls, version: str) -> tuple[list[int], int]:
        '''Return the numbers of version, which compare component by component, and its nb revision'''
        numbers: list[int] = []
        revision = 0
        for match in cls.TOKEN_RE.finditer(version):
            token = match.group(0).lower()
            if token.isdigit():
                numbers.append(int(token))
            elif token.startswith('nb'):
                revision = int(token[2:] or 0)
            elif token in cls.MODIFIERS:
                numbers.append(cls.MODIFIERS[token])
            elif token in ('.', '_'):
                numbers.append(0)
            else:
                # A letter after a separator sorts after it, as 'a' is 1, 'b' is 2 and so on
                numbers.extend([0, ord(token) - ord('a') + 1])
        return numbers, revision

    @classmethod
    def compare(cls, left: str, right: str) -> int:
        '''Return a negative number, zero or a positive number as left is before, equal to or after right'''
        left_numbers, left_revision = cls.key(left)
        right_numbers, right_revision = cls.key(right)
        width = max(len(left_numbers), len(right_numbers))
        left_numbers += [0] * (width - len(left_numbers))
        right_numbers += [0] * (width - len(right_numbers))
        if left_numbers != right_numbers:
            return -1 if left_numbers < right_numbers else 1
        return (left_revision > right_revision) - (left_revision < right_revision)


class BsdPackageRepositoryBase:
    '''The package catalogue and packages for one OS, version, architecture and branch

    Shared by FreeBSD and NetBSD. Subclasses give the repository layout, load the catalogue and
    name the packages each package depends on. The repository folder is saved under the packages
    folder of the OS folder with the same path as on the mirror, so it can be a file:// base URL.
    Packages are extracted into the sysroot.
    '''

    CATALOGUE_DIR_NAME = 'catalogue'
    # The catalogue file, relative to the repository URL and to the branch folder
    PACKAGESITE_RELPATH: str
    # The unpacked catalogue file in the catalogue folder
    CATALOGUE_NAME: str

    def __init__(self, os_: OS, baseurl: str, sysroot: Path | None, conf_path: Path | None,
                 allow_archive: bool = True) -> None:
        self.os = os_
        dir_version = self._dir_version()
        self.sysroot = sysroot or os_.default_sysroot_dir(dir_version, os_.values['version'])
        self.conf_path = conf_path
        archive_baseurl = os_.settings['pkg_baseurl_archive'] if allow_archive else None
        self.packages = Packages(baseurl, archive_baseurl, os_.os_dir(dir_version) / 'packages',
                                 f'{self._repo_dir()}/{os_.values["branch"]}')
        self.branch_dir = self.packages.dir
        self.catalogue_dir = self.branch_dir / self.CATALOGUE_DIR_NAME
        self.packagesite = self.branch_dir / self.PACKAGESITE_RELPATH
        self.catalogue: dict[str, Package] = {}

    def _dir_version(self) -> str:
        '''Return the version in the name of the OS folder, such as 15 for freebsd-15-aarch64'''
        raise NotImplementedError

    def _repo_dir(self) -> str:
        '''Return the folder of the repository under the base URL, before the branch'''
        raise NotImplementedError

    def _fetch_catalogue(self, verify_tls: bool) -> Path:
        '''Download and unpack the catalogue unless it is already unpacked, and return the unpacked file'''
        catalogue_path = self.catalogue_dir / self.CATALOGUE_NAME
        if catalogue_path.is_file():
            logger.info('Catalogue already extracted in %s', self.catalogue_dir)
            return catalogue_path
        if self.packagesite.is_file():
            logger.info('Using %s already downloaded', self.packagesite)
        else:
            self.packages.fetch(self.PACKAGESITE_RELPATH, verify_tls)
        self._unpack_catalogue(catalogue_path)
        return catalogue_path

    def _unpack_catalogue(self, catalogue_path: Path) -> None:
        '''Unpack the downloaded catalogue to catalogue_path, with any files that verify it'''
        raise NotImplementedError

    def _dependencies(self, package: Package, known: Collection[str]) -> list[str]:
        '''Return the names of the packages that package needs, leaving out those already in known

        A dependency that has several matches is not returned, as _unmet_dependencies checks it
        once every package is resolved.
        '''
        raise NotImplementedError

    def _unmet_dependencies(self, package: Package, resolved: Collection[str]) -> list[str]:
        '''Return a message for each dependency of package that resolved does not satisfy'''
        return []

    def _canonical_name(self, name: str) -> str:
        '''Return the catalogue key for a package name given by the user, or raise ValueError'''
        if name not in self.catalogue:
            raise ValueError(f'package not found in catalogue: {name}')
        return name

    def _package_path(self, package: Package) -> Path:
        '''Return where the package is saved, mirroring its repopath under the branch folder'''
        parts = PurePosixPath(package['repopath']).parts
        if not parts or parts[0] == '/' or '..' in parts:
            raise ValueError(f'invalid repository path: {package["repopath"]}')
        return self.branch_dir.joinpath(*parts)

    def _package_problem(self, path: Path, package: Package) -> str | None:
        '''Return why the file does not match its catalogue entry, or None if it matches'''
        return Crypto.package_mismatch(path, package)

    def load_catalogue(self, verify_tls: bool, ignore_verify: bool) -> None:
        '''Download, verify and index the catalogue'''
        raise NotImplementedError

    def resolve(self, names: Sequence[str]) -> list[Package]:
        '''Return the requested packages followed by all their dependencies, without duplicates'''
        requested = [self._canonical_name(name) for name in names]
        resolved: dict[str, Package] = {}
        pending = deque(requested)
        while pending:
            name = self._canonical_name(pending.popleft())
            if name in resolved:
                continue
            package = self.catalogue[name]
            resolved[name] = package
            pending.extend(self._dependencies(package, {*requested, *resolved}))
        # Checked after the closure is complete, so a match resolved later still counts
        problems = [problem for package in resolved.values()
                    for problem in self._unmet_dependencies(package, resolved)]
        if problems:
            raise ValueError('; '.join(problems))
        return list(resolved.values())

    def fetch(self, package: Package, verify_tls: bool) -> None:
        '''Download the package unless a matching copy already exists, then verify the download'''
        dest = self._package_path(package)
        if self._package_problem(dest, package) is None:
            logger.info('Already downloaded %s', dest)
            return
        self.packages.fetch(package['repopath'], verify_tls)
        problem = self._package_problem(dest, package)
        if problem is not None:
            raise ValueError(f'verification failed for {package["name"]}: {problem}')

    def extract(self, package: Package) -> None:
        '''Extract the downloaded package into the sysroot'''
        Packages.extract(self._package_path(package), self.sysroot)


class FreeBsdPackageRepository(BsdPackageRepositoryBase):
    '''Packages from a FreeBSD pkg repository, catalogued in packagesite.pkg'''

    PACKAGESITE_RELPATH = 'packagesite.pkg'
    PACKAGESITE_YAML_NAME = 'packagesite.yaml'
    CATALOGUE_NAME = PACKAGESITE_YAML_NAME
    PACKAGESITE_PUB_NAME = f'{PACKAGESITE_YAML_NAME}.pub'
    PACKAGESITE_SIG_NAME = f'{PACKAGESITE_YAML_NAME}.sig'
    CONF_RELPATH = Path('etc/pkg/FreeBSD.conf')

    def __init__(self, os_: OS, baseurl: str, sysroot: Path | None, conf_path: Path | None,
                 allow_archive: bool = True) -> None:
        super().__init__(os_, baseurl, sysroot, conf_path, allow_archive)
        if conf_path is None:
            self.conf_path = self.sysroot / self.CONF_RELPATH

    def _dir_version(self) -> str:
        '''Return the major version, so 15.1 uses freebsd-15-aarch64'''
        return self.os.major_version()

    def _repo_dir(self) -> str:
        '''Return the folder named by the major version, so 15.1 uses FreeBSD:15:aarch64'''
        return f'FreeBSD:{self._dir_version()}:{self.os.values["arch"]}'

    def _unpack_catalogue(self, catalogue_path: Path) -> None:
        '''Extract packagesite.pkg, which holds packagesite.yaml and the files that verify it'''
        Packages.extract(self.packagesite, self.catalogue_dir)

    def _dependencies(self, package: Package, known: Collection[str]) -> list[str]:
        '''Return the dependency names, which are the keys of the 'deps' object'''
        return list(package.get('deps', {}))

    @staticmethod
    def _index(path: Path) -> dict[str, Package]:
        '''Index the packagesite.yaml JSON lines by package name'''
        catalogue: dict[str, Package] = {}
        with path.open(encoding='utf-8') as catalogue_file:
            for line in catalogue_file:
                if line.strip():
                    package = json.loads(line)
                    catalogue[package['name']] = package
        logger.debug('Loaded %d packages from %s', len(catalogue), path)
        return catalogue

    def _fingerprint_dirs(self, conf_path: Path) -> tuple[Path, Path] | None:
        '''Return the trusted and revoked key folders named in FreeBSD.conf

        The fingerprints path is relative to the root holding the conf's etc folder, where
        base.txz installs it. That root may differ from the sysroot of the current run.
        Returns None, with a warning, if FreeBSD.conf is missing or has no fingerprints setting.
        Raises ValueError if the trusted folder does not exist.
        '''
        if not conf_path.is_file():
            logger.warning('No FreeBSD.conf at %s, the public key cannot be verified', conf_path)
            return None
        match = re.search(r'^\s*fingerprints\s*[:=]\s*"?([^"\s,]+)', conf_path.read_text(),
                          re.MULTILINE)
        if match is None:
            logger.warning('No fingerprints setting in %s, the public key cannot be verified',
                           conf_path)
            return None
        conf_root = conf_path.resolve().parents[len(self.CONF_RELPATH.parts) - 1]
        keys_dir = conf_root / match.group(1).lstrip('/')
        trusted_dir = keys_dir / 'trusted'
        if not trusted_dir.is_dir():
            raise ValueError(f'no trusted keys directory {trusted_dir} (from {conf_path})')
        return trusted_dir, keys_dir / 'revoked'

    def load_catalogue(self, verify_tls: bool, ignore_verify: bool) -> None:
        '''Download and extract packagesite.pkg if needed, verify it, then index it'''
        yaml_path = self._fetch_catalogue(verify_tls)
        if ignore_verify:
            logger.warning('catalogue signature check is disabled')
        else:
            pub_path = self.catalogue_dir / self.PACKAGESITE_PUB_NAME
            Crypto.verify_signature(yaml_path, pub_path,
                                    self.catalogue_dir / self.PACKAGESITE_SIG_NAME)
            key_dirs = self._fingerprint_dirs(self.conf_path)
            if key_dirs is not None:
                Crypto.verify_key_trust(pub_path, *key_dirs)
        self.catalogue = self._index(yaml_path)


class NetBsdPackageRepository(BsdPackageRepositoryBase):
    '''Packages from a NetBSD pkgsrc repository, catalogued in All/pkg_summary.bz2

    Entries are keyed by PKGNAME, which includes the version, as more than one version of a
    package can be listed. A name given by the user can also be the package name without its version. Each DEPENDS
    pattern is matched against the PKGNAMEs. Each package is verified against the SHA512 in
    SHA512.bz2, when the repository has one.
    '''

    PACKAGES_DIR = 'All'
    PACKAGESITE_RELPATH = f'{PACKAGES_DIR}/pkg_summary.bz2'
    CATALOGUE_NAME = 'pkg_summary'
    # Beside All/ in the branch folder, with lines such as 'SHA512 (All/name.tgz) = <hex>'
    SHA512_RELPATH = 'SHA512.bz2'
    SHA512_NAME = 'SHA512'
    SHA512_LINE_RE = re.compile(r'SHA512 \((.+)\) = ([0-9a-fA-F]{128})')
    # Fields that can appear more than once in an entry
    MULTI_VALUED_FIELDS = frozenset({'DEPENDS', 'PROVIDES'})
    ALTERNATES_RE = re.compile(r'\{([^{}]*)\}')
    # Text of a pattern before its first glob, alternate or version constraint
    PATTERN_PREFIX_RE = re.compile(r'[*?\[{<>]')
    CONSTRAINTS_RE = re.compile(r'(?:(?:<=|>=|<|>)[^<>]+)+')
    CONSTRAINT_RE = re.compile(r'(<=|>=|<|>)([^<>]+)')
    COMPARISONS = {
        '<': operator.lt,
        '<=': operator.le,
        '>': operator.gt,
        '>=': operator.ge,
    }

    def __init__(self, os_: OS, baseurl: str, sysroot: Path | None, conf_path: Path | None,
                 allow_archive: bool = True) -> None:
        super().__init__(os_, baseurl, sysroot, conf_path, allow_archive)
        # Repository path of each package, such as All/name.tgz, to its lowercase SHA512 digest, or None
        # when the repository has no SHA512.bz2
        self.sha512s: dict[str, str] | None = None
        # The PKGNAMEs in sorted order, for finding those that start with a pattern's prefix
        self.sorted_names: list[str] = []

    def _dir_version(self) -> str:
        '''Return the full release version, so 11 uses netbsd-11.0-aarch64'''
        return self.os.release_version()

    def _repo_dir(self) -> str:
        '''Return the folder named by the architecture'''
        return self.os.values['arch']

    def _unpack_catalogue(self, catalogue_path: Path) -> None:
        '''Decompress pkg_summary.bz2'''
        Packages.decompress(self.packagesite, catalogue_path)

    def _canonical_name(self, name: str) -> str:
        '''Return the PKGNAME for name, which is either a PKGNAME or a package name without its version

        A PKGNAME matches exactly. Otherwise the name is the package name without its version, and
        when several versions match, the latest by Dewey version is used with a warning.
        '''
        if name in self.catalogue:
            return name
        matches = [pkgname for pkgname in self.catalogue
                   if pkgname.rpartition('-')[0] == name]
        if not matches:
            raise ValueError(f'package not found in catalogue: {name}')
        if len(matches) == 1:
            return matches[0]
        latest = max(matches, key=functools.cmp_to_key(
            lambda left, right: Dewey.compare(left.rpartition('-')[2], right.rpartition('-')[2])))
        logger.warning('%s matches several packages: %s. Using %s, the latest version. '
                       'Give the full PKGNAME to choose another.', name, ', '.join(matches), latest)
        return latest

    def _dependencies(self, package: Package, known: Collection[str]) -> list[str]:
        '''Return the PKGNAME of each DEPENDS pattern of package that matches exactly one package

        A pattern with several matches, or none, is left to _unmet_dependencies, which checks it once
        every package is resolved.
        '''
        names: list[str] = []
        for pattern in package.get('DEPENDS', []):
            matches = self._matches(pattern)
            if len(matches) == 1 and matches[0] not in known:
                names.append(matches[0])
        return names

    def _unmet_dependencies(self, package: Package, resolved: Collection[str]) -> list[str]:
        '''Return a message for each DEPENDS pattern of package that several packages match, none of
        which is installed in the sysroot or resolved in this run

        A pattern that matches nothing is only a warning, as the package may not be in this branch.
        '''
        problems: list[str] = []
        for pattern in package.get('DEPENDS', []):
            matches = self._matches(pattern)
            if not matches:
                logger.warning('%s depends on %s, which is not in the catalogue', package['PKGNAME'], pattern)
                continue
            if len(matches) == 1:
                continue
            installed = [name for name in matches if self._installed(name)]
            if installed:
                logger.info('%s depends on %s, which is already in the sysroot as %s',
                            package['PKGNAME'], pattern, installed[0])
                continue
            if any(name in resolved for name in matches):
                continue
            problems.append(
                f'{package["PKGNAME"]} depends on {pattern}, which matches several packages: '
                f'{", ".join(matches)}. None is in the sysroot or in this run, so add one of them '
                'to the list of packages to install.')
        return problems

    def _matches(self, pattern: str) -> list[str]:
        '''Return the PKGNAMEs that a DEPENDS pattern matches, in sorted order'''
        matches: set[str] = set()
        for expanded in self._expand_alternates(pattern):
            prefix = self.PATTERN_PREFIX_RE.split(expanded, maxsplit=1)[0]
            start = bisect.bisect_left(self.sorted_names, prefix)
            for pkgname in self.sorted_names[start:]:
                if not pkgname.startswith(prefix):
                    break
                if self._pattern_matches(expanded, pkgname):
                    matches.add(pkgname)
        return sorted(matches)

    @classmethod
    def _expand_alternates(cls, pattern: str) -> list[str]:
        '''Expand the csh-style alternates in pattern, such as name-{1,nb*}, into patterns without braces'''
        match = cls.ALTERNATES_RE.search(pattern)
        if match is None:
            return [pattern]
        expanded: list[str] = []
        for alternative in match.group(1).split(','):
            expanded.extend(cls._expand_alternates(
                pattern[:match.start()] + alternative + pattern[match.end():]))
        return expanded

    @classmethod
    def _pattern_matches(cls, pattern: str, pkgname: str) -> bool:
        '''Return whether pkgname matches a pattern without alternates, as pkg_match in pkg_install does

        A pattern with a version constraint, such as foo>=1.0<2, matches the PKGNAMEs with the same name
        whose version satisfies every constraint. Otherwise the pattern is a glob, which may also match
        the pattern followed by a version, so that nodejs matches nodejs-22.0.
        '''
        relation = re.search(r'[<>]', pattern)
        if relation is not None:
            name, _, version = pkgname.rpartition('-')
            constraints = pattern[relation.start():]
            if name != pattern[:relation.start()] or not cls.CONSTRAINTS_RE.fullmatch(constraints):
                return False
            return all(cls.COMPARISONS[operator_](Dewey.compare(version, bound), 0)
                       for operator_, bound in cls.CONSTRAINT_RE.findall(constraints))
        return (fnmatch.fnmatchcase(pkgname, pattern)
                or fnmatch.fnmatchcase(pkgname, f'{pattern}-[0-9]*'))

    def _installed(self, pkgname: str) -> bool:
        '''Return whether every path the package provides is in the sysroot, so the package is installed'''
        provides = self.catalogue[pkgname].get('PROVIDES', [])
        return bool(provides) and all(
            (self.sysroot / posixpath.normpath(path).lstrip('/')).exists() for path in provides)

    def _fetch_if_present(self, relpath: str, verify_tls: bool) -> bool:
        '''Download relpath into the branch folder, returning False if the repository does not have it

        A missing file is any 4xx from the server, or a missing file for a local mirror. Other errors
        are raised.
        '''
        try:
            self.packages.fetch(relpath, verify_tls)
        except FileNotFoundError:
            return False
        except requests.HTTPError as exc:
            if exc.response is not None and 400 <= exc.response.status_code < 500:
                return False
            raise
        return True

    def _package_problem(self, path: Path, package: Package) -> str | None:
        '''Return why the file does not match FILE_SIZE or its SHA512 in the catalogue, or None

        The SHA512 is only checked when the repository has a SHA512.bz2, and a package missing from it
        is logged, not reported as a problem.
        '''
        if not path.is_file():
            return f'{path} does not exist'
        size = path.stat().st_size
        if size != package['pkgsize']:
            return f'{path} is {size} bytes, catalogue expects {package["pkgsize"]}'
        if self.sha512s is None:
            return None
        expected = self.sha512s.get(package['repopath'])
        if expected is None:
            logger.warning('%s has no SHA512 in %s, so its checksum is not checked',
                           package['name'], self.SHA512_NAME)
            return None
        actual = Crypto.sha512_file(path)
        if actual != expected:
            return f'{path} SHA512 is {actual}, catalogue expects {expected}'
        return None

    @classmethod
    def _entries(cls, path: Path) -> Iterator[Package]:
        '''Yield each entry of the pkg_summary, separated by blank lines, with KEY=value fields'''
        package: Package = {}
        with path.open(encoding='utf-8', errors='replace') as summary:
            for line in summary:
                line = line.rstrip('\n')
                if not line:
                    if package:
                        yield package
                        package = {}
                    continue
                key, separator, value = line.partition('=')
                if not separator:
                    continue
                if key in cls.MULTI_VALUED_FIELDS:
                    package.setdefault(key, []).append(value)
                else:
                    package.setdefault(key, value)
        if package:
            yield package

    @classmethod
    def _index(cls, path: Path) -> dict[str, Package]:
        '''Index the entries by PKGNAME'''
        catalogue: dict[str, Package] = {}
        for package in cls._entries(path):
            if 'PKGNAME' not in package:
                raise ValueError(f'entry without PKGNAME in {path}')
            name = package['PKGNAME']
            if name in catalogue:
                raise ValueError(f'duplicate PKGNAME {name} in {path}')
            if 'FILE_NAME' not in package or 'FILE_SIZE' not in package:
                raise ValueError(f'{name} has no FILE_NAME or FILE_SIZE in {path}')
            # The keys shared with FreeBSD: where the package is in the repository, and its size
            package['name'] = name
            package['repopath'] = f'{cls.PACKAGES_DIR}/{package["FILE_NAME"]}'
            package['pkgsize'] = int(package['FILE_SIZE'])
            catalogue[name] = package
        logger.debug('Loaded %d packages from %s', len(catalogue), path)
        return catalogue

    @classmethod
    def _index_sha512(cls, path: Path) -> dict[str, str]:
        '''Map each repository path in the SHA512 file to its lowercase hex digest, skipping other lines'''
        digests: dict[str, str] = {}
        with path.open(encoding='utf-8', errors='replace') as lines:
            for line in lines:
                match = cls.SHA512_LINE_RE.fullmatch(line.strip())
                if match:
                    digests[match.group(1)] = match.group(2).lower()
        logger.debug('Loaded %d SHA512 digests from %s', len(digests), path)
        return digests

    def load_catalogue(self, verify_tls: bool, ignore_verify: bool) -> None:
        '''Download and decompress pkg_summary.bz2 and SHA512.bz2 if needed, then index them

        SHA512.bz2 is optional, as some repositories do not have it, so packages are then not checked
        against a SHA512.
        '''
        summary_path = self._fetch_catalogue(verify_tls)
        sha512_path = self.catalogue_dir / self.SHA512_NAME
        if not sha512_path.is_file():
            if self._fetch_if_present(self.SHA512_RELPATH, verify_tls):
                Packages.decompress(self.branch_dir / self.SHA512_RELPATH, sha512_path)
            else:
                logger.warning('No %s in the repository, so packages are not checked against SHA512',
                               self.SHA512_RELPATH)
        # TODO: verify the signatures of pkg_summary.bz2 and SHA512.bz2, and honour ignore_verify
        self.catalogue = self._index(summary_path)
        self.sorted_names = sorted(self.catalogue)
        self.sha512s = self._index_sha512(sha512_path) if sha512_path.is_file() else None


REPOSITORY_TYPES: dict[OSType, type[BsdPackageRepositoryBase]] = {
    OSType.FREEBSD: FreeBsdPackageRepository,
    OSType.NETBSD: NetBsdPackageRepository,
}


class SystemRelease:
    '''An installation tarball, such as base.txz, from the FreeBSD release folder'''

    # Only FreeBSD is for now supported. If we want to support NetBSD, we shoudl
    # create a base class, and refactor similar to how we handle the package
    # repository.

    DEFAULT_PACKAGE = 'base.txz'

    def __init__(self, os_: OS, baseurl: str, sysroot: Path | None,
                 package: str = DEFAULT_PACKAGE, allow_archive: bool = True) -> None:
        self.os = os_
        dir_version = os_.major_version()
        self.sysroot = sysroot or os_.default_sysroot_dir(dir_version, os_.release_version())
        self.package = package
        arch = os_.values['arch']
        release_arch = os_.settings['arch_names'].get(arch, arch)
        archive_baseurl = os_.settings['release_baseurl_archive'] if allow_archive else None
        self.packages = Packages(baseurl, archive_baseurl, os_.os_dir(dir_version) / 'releases',
                                 f'{release_arch}/{os_.release_version()}-{os_.values["branch"]}')
        self.branch_dir = self.packages.dir

    def download(self, verify_tls: bool, extract: bool) -> None:
        '''Download the tarball into the release folder unless its SHA-256 file matches, then extract it into the sysroot

        The SHA-256 is written next to the tarball after each download. Extraction overwrites
        existing files in the sysroot.
        '''
        dest = self.branch_dir / self.package
        if Crypto.sha256_sidecar_matches(dest):
            logger.info('Reusing %s, SHA-256 matches', dest)
        else:
            Crypto.sha256_sidecar(dest).unlink(missing_ok=True)
            self.packages.fetch(self.package, verify_tls)
            Crypto.write_sha256_sidecar(dest)
        if extract:
            Packages.extract(dest, self.sysroot, Packages.allow_absolute_links)


def main(argv: Sequence[str] | None = None) -> int:
    '''Parse the arguments and coordinate the download'''
    os_parser = argparse.ArgumentParser(add_help=False)
    os_parser.add_argument('--os', default=OSType.FREEBSD.value.lower(), type=str.lower,
                           choices=[os_type.value.lower() for os_type in OSType],
                           help='operating system of the repository and releases '
                                '(default: %(default)s, ignoring case)')
    os_args, _ = os_parser.parse_known_args(argv)
    os_type = OSType.from_name(os_args.os)
    xbsd = OS.SETTINGS[os_type]

    parser = argparse.ArgumentParser(
        parents=[os_parser],
        description='Download packages from a FreeBSD or NetBSD package repository.')
    parser.add_argument('packages', nargs='*', metavar='package',
                        help='package name(s) to download, including dependencies; with --system, '
                             'the tarball name (default: base.txz)')
    parser.add_argument('--baseurl', default=None,
                        help=f'base URL of the package mirror (default: {xbsd["pkg_baseurl"]}, '
                             f'or {xbsd["release_baseurl"]} with --system)')
    parser.add_argument('--version', default=xbsd['default_version'],
                        help='OS version, such as 15 or 15.1 (default: %(default)s)')
    parser.add_argument('--arch', default=xbsd['default_arch'],
                        help='OS architecture (default: %(default)s)')
    parser.add_argument('--branch', default=None,
                        help=f'package branch, e.g. latest or quarterly, or for NetBSD the folder '
                             f'name in the repository, such as 11.0_2026Q2 '
                             f'(default: {xbsd["pkg_default_branch"] or "the release version"}, '
                             f'or {xbsd["release_default_branch"] or "not supported"} with --system)')
    parser.add_argument('--system', action='store_true',
                        help='download the installation tarball from the OS releases '
                             'instead of packages')
    parser.add_argument('--ignore-ca', action='store_true',
                        help='do not verify HTTPS certificates')
    parser.add_argument('--ignore-verify', action='store_true',
                        help='skip the packagesite.yaml signature and public key checks')
    parser.add_argument('--conf',
                        help='OS configuration file naming the trusted keys folder '
                             f'(default: {{sysroot}}/{FreeBsdPackageRepository.CONF_RELPATH.as_posix()})')
    parser.add_argument('--sysroot', default=None,
                        help='folder to extract packages into '
                             f'(default: {xbsd["dir_prefix"]}-{{major}}-{{arch}}/{xbsd["sysroot_dir_name"]}/{{version}}/{{branch}}, '
                             'where version is the full release with --system)')
    parser.add_argument('--no-extract', action='store_true',
                        help='download packages but do not extract them into the sysroot')
    parser.add_argument('--clean', action='store_true',
                        help='remove the catalogue and packages first; without package '
                             'names, exit after cleaning')
    parser.add_argument('--log-level', default='INFO', type=str.upper,
                        choices=LOG_LEVELS,
                        help='logging level (default: %(default)s)')
    args = parser.parse_args(argv)

    # Only the default mirror falls back to the archive; a --baseurl given by the user is used alone
    allow_archive = args.baseurl is None
    if args.baseurl is None:
        args.baseurl = xbsd['release_baseurl'] if args.system else xbsd['pkg_baseurl']
    # A local mirror has no archive to fall back to
    allow_archive = allow_archive and not Packages.is_file_url(args.baseurl)
    if args.system and os_type is OSType.NETBSD:
        parser.error('--system is not supported for NetBSD; only packages can be downloaded')
    if args.branch is None:
        args.branch = xbsd['release_default_branch'] if args.system else (
            xbsd['pkg_default_branch'] or OS.release_name(args.version))
    if args.system and len(args.packages) > 1:
        parser.error('--system takes at most one tarball name')
    if args.system and args.packages and Path(args.packages[0]).name != args.packages[0]:
        parser.error(f'tarball name must not include a folder: {args.packages[0]!r}')
    if not args.system and not args.packages and not args.clean:
        parser.error('at least one package is required unless --clean or --system is given')
    for option, value in (('--version', args.version), ('--arch', args.arch),
                          ('--branch', args.branch)):
        if not value or value in ('.', '..') or Path(value).name != value:
            parser.error(f'{option} must be a single folder name: {value!r}')

    logging.basicConfig(level=LOG_LEVELS[args.log_level],
                        format='%(asctime)s %(levelname)s %(message)s')

    verify = not args.ignore_ca
    if not verify:
        urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
        logger.warning('HTTPS certificate checks are disabled')

    os_ = OS(os_type, args.version, args.arch, args.branch)
    sysroot = Path(args.sysroot) if args.sysroot else None
    conf_path = Path(args.conf) if args.conf else None
    if args.system:
        package = args.packages[0] if args.packages else SystemRelease.DEFAULT_PACKAGE
        release = SystemRelease(os_, args.baseurl, sysroot, package, allow_archive)
        branch_dir = release.branch_dir
    else:
        repo = REPOSITORY_TYPES[os_type](os_, args.baseurl, sysroot, conf_path, allow_archive)
        branch_dir = repo.branch_dir

    created_root = None
    if not branch_dir.exists():
        created_root = branch_dir
        while not created_root.parent.exists():
            created_root = created_root.parent

    try:
        if args.clean:
            Packages.remove_tree(branch_dir)
            if not args.packages:
                return 0

        if args.system:
            release.download(verify, extract=not args.no_extract)
            return 0

        repo.load_catalogue(verify, args.ignore_verify)
        for package in repo.resolve(args.packages):
            repo.fetch(package, verify)
            if not args.no_extract:
                repo.extract(package)

    except (requests.RequestException, OSError, ValueError, zstandard.ZstdError,
            tarfile.TarError) as exc:
        logger.error('%s', exc)
        if created_root is not None and created_root.exists() and \
                Packages.remove_if_empty(created_root):
            logger.info('Removed empty %s', created_root)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
