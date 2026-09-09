# Third-Party Notices

Dragonchain is built on top of and links against the following third-party
libraries. This file lists their upstream source locations and licenses so that
distributing the pre-built binaries (which include or link these libraries)
remains compliant.

## Runtime libraries bundled in the Windows archive

These DLLs are placed next to `dragonchain-qt.exe` / `dragonchaind.exe` in
`dragonchain-windows-x86_64.tar.gz`.

| Library | Version | License | Upstream source |
|---------|---------|---------|-----------------|
| `libgcc_s_seh-1.dll` | from MinGW-w64 GCC | GPLv3 with the **GCC Runtime Library Exception** | https://gcc.gnu.org/ , https://www.mingw-w64.org/ |
| `libstdc++-6.dll` | from MinGW-w64 GCC | GPLv3 with the **GCC Runtime Library Exception** | https://gcc.gnu.org/ , https://www.mingw-w64.org/ |
| `libwinpthread-1.dll` | from MinGW-w64 | MinGW-w64 runtime license (permissive) | https://www.mingw-w64.org/ |

The GCC Runtime Library Exception allows these runtime DLLs to be redistributed
alongside a program without imposing the GPL on that program.

## Statically linked libraries (no separate DLL shipped)

These are compiled into the binaries via the `depends/` build system
(`--disable-shared`), so no standalone DLL is distributed for them.

| Library | Version | License | Upstream source |
|---------|---------|---------|-----------------|
| Berkeley DB (BDB) | 4.8.30 | Sleepycat License (permissive; the last BDB version before 6.x switched to AGPL) | https://www.oracle.com/database/technologies/related/berkeleydb-downloads.html |
| libqrencode | 3.4.4 | LGPL-2.1 | https://github.com/fukuchi/libqrencode |
| Qt | 5.15.2 | LGPLv3 / GPLv3 / commercial | https://www.qt.io/ |
| Boost | 1.77 | Boost Software License 1.0 | https://www.boost.org/ |
| OpenSSL | (depends build) | OpenSSL License | https://www.openssl.org/ |
| libevent | (depends build) | BSD-3-Clause | https://libevent.org/ |
| protobuf | (depends build) | BSD-3-Clause | https://github.com/protocolbuffers/protobuf |
| miniupnpc | (depends build) | BSD-3-Clause | https://miniupnp.tuxfamily.org/ |
| zlib | (depends build) | zlib License | https://zlib.net/ |

## Falcon-512 post-quantum signatures

Dragonchain replaces secp256k1 ECDSA with Falcon-512. The Falcon implementation
is derived from the PQClean project.

| Library | License | Upstream source |
|---------|---------|-----------------|
| Falcon-512 (Falcon Project / PQClean) | MIT | https://falcon-sign.info/ , https://github.com/PQClean/PQClean |

## Notes

- Only the Windows archive ships the three MinGW runtime DLLs listed above;
  everything else is statically linked or provided by the operating system.
- Linux and macOS archives are built against system/Homebrew packages; see the
  corresponding package managers for their license terms.
- This notice is informational. For the full license text of each library,
  follow the upstream links above.
