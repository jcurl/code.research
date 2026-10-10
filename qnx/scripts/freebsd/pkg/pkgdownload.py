#!/usr/bin/env python3

'''Download packages from a FreeBSD or NetBSD package repository.

The repository for a given OS, version, architecture and branch is located at
<baseurl>/FreeBSD:<version>:<arch>/<branch>/ for FreeBSD. For a file:// baseurl, the
repository is located at <baseurl>/<branch>/ and the version and architecture are not used.

With --system, the installation tarball (base.txz by default) is downloaded from the OS
release folder into system/<version>/<branch>/ and extracted into the sysroot, instead of downloading packages.
'''

import argparse
import hashlib
import json
import logging
import lzma
import os
import re
import shutil
import sys
import tarfile
from collections import deque
from collections.abc import Callable, Sequence
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
    '''Paths and URLs for one operating system, version, architecture and branch'''

    SETTINGS: dict[OSType, dict[str, Any]] = {
        OSType.FREEBSD: {
            'dir_prefix': 'freebsd',
            'default_version': '15',
            'default_arch': 'aarch64',
            'default_branch': 'latest',
            'pkg_baseurl': 'https://pkg.freebsd.org/',
            'repo_dir_format': 'FreeBSD:{version}:{arch}',
            'release_baseurl': 'http://ftp.freebsd.org/pub/FreeBSD/releases',
            'archive_release_baseurl': 'http://ftp-archive.freebsd.org/pub/FreeBSD-Archive/old-releases',
            'archive_package_baseurl': 'https://mirror.sg.gs/freebsd-pkg',
            'conf_relpath': Path('etc/pkg/FreeBSD.conf'),
            'sysroot_dir_name': 'sysroot',
            # Release folders use a different name for some architectures. Others are used unchanged.
            'arch_names': {'aarch64': 'arm64', 'amd64': 'amd64'},
        },
        # TODO: placeholders copied from FreeBSD, replace with the NetBSD layout after testing
        OSType.NETBSD: {
            'dir_prefix': 'netbsd',
            'default_version': '10',
            'default_arch': 'aarch64',
            'default_branch': 'latest',
            'pkg_baseurl': 'https://pkg.netbsd.example.com/',
            'repo_dir_format': 'NetBSD:{version}:{arch}',
            'release_baseurl': 'https://ftp.netbsd.example.com/pub/NetBSD/releases',
            'archive_release_baseurl':
                'https://ftp-archive.netbsd.example.com/pub/NetBSD-Archive/old-releases',
            'archive_package_baseurl': 'https://pkg-archive.netbsd.example.com/NetBSD-Archive/packages',
            'conf_relpath': Path('etc/pkg/NetBSD.conf'),
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
        '''The settings for this operating system'''
        return self._settings

    def branch_dir(self) -> Path:
        '''Return the folder holding the catalogue and packages, under the major version folder'''
        return self.major_dir() / 'pkg' / self._values['branch']

    def major_version(self) -> str:
        '''Return the major version, such as 15 for 15.1, used by the package repository'''
        return self.release_version().split('.')[0]

    def major_dir(self) -> Path:
        '''Return the folder for the major version and arch, shared by all releases of that major version'''
        prefix = self._settings['dir_prefix']
        return Path(f'{prefix}-{self.major_version()}-{self._values["arch"]}')

    def system_branch_dir(self) -> Path:
        '''Return the folder for system files, system/<version>/<branch> under the major version folder'''
        return self.major_dir() / 'system' / self.release_version() / self._values['branch']

    def default_sysroot_dir(self, version: str) -> Path:
        '''Return the default sysroot, sysroot/<version>/<branch> under the major version folder'''
        return self.major_dir() / self._settings['sysroot_dir_name'] / version / self._values['branch']

    def release_version(self) -> str:
        '''Return the full release version, where a bare major version such as 15 means 15.0'''
        version = self._values['version']
        return version if '.' in version else f'{version}.0'

    def conf_path(self, sysroot: Path) -> Path:
        '''Return the default OS configuration path under sysroot, FreeBSD.conf for FreeBSD'''
        return sysroot / self._settings['conf_relpath']

    def repo_url(self, baseurl: str, archive: bool = False) -> str:
        '''Return the package repository URL, or the archived one when archive is True

        The repository is named by the major version, so 15.1 uses the FreeBSD:15 repository.
        '''
        base = self._settings['archive_package_baseurl'] if archive else baseurl
        arch, branch = (self._values[key] for key in ('arch', 'branch'))
        repo_dir = self._settings['repo_dir_format'].format(version=self.major_version(), arch=arch)
        return f'{base.rstrip("/")}/{repo_dir}/{branch}/'

    def release_url(self, baseurl: str, name: str, archive: bool = False) -> str:
        '''Return the URL of a release file such as base.txz, or the archived one when archive is True

        The URL is <base>/<release arch>/<version>-<branch>/<name>.
        '''
        base = self._settings['archive_release_baseurl'] if archive else baseurl
        arch = self._values['arch']
        release_arch = self._settings['arch_names'].get(arch, arch)
        return (f'{base.rstrip("/")}/{release_arch}/'
                f'{self.release_version()}-{self._values["branch"]}/{name}')

    def fingerprint_dirs(self, conf_path: Path) -> tuple[Path, Path] | None:
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
        conf_root = conf_path.resolve().parents[len(self._settings['conf_relpath'].parts) - 1]
        keys_dir = conf_root / match.group(1).lstrip('/')
        trusted_dir = keys_dir / 'trusted'
        if not trusted_dir.is_dir():
            raise ValueError(f'no trusted keys directory {trusted_dir} (from {conf_path})')
        return trusted_dir, keys_dir / 'revoked'


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
    '''Download and extraction of files, as static methods'''

    DOWNLOAD_TIMEOUT = 60
    ZSTD_MAGIC = b'\x28\xb5\x2f\xfd'
    XZ_MAGIC = b'\xfd7zXZ\x00'

    @staticmethod
    def is_file_url(url: str) -> bool:
        '''Return True when the URL is a file:// local path'''
        return urlsplit(url).scheme == 'file'

    @staticmethod
    def remove_if_empty(path: Path) -> bool:
        '''Remove the folder tree at path if it holds no files, only folders; return True if removed'''
        for root, dirs, files in os.walk(path):
            if files or any(os.path.islink(os.path.join(root, d)) for d in dirs):
                return False
        shutil.rmtree(path)
        return True

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
            with requests.get(url, stream=True, timeout=Packages.DOWNLOAD_TIMEOUT,
                              verify=verify_tls) as response:
                response.raise_for_status()
                with dest.open('wb') as out:
                    for chunk in response.iter_content(chunk_size=64 * 1024):
                        out.write(chunk)
        logger.info('Downloaded %s', dest)

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
    def extract(archive: Path, dest_dir: Path,
                extraction_filter: str | Callable[[tarfile.TarInfo, str], tarfile.TarInfo | None]
                = 'data') -> None:
        '''Extract the zstd- or xz-compressed tar archive into dest_dir, overwriting existing files

        The compression is chosen from the file's magic bytes, as FreeBSD 13 packages use xz.
        Root-level '+' files (package metadata) are skipped.
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
            else:
                raise ValueError(f'{archive} is neither zstd nor xz compressed')
            dest_dir.mkdir(parents=True, exist_ok=True)
            with decompressed as reader, tarfile.open(fileobj=reader, mode='r|') as tar:
                members = (member for member in tar if not Packages._is_root_metadata(member))
                tar.extractall(dest_dir, members=members, filter=extraction_filter)
        logger.info('Extracted %s to %s', archive, dest_dir)

    @staticmethod
    def _is_root_metadata(member: tarfile.TarInfo) -> bool:
        '''Return True for root-level package metadata such as +MANIFEST'''
        name = member.name.removeprefix('./')
        return '/' not in name and name.startswith('+')


class PackageRepository:
    '''The package catalogue and packages for one version, architecture and branch

    Packages are saved under the branch folder, mirroring their repopath, and extracted into the sysroot.
    '''

    CATALOGUE_DIR_NAME = 'catalogue'
    PACKAGESITE_NAME = 'packagesite.pkg'
    PACKAGESITE_YAML_NAME = 'packagesite.yaml'
    PACKAGESITE_PUB_NAME = f'{PACKAGESITE_YAML_NAME}.pub'
    PACKAGESITE_SIG_NAME = f'{PACKAGESITE_YAML_NAME}.sig'

    def __init__(self, os_: OS, baseurl: str, sysroot: Path, conf_path: Path) -> None:
        self.os = os_
        self.sysroot = sysroot
        self.conf_path = conf_path
        self.branch_dir = os_.branch_dir()
        self.catalogue_dir = self.branch_dir / self.CATALOGUE_DIR_NAME
        self.packagesite = self.branch_dir / self.PACKAGESITE_NAME
        if Packages.is_file_url(baseurl):
            self.repo_url = f'{baseurl.rstrip("/")}/{os_.values["branch"]}/'
            self.archive_repo_url: str | None = None
        else:
            self.repo_url = os_.repo_url(baseurl)
            self.archive_repo_url = os_.repo_url(baseurl, archive=True)
        self.catalogue: dict[str, Package] = {}

    @staticmethod
    def clean(branch_dir: Path) -> None:
        '''Remove the folder with the catalogue and downloaded packages'''
        if not branch_dir.exists():
            logger.info('Nothing to clean, %s does not exist', branch_dir)
            return
        shutil.rmtree(branch_dir)
        logger.info('Removed %s', branch_dir)

    def _download(self, relpath: str, dest: Path, verify_tls: bool) -> None:
        '''Download relpath from the repository to dest, retrying once from the archive if the download fails

        Once the archive is used, it is used for the rest of the run, as the release has moved there.
        '''
        try:
            Packages.file(f'{self.repo_url}{relpath}', dest, verify_tls)
        except requests.RequestException as exc:
            if self.archive_repo_url is None or self.repo_url == self.archive_repo_url:
                raise
            logger.warning('Download failed (%s), trying the archive', exc)
            self.repo_url = self.archive_repo_url
            Packages.file(f'{self.repo_url}{relpath}', dest, verify_tls)

    def load_catalogue(self, verify_tls: bool, ignore_verify: bool) -> None:
        '''Download and extract packagesite.pkg if needed, verify it, then index it'''
        self.catalogue_dir.mkdir(parents=True, exist_ok=True)
        yaml_path = self.catalogue_dir / self.PACKAGESITE_YAML_NAME
        if yaml_path.is_file():
            logger.info('Catalogue already extracted in %s', self.catalogue_dir)
        else:
            self._download(self.PACKAGESITE_NAME, self.packagesite, verify_tls)
            Packages.extract(self.packagesite, self.catalogue_dir)
        if ignore_verify:
            logger.warning('catalogue signature check is disabled')
        else:
            pub_path = self.catalogue_dir / self.PACKAGESITE_PUB_NAME
            Crypto.verify_signature(yaml_path, pub_path,
                                    self.catalogue_dir / self.PACKAGESITE_SIG_NAME)
            key_dirs = self.os.fingerprint_dirs(self.conf_path)
            if key_dirs is not None:
                Crypto.verify_key_trust(pub_path, *key_dirs)
        self.catalogue = self._index(yaml_path)

    def resolve(self, names: Sequence[str]) -> list[Package]:
        '''Return the requested packages followed by all their dependencies, without duplicates'''
        resolved: dict[str, Package] = {}
        pending = deque(names)
        while pending:
            name = pending.popleft()
            if name in resolved:
                continue
            package = self.catalogue.get(name)
            if package is None:
                raise ValueError(f'package not found in catalogue: {name}')
            resolved[name] = package
            # 'deps' is an object keyed by dependency name
            pending.extend(package.get('deps', {}))
        return list(resolved.values())

    def fetch(self, package: Package, verify_tls: bool) -> None:
        '''Download the package unless a matching copy already exists, then verify the download'''
        dest = self._package_path(package)
        if Crypto.package_mismatch(dest, package) is None:
            logger.info('Already downloaded %s', dest)
            return
        dest.parent.mkdir(parents=True, exist_ok=True)
        self._download(package["repopath"], dest, verify_tls)
        problem = Crypto.package_mismatch(dest, package)
        if problem is not None:
            raise ValueError(f'verification failed for {package["name"]}: {problem}')

    def extract(self, package: Package) -> None:
        '''Extract the downloaded package into the sysroot'''
        Packages.extract(self._package_path(package), self.sysroot)

    def _package_path(self, package: Package) -> Path:
        '''Return where the package is saved, mirroring its repopath under the branch folder'''
        parts = PurePosixPath(package['repopath']).parts
        if not parts or parts[0] == '/' or '..' in parts:
            raise ValueError(f'invalid repository path: {package["repopath"]}')
        return self.branch_dir.joinpath(*parts)

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


class SystemRelease:
    '''An installation tarball, such as base.txz, from the FreeBSD release folder'''

    SYSTEM_DIR_NAME = 'system'
    DEFAULT_PACKAGE = 'base.txz'

    def __init__(self, os_: OS, baseurl: str, sysroot: Path,
                 package: str = DEFAULT_PACKAGE) -> None:
        self.os = os_
        self.sysroot = sysroot
        self.package = package
        self.archive_url: str | None = None
        if Packages.is_file_url(baseurl):
            branch = os_.values['branch']
            self.url = f'{baseurl.rstrip("/")}/{branch}/{self.SYSTEM_DIR_NAME}/{package}'
        else:
            self.url = os_.release_url(baseurl, package)
            self.archive_url = os_.release_url(baseurl, package, archive=True)

    def fetch(self, dest: Path, verify_tls: bool) -> None:
        '''Download the tarball to dest, retrying once from the archive if the release download fails

        Old releases move to the archive, so a 404 from the release folder is retried there.
        '''
        try:
            Packages.file(self.url, dest, verify_tls)
        except requests.RequestException as exc:
            if self.archive_url is None:
                raise
            logger.warning('Download failed (%s), trying the archive', exc)
            Packages.file(self.archive_url, dest, verify_tls)

    def download(self, branch_dir: Path, verify_tls: bool, extract: bool) -> None:
        '''Download the tarball into branch_dir unless its SHA-256 file matches, then extract it into the sysroot

        The SHA-256 is written next to the tarball after each download. Extraction overwrites
        existing files in the sysroot.
        '''
        dest = branch_dir / self.package
        dest.parent.mkdir(parents=True, exist_ok=True)
        if Crypto.sha256_sidecar_matches(dest):
            logger.info('Reusing %s, SHA-256 matches', dest)
        else:
            Crypto.sha256_sidecar(dest).unlink(missing_ok=True)
            self.fetch(dest, verify_tls)
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
    xbsd = OS.SETTINGS[OSType.from_name(os_args.os)]

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
                        help='OS major version (default: %(default)s)')
    parser.add_argument('--arch', default=xbsd['default_arch'],
                        help='OS architecture (default: %(default)s)')
    parser.add_argument('--branch', default=None,
                        help=f'package branch, e.g. latest or quarterly '
                             f'(default: {xbsd["default_branch"]}, or RELEASE with --system)')
    parser.add_argument('--system', action='store_true',
                        help='download the installation tarball from the OS releases '
                             'instead of packages')
    parser.add_argument('--ignore-ca', action='store_true',
                        help='do not verify HTTPS certificates')
    parser.add_argument('--ignore-verify', action='store_true',
                        help='skip the packagesite.yaml signature and public key checks')
    parser.add_argument('--conf',
                        help='OS configuration file naming the trusted keys folder '
                             f'(default: {{sysroot}}/{xbsd["conf_relpath"].as_posix()})')
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

    if args.baseurl is None:
        args.baseurl = xbsd['release_baseurl'] if args.system else xbsd['pkg_baseurl']
    if args.branch is None:
        args.branch = 'RELEASE' if args.system else xbsd['default_branch']
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

    os_ = OS(OSType.from_name(args.os), args.version, args.arch, args.branch)
    branch_dir = os_.system_branch_dir() if args.system else os_.branch_dir()
    sysroot = Path(args.sysroot) if args.sysroot else os_.default_sysroot_dir(
        os_.release_version() if args.system else args.version)
    conf_path = Path(args.conf) if args.conf else os_.conf_path(sysroot)

    created_root = None
    if not branch_dir.exists():
        created_root = branch_dir
        while not created_root.parent.exists():
            created_root = created_root.parent

    try:
        if args.clean:
            PackageRepository.clean(branch_dir)
            if not args.packages:
                return 0
        if args.system:
            package = args.packages[0] if args.packages else SystemRelease.DEFAULT_PACKAGE
            SystemRelease(os_, args.baseurl, sysroot, package).download(
                branch_dir, verify, extract=not args.no_extract)
            return 0
        repo = PackageRepository(os_, args.baseurl, sysroot, conf_path)
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
