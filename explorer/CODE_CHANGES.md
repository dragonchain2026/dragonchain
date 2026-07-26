# Dragonchain 区块浏览器代码修改对比文档

本文档详细记录部署调试过程中所有代码文件的修改内容。

---

## 1. settings.json - 主配置文件

**文件路径**: `/path/to/explorer/settings.json`

### 1.1 钱包 RPC 配置

**修改前**:
```json
"wallet": {
  "host": "localhost",
  "port": 9332,
  "username": "[需要配置]",
  "password": "[需要配置]"
},
```

**修改后**:
```json
"wallet": {
  "host": "localhost",
  "port": 9332,
  "username": "your_rpc_username",
  "password": "your_rpc_password"
},
```

---

### 1.2 API 配置

**修改前**:
```json
"api": {
  "blockindex": 1,
  "blockhash": "[需要配置]",
  "txhash": "[需要配置]",
  "address": "[需要配置]"
},
```

**修改后**:
```json
"api": {
  "blockindex": 1,
  "blockhash": "3c9761d390215d5e51843db7802845052ad8bdfad2014050b9cc6b82337089c2",
  "txhash": "79d2d2cf340893bc0f5df7cc596ba52fa979727f759f8feef347268414ff5fac",
  "address": "SUa97qdpNxXSkcMGMxBpkw3qYweF1KTkbA"
},
```

---

### 1.3 创世区块配置

**修改前**:
```json
"genesis_tx": "[需要配置]",
"genesis_block": "[需要配置]",
```

**修改后**:
```json
"genesis_tx": "79d2d2cf340893bc0f5df7cc596ba52fa979727f759f8feef347268414ff5fac",
"genesis_block": "3c9761d390215d5e51843db7802845052ad8bdfad2014050b9cc6b82337089c2",
```

---

### 1.4 数据库配置

**修改前**:
```json
"dbsettings": {
  "user": "iquidus",
  "password": "your_db_password",
  "database": "explorerdb",
  "address": "localhost",
  "port": 27017
},
```

**修改后**:
```json
"dbsettings": {
  "user": "",
  "password": "",
  "database": "explorerdb",
  "address": "localhost",
  "port": 27017
},
```

**说明**:
- 本地 MongoDB 默认未启用认证，因此清空用户名密码
- 生产环境建议启用认证并配置相应的用户权限

---

## 2. lib/database.js - 数据库操作库（关键修复）

**文件路径**: `/path/to/explorer/lib/database.js`

### 2.1 update_db 函数中的区块高度判断问题

**修改位置**: 第 685 行

**问题描述**:
当区块链高度为 0（仅创世区块）时，`if (!count)` 会错误地返回 true。因为在 JavaScript 中，`0` 是 falsy 值，导致代码误判为 RPC 连接失败。

**修改前**:
```javascript
update_db: function(coin, cb) {
  lib.get_blockcount( function (count) {
    if (!count){
      console.log('Unable to connect to explorer API');
      return cb(false);
    }
    lib.get_supply( function (supply){
      lib.get_connectioncount(function (connections) {
        Stats.findOneAndUpdate({coin: coin}, {
          $set: {
            coin: coin,
            count : count,
            supply: supply,
            connections: connections
          }
        }, {
          new: true
        }, function(err, new_stats) {
          if(err) {
            console.log("Error during Stats Update:", err);
          }
          return cb({coin: coin,
            count : count,
            supply: supply,
            connections: connections,
            last: (new_stats.last ? new_stats.last : 0)});
        });
      });
    });
  });
},
```

**修改后**:
```javascript
update_db: function(coin, cb) {
  lib.get_blockcount( function (count) {
    if (count === null || count === undefined){
      console.log('Unable to connect to explorer API');
      return cb(false);
    }
    lib.get_supply( function (supply){
      lib.get_connectioncount(function (connections) {
        Stats.findOneAndUpdate({coin: coin}, {
          $set: {
            coin: coin,
            count : count,
            supply: supply,
            connections: connections
          }
        }, {
          new: true
        }, function(err, new_stats) {
          if(err) {
            console.log("Error during Stats Update:", err);
          }
          return cb({coin: coin,
            count : count,
            supply: supply,
            connections: connections,
            last: (new_stats.last ? new_stats.last : 0)});
        });
      });
    });
  });
},
```

**修改说明**:

| 变更项 | 修改前 | 修改后 |
|--------|--------|--------|
| 判断条件 | `if (!count)` | `if (count === null || count === undefined)` |
| 判断逻辑 | 任何 falsy 值（0, '', null, undefined）都视为失败 | 仅当真正的 null 或 undefined 时才视为失败 |
| 0 高度处理 | 错误地判定为失败 | 正确识别为有效高度 |

---

## 3. dragonchain.conf - Dragonchain 节点配置

**文件路径**: `~/.dragonchain/dragonchain.conf`

**完整配置内容**:
```ini
server=1
txindex=1
rpcuser=your_rpc_username
rpcpassword=your_rpc_password
rpcallowip=127.0.0.1
rpcport=9332
listen=1
```

**配置说明**:

| 配置项 | 值 | 说明 |
|--------|-----|------|
| server | 1 | 启用 RPC 服务器 |
| txindex | 1 | 启用交易索引（浏览器必需） |
| rpcuser | your_rpc_username | RPC 用户名 |
| rpcpassword | your_rpc_password | RPC 密码 |
| rpcallowip | 127.0.0.1 | 允许连接的 IP（仅本地） |
| rpcport | 9332 | RPC 端口号 |
| listen | 1 | 启用 P2P 网络监听 |

