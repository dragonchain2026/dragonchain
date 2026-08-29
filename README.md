Dragonchain
===========

**Version**: v0.6.0
**Genesis**: 2026-05-04 (Block Hash: `3c9761d390215d5e51843db7802845052ad8bdfad2014050b9cc6b82337089c2`)
**Based on**: Sugarchain Yumekawa v0.16.3 (Bitcoin Core fork)

Dragonchain is a post-quantum secure blockchain that integrates **Falcon-512** lattice-based signatures, replacing the original secp256k1 ECDSA. All wallet operations and transaction signing use Falcon-512 post-quantum cryptography.


Key Changes from Sugarchain
---------------------------

### Falcon-512 Post-Quantum Signatures

| Parameter | secp256k1 (Original) | Falcon-512 (Dragonchain) |
|-----------|---------------------|--------------------------|
| Private key | 32 bytes | **1281 bytes** |
| Public key | 33/65 bytes | **898 bytes** (897+1) |
| Signature | ~71 bytes DER | **≤ 690 bytes** |

Defined in `src/key.h`:
```cpp
#define PQCLEAN_FALCON512_CLEAN_CRYPTO_SECRETKEYBYTES_   1281
#define PQCLEAN_FALCON512_CLEAN_CRYPTO_PUBLICKEYBYTES_   897
#define PQCLEAN_FALCON512_CLEAN_CRYPTO_BYTES_            690
```

### New Genesis Block (2026-05-04)

| Parameter | Value |
|-----------|-------|
| Timestamp | 1777900800 |
| Message | "Dragonchain launched on 5 May 2026" |
| nBits | 0x1f3fffff |
| Block reward | 42.94967296 COIN |
| PoW algorithm | Yespower 1.0 (N=2048, r=32) |
| Address prefix | `dragon` (Bech32) |

### Notable Bug Fixes (v0.0.1.1)

- **Wallet password bug**: Hardcoded 32-byte key size check in `crypter.cpp` `DecryptKey()` prevented encrypted wallet unlock (commit `bf3fccd`)
- **Private key import/export**: `base58.cpp` `CBitcoinSecret::GetKey()` / `IsValid()` truncated Falcon-512 keys to 32 bytes
- **BIP32 HD derivation**: `CKey::Derive()` `assert(size()==32)` replaced with graceful failure for Falcon-512
- See `/home/zhonghuasheng/202605041/2/技术文档新/` for full technical documentation


Original Sugarchain Yumekawa
-----------------------------

Dragonchain is forked from Sugarchain Yumekawa. The name `Yumekawa (夢川)` can be translated as:
- "Yume (夢)" means dream and "Kawa (川)" means river: `Dream River`
- The second letter "Kawa" stands for "Kawaii (可愛い)": `Dreamy Cute`
- Yumekawa replaces the word `Core` (e.g. Bitcoin Core), which sounds a bit centralized


License
-------
Dragonchain is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.
- Copyright (c) 2009-2010 Satoshi Nakamoto
- Copyright (c) 2009-2018 The Bitcoin Core developers
- Copyright (c) 2013-2019 Alexander Peslyak - Yespower 1.0.1
- Copyright (c) 2016-2018 The Zcash developers - DigiShieldZEC
- Copyright (c) 2018-2020 The Dragonchain developers
- Copyright (c) 2026 The Dragonchain developers


