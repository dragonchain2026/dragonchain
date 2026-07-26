# Dragonchain 区块链浏览器部署调试文档

## 文档信息

| 项目 | 内容 |
|------|------|
| 区块链 | Dragonchain |
| 浏览器 | Iquidus Explorer v1.7.4 |
| 部署日期 | 2026-05-07 |
| 部署环境 | Linux / Node.js / MongoDB |

---

## 1. 概述

本文档记录 Dragonchain 区块链浏览器的完整部署和调试过程，包含环境配置、问题排查、代码修改等内容。

---

## 2. 环境要求

| 软件 | 版本 |
|------|------|
| Node.js | 12+ (当前使用 Node 22) |
| MongoDB | 7.0 |
| Dragonchaind | v0.6.0 |

---

## 3. 部署流程

### 3.1 Dragonchaind RPC 配置

**配置文件路径**: `~/.dragonchain/dragonchain.conf`

**配置内容**:
```ini
server=1
txindex=1
rpcuser=your_rpc_username
rpcpassword=your_rpc_password
rpcallowip=127.0.0.1
rpcport=9332
listen=1
```

### 3.2 MongoDB 安装与配置

#### 安装 MongoDB 7.0
```bash
# 添加 MongoDB 密钥
wget -qO - https://www.mongodb.org/static/pgp/server-7.0.asc | sudo apt-key add -

# 添加 APT 源
echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb-server-7.0.gpg ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/7.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-7.0.list

# 安装
sudo apt-get update
sudo apt-get install -y mongodb-org

# 启动服务
sudo systemctl start mongod
sudo systemctl enable mongod
```

### 3.3 Explorer 配置

**配置文件路径**: `/path/to/explorer/settings.json`

**主要配置变更**:

| 配置项 | 值 |
|--------|-----|
| coin | Dragonchain |
| symbol | DRGN |
| wallet.host | localhost |
| wallet.port | 9332 |
| wallet.username | your_rpc_username |
| wallet.password | your_rpc_password |
| genesis_tx | 79d2d2cf340893bc0f5df7cc596ba52fa979727f759f8feef347268414ff5fac |
| genesis_block | 3c9761d390215d5e51843db7802845052ad8bdfad2014050b9cc6b82337089c2 |
| dbsettings.user | (空，无需认证) |
| dbsettings.password | (空，无需认证) |
| dbsettings.database | explorerdb |

**获取创世区块信息的命令**:
```bash
# 获取创世区块哈希
curl --user your_rpc_username:your_rpc_password --data-binary '{"jsonrpc":"1.0","id":"1","method":"getblockhash","params":[0]}' http://127.0.0.1:9332/

# 获取创世区块详细信息和交易哈希
curl --user your_rpc_username:your_rpc_password --data-binary '{"jsonrpc":"1.0","id":"1","method":"getblock","params":["3c9761d390215d5e51843db7802845052ad8bdfad2014050b9cc6b82337089c2",2]}' http://127.0.0.1:9332/
```

### 3.4 安装 Node.js 依赖

```bash
cd /path/to/explorer
npm install
```

---

## 4. 问题排查与修复

### 4.1 问题1：同步脚本误判创世区块高度为 RPC 连接失败

**现象**:
运行 `node scripts/sync.js index update` 时，虽然 RPC 正常工作，但始终报错 "Unable to connect to explorer API"

**原因**:
在 JavaScript 中，`0` 是 falsy 值。当区块高度为 0（创世区块）时，`if (!count)` 判断错误地将有效响应识别为失败。

**修改文件**: `/path/to/explorer/lib/database.js`

**修改位置**: 第 685 行

#### 代码对比

**修改前**:
```javascript
update_db: function(coin, cb) {
  lib.get_blockcount( function (count) {
    if (!count){
      console.log('Unable to connect to explorer API');
      return cb(false);
    }
```

**修改后**:
```javascript
update_db: function(coin, cb) {
  lib.get_blockcount( function (count) {
    if (count === null || count === undefined){
      console.log('Unable to connect to explorer API');
      return cb(false);
    }
```