---

## 4. 新增文件清单

### 4.1 test_rpc.js - RPC 测试脚本（临时）

**文件路径**: `/path/to/explorer/test_rpc.js`

**内容**:
```javascript
var settings = require('./lib/settings');
const Client = require('bitcoin-core');
const client = new Client(settings.wallet);

console.log('Testing RPC connection...');
console.log('Settings:', settings.wallet);

client.command([{method: 'getblockcount', parameters: []}], function(err, response) {
  if (err) {
    console.error('Error:', err);
  } else {
    console.log('Response:', response);
  }
});
```

---

### 4.2 get_genesis_info.sh - 获取创世区块信息脚本

**文件路径**: `/path/to/explorer/get_genesis_info.sh`

**内容**:
```bash
#!/bin/bash

# Dragonchain Explorer - 获取创世区块信息脚本
# 使用方法: ./get_genesis_info.sh [rpc-host] [rpc-port] [rpc-user] [rpc-password]

RPC_HOST=${1:-localhost}
RPC_PORT=${2:-9332}
RPC_USER=${3:-your_rpc_username}
RPC_PASS=${4:-your_rpc_password}

echo "=========================================="
echo "Dragonchain 创世区块信息获取工具"
echo "=========================================="
echo ""
echo "RPC 连接信息:"
echo "  Host: $RPC_HOST"
echo "  Port: $RPC_PORT"
echo "  User: $RPC_USER"
echo ""

# 函数: 调用 RPC
rpc_call() {
  local method=$1
  shift
  local params=$*

  curl -s -u $RPC_USER:$RPC_PASS \
    -H 'Content-Type: application/json' \
    -d "{\"jsonrpc\":\"1.0\",\"id\":\"1\",\"method\":\"$method\",\"params\":[$params]}" \
    http://$RPC_HOST:$RPC_PORT/
}

echo "步骤 1: 获取创世区块哈希 (高度 0)..."
GENESIS_BLOCK_HASH=$(rpc_call getblockhash 0 | grep -o '"result":"[^"]*"' | cut -d'"' -f4)

if [ -z "$GENESIS_BLOCK_HASH" ]; then
  echo "错误: 无法获取创世区块哈希!"
  echo "请检查:"
  echo "  1. Dragonchaind 是否正在运行?"
  echo "  2. 是否开启了 -server 和 RPC?"
  echo "  3. 用户名密码是否正确?"
  exit 1
fi

echo "  ✓ 创世区块哈希: $GENESIS_BLOCK_HASH"
echo ""

echo "步骤 2: 获取创世区块详细信息..."
GENESIS_BLOCK=$(rpc_call getblock "\"$GENESIS_BLOCK_HASH\"" 2)
GENESIS_TX=$(echo "$GENESIS_BLOCK" | grep -o '"tx":\["[^"]*"' | cut -d'"' -f4)

echo "  ✓ 创世交易哈希: $GENESIS_TX"
echo ""

echo "步骤 3: 获取一个测试地址..."
# 获取或生成一个测试地址（为了示例）
TEST_ADDRESS=$(rpc_call getnewaddress | grep -o '"result":"[^"]*"' | cut -d'"' -f4)

if [ -z "$TEST_ADDRESS" ]; then
  TEST_ADDRESS="请替换为你钱包中的一个有效地址"
fi

echo "  ✓ 测试地址: $TEST_ADDRESS"
echo ""

echo "=========================================="
echo "settings.json 配置参考"
echo "=========================================="
echo ""
echo "请将以下内容复制到 settings.json 中:"
echo ""
echo "  \"genesis_tx\": \"$GENESIS_TX\","
echo "  \"genesis_block\": \"$GENESIS_BLOCK_HASH\","
echo ""
echo "  \"api\": {"
echo "    \"blockindex\": 1,"
echo "    \"blockhash\": \"$GENESIS_BLOCK_HASH\","
echo "    \"txhash\": \"$GENESIS_TX\","
echo "    \"address\": \"$TEST_ADDRESS\""
echo "  }"
echo ""
echo "=========================================="
echo ""
echo "提示: 记得在 settings.json 中配置正确的 RPC 用户名和密码!"
echo ""
```

---

## 5. 修改统计

| 项目 | 数量 |
|------|------|
| 修改的文件 | 3 个 |
| 新增的文件 | 3 个 |
| 代码提交建议 | 1 个 bug 修复 + 配置更新 |

---

## 6. 关键 Bug 修复说明

### 问题 ID: BUG-001

| 项目 | 内容 |
|------|------|
| 标题 | 创世区块高度为 0 时同步失败 |
| 严重性 | 高 |
| 影响 | 新区块链无法完成首次同步 |
| 原因 | JavaScript 中 falsy 值判断不当 |
| 修复方式 | 明确判断 null/undefined |
| 修复文件 | lib/database.js |
| 修复日期 | 2026-05-07 |

---

## 7. Git 提交建议

如果要将这些修改提交到 Git，建议的提交信息：

```
fix: 处理创世区块高度为 0 时的同步问题

- 修改 database.js 中的 update_db 函数
- 将 if (!count) 改为 if (count === null || count === undefined)
- 允许区块高度 0 被正确处理

config: 更新 Dragonchain 浏览器配置

- 配置 RPC 连接信息
- 配置创世区块哈希和交易哈希
- 调整 MongoDB 认证配置（本地无需认证）
- 添加 Dragonchain 符号和名称
```