Minimum Requirement
-------------------
- CPU: 1 Core
- RAM: 2048 MB (at least 3 GB [swap](https://github.com/dragonchain-project/doc/blob/master/swap.md))
- DISK: HDD 5 GB


Depends on Bitcoin Core
-----------------------
Exactly the same as dependencies of [Bitcoin Core v0.16.3](https://github.com/bitcoin/bitcoin/tree/49e34e288005a5b144a642e197b628396f5a0765).

- Debian 10 (Recommended, No PPA)
```bash
sudo apt-get install -y \
software-properties-common build-essential libtool autotools-dev automake pkg-config \
libssl-dev libevent-dev bsdmainutils libboost-all-dev \
libminiupnpc-dev libzmq3-dev libqt5gui5 libqt5core5a \
libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev \
protobuf-compiler libqrencode-dev help2man
```

- PPA is *only* for Ubuntu. No `libdb4.8-dev` and `libdb4.8++-dev` packages on Debian.

- <details><summary>Old Ubuntu</summary>

  * Ubuntu 16.04
  ```bash
  sudo add-apt-repository -y ppa:bitcoin/bitcoin && \
  sudo apt-get update && \
  sudo apt-get install -y \
  libdb4.8-dev libdb4.8++-dev \
  software-properties-common build-essential libtool autotools-dev automake pkg-config \
  libssl-dev libevent-dev bsdmainutils libboost-all-dev \
  libminiupnpc-dev libzmq3-dev libqt5gui5 libqt5core5a \
  libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev \
  protobuf-compiler libqrencode-dev help2man
  ```

  * Ubuntu 18.04+
  ```bash
  sudo add-apt-repository -y ppa:luke-jr/bitcoincore && \
  sudo apt-get update && \
  sudo apt-get install -y \
  libdb4.8-dev libdb4.8++-dev \
  software-properties-common build-essential libtool autotools-dev automake pkg-config \
  libssl-dev libevent-dev bsdmainutils libboost-all-dev \
  libminiupnpc-dev libzmq3-dev libqt5gui5 libqt5core5a \
  libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev \
  protobuf-compiler libqrencode-dev help2man
  ```
</details>


Build
-----
- Debian 10+ (Recommended, No PPA)
```bash
./autogen.sh && \
./contrib/install_db4.sh `pwd` && \
export BDB_PREFIX=$PWD/db4 && \
./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" && \
make -j$(nproc) && \
make check -j$(nproc)
```

- (optional) Following can be deleted `rm -rf db4/ && rm -f db-4.8.30.NC.tar.gz`

- <details><summary>Old Ubuntu</summary>

  * Ubuntu 16.04+
  ```bash
  ./autogen.sh && \
  ./configure && \
  make -j$(nproc) && \
  make check -j$(nproc)
  ```
</details>


Options after Build
-------------------
- (optional) Reduce binary size using strip (about 90% file size reduction)
```bash
strip ./src/dragonchain-cli && \
strip ./src/dragonchaind && \
strip ./src/qt/dragonchain-qt && \
strip ./src/dragonchain-tx && \
strip ./src/test/test_dragonchain
```

- (optional) After bump version on `configure.ac`, update binary docs (manpages) using help2man `.1` files
```bash
make -j$(nproc) && ./contrib/devtools/gen-manpages.sh
```

- (optional) build for Windows and OSX you may need `--disable-shared` option with make.

- (optional) Add seeds/nodes from [DNSSEED](https://github.com/dragonchain-project/dragonchain-seeder)  
  https://github.com/dragonchain-project/dragonchain/tree/master-v0.16.3/contrib/seeds


Unit Test
---------
All Dragonchain developers should execute this unit test. Some updates may break these tests in some occasions.

- Test All
```bash
./src/test/test_dragonchain test_bitcoin --log_level=test_suite
```

- (optional) Test Partially: ie `blockencodings_tests`
```bash
./src/test/test_dragonchain test_bitcoin --log_level=test_suite --run_test=blockencodings_tests
```

- (optional) Test QT (GUI)
```bash
./src/qt/test/test_dragonchain-qt
```


Run
---
The options `-rpcuser`, `-rpcpassword`, and `-printtoconsole` are optional. `server=1` needed by RPC servers or cpuminer when solo-mining.

- Mainnet: debug mode: `net` for Network
  > ./src/qt/dragonchain-qt -server=1 -rpcuser=rpcuser -rpcpassword=rpcpassword **-debug=net** -printtoconsole

- Testnet
  > ./src/qt/dragonchain-qt **-testnet**

- Regtest
  > ./src/qt/dragonchain-qt **-regtest**

- Reference  
  https://en.bitcoin.it/w/index.php?title=Running_Bitcoin&oldid=66644


CLI
---
- `-prunedebuglogfile`: Prune (limit) filesize of debug.log
  > ./src/qt/dragonchain-qt -prunedebuglogfile

  > 2020-09-15 19:41:34 DEBUG.LOG PRUNED at 10000063


Known Issues
------------
- Transaction too large:
  * This is a part of BTC, and hopefully will be fixed in next *Taproot* Softfork.
- Slow update balance on wallet:
  * This slow is a part of BTC.
  * Update total balance *every minute (12 blocks)* interval.
  * A workaround at this moment. [source](https://github.com/dragonchain-project/dragonchain/commit/72436c90b29844cf507895df053103f9b6840776#diff-2e3836af182cfb375329c3463ffd91f8)
- Poor performance on ARM CPUs (32/64-Bit):
  * No ARM optimization for Yespower yet.
- Poor performance on 32-Bit OS:
  * No SSE2 optimization for Yespower yet. [source](https://github.com/dragonchain-project/dragonchain/blob/d977987a83aba115d50a9130f0d7914330d1bc75/src/crypto/yespower-1.0.1/yespower-opt.c#L59)
- Slow startup on low memory machines:
  * Startup can take up to some hours on 1cpu 1024ram (+swap 3GB) VPS.
  * Workaround is just increase RAM at least 2 GB.


Release Process using GITIAN
----------------------------
- All Dragonchain developers should do following GITIAN release process. It's the safest way to distribute binaries to people.
- Please use GITIAN release with checking PGP signature, or compile it yourself on your own machine.

https://gist.github.com/cryptozeny/3501c77750541208b9dd1a9e9719fc53