**修改说明**:
将判断条件从简单的 falsy 检查改为明确的 null/undefined 检查，使得区块高度 0 可以被正确处理。

---

### 4.2 问题2：MongoDB 认证配置

**现象**:
最初配置了 MongoDB 用户名和密码，但连接失败。

**解决方案**:
由于本地部署的 MongoDB 默认未启用认证，我们简化了配置，清空了 dbsettings 中的 user 和 password 字段。

**配置修改**:

**修改文件**: `/path/to/explorer/settings.json`

**修改前**:
```json
"dbsettings": {
  "user": "iquidus",
  "password": "your_db_password",
  "database": "explorerdb",
  "address": "localhost",
  "port": 27017
}
```

**修改后**:
```json
"dbsettings": {
  "user": "",
  "password": "",
  "database": "explorerdb",
  "address": "localhost",
  "port": 27017
}
```

---

## 5. 创世区块信息

```json
{
  "hash": "3c9761d390215d5e51843db7802845052ad8bdfad2014050b9cc6b82337089c2",
  "height": 0,
  "tx": "79d2d2cf340893bc0f5df7cc596ba52fa979727f759f8feef347268414ff5fac",
  "time": 1777900800,
  "nonce": 45,
  "bits": "1f3fffff",
  "difficulty": 2.384149979653205e-07
}
```

---

## 6. 启动与同步

### 6.1 启动 Explorer

```bash
# 方式1：单实例（开发调试）
node --stack-size=10000 bin/instance

# 方式2：集群模式（生产）
npm start
```

### 6.2 首次同步数据

```bash
# 完整重新索引
node scripts/sync.js index reindex

# 更新同步
node scripts/sync.js index update
```

### 6.3 设置定时同步

编辑 crontab：
```bash
crontab -e
```

添加：
```bash
# 每分钟同步区块
*/1 * * * * cd /path/to/explorer && node scripts/sync.js index update > /dev/null 2>&1

# 每5分钟同步节点信息
*/5 * * * * cd /path/to/explorer && node scripts/peers.js > /dev/null 2>&1
```

---

## 7. 访问地址

| 页面 | URL |
|------|-----|
| 首页 | http://localhost:3001/ |
| 创世区块 | http://localhost:3001/block/3c9761d390215d5e51843db7802845052ad8bdfad2014050b9cc6b82337089c2 |
| API 文档 | http://localhost:3001/info |

---

## 8. 服务状态检查

### 检查 Dragonchaind
```bash
ps aux | grep dragonchaind
curl --user your_rpc_username:your_rpc_password --data-binary '{"jsonrpc":"1.0","id":"1","method":"getblockcount","params":[]}' http://127.0.0.1:9332/
```

### 检查 MongoDB
```bash
sudo systemctl status mongod
```

### 检查 Explorer
```bash
curl -s http://localhost:3001/ext/summary
```

---

## 9. 文件清单

### 修改过的文件

| 文件路径 | 说明 |
|----------|------|
| `/path/to/explorer/settings.json` | 主配置文件 |
| `/path/to/explorer/lib/database.js` | 修复区块高度 0 的判断问题 |
| `~/.dragonchain/dragonchain.conf` | Dragonchaind 配置 |

### 新建的文件

| 文件路径 | 说明 |
|----------|------|
| `/path/to/explorer/test_rpc.js` | RPC 连接测试脚本（临时） |
| `/path/to/explorer/README-DRAGONCHAIN.md` | 中文部署指南 |
| `/path/to/explorer/get_genesis_info.sh` | 获取创世区块信息脚本 |
| 本文件 | 本文档 |

---

## 10. 技术总结

本次部署成功将 Iquidus Explorer 适配到 Dragonchain 区块链，主要解决了以下问题：

1. **创世区块高度为 0 的特殊情况处理** - 修改了 falsy 值的判断逻辑
2. **MongoDB 认证配置简化** - 在本地环境中移除不必要的认证
3. **RPC 连接配置** - 正确配置了 dragonchaind 的 RPC 参数

浏览器已成功运行在 http://localhost:3001，并已同步创世区块数据。
