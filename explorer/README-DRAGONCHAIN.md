# Dragonchain 区块浏览器部署指南

## 📋 目录
1. [环境要求](#环境要求)
2. [快速部署](#快速部署)
3. [详细步骤](#详细步骤)
4. [常见问题](#常见问题)

---

## 🖥️ 环境要求

| 软件 | 版本要求 |
|------|---------|
| Node.js | >= 8.17.0 (建议 12.x) |
| MongoDB | 4.2.x |
| Dragonchaind | 已开启 -txindex 和 RPC |

---

## 🚀 快速部署

### 1. 准备工作

确保 Dragonchaind 正在运行，并开启了以下参数:
```bash
dragonchaind -daemon -txindex -server -rpcuser=你的用户名 -rpcpassword=你的密码
```

### 2. 安装依赖

```bash
# Node.js 12.x (Ubuntu/Debian)
curl -fsSL https://deb.nodesource.com/setup_12.x | sudo -E bash -
sudo apt-get install -y nodejs

# MongoDB 4.2.x
wget -qO - https://www.mongodb.org/static/pgp/server-4.2.asc | sudo apt-key add -
echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu bionic/mongodb-org/4.2 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.2.list
sudo apt-get update
sudo apt-get install -y mongodb-org

# 启动 MongoDB
sudo systemctl start mongod
sudo systemctl enable mongod
```

### 3. 配置 MongoDB

```bash
# 进入 MongoDB shell
mongo

# 创建数据库和用户
> use explorerdb
> db.createUser({ user: "iquidus", pwd: "your_db_password", roles: ["readWrite"] })
> exit
```

### 4. 获取创世区块信息

```bash
# 运行脚本获取创世区块信息
./get_genesis_info.sh localhost 9332 你的RPC用户名 你的RPC密码

# 或者手动获取
curl --user rpcuser:rpcpassword --data-binary '{"jsonrpc":"1.0","id":"1","method":"getblockhash","params":[0]}' http://127.0.0.1:9332/
```

### 5. 编辑配置文件

编辑 `settings.json`:
```json
{
  "title": "Dragonchain Explorer",
  "coin": "Dragonchain",
  "symbol": "DRGN",
  
  "wallet": {
    "host": "localhost",
    "port": 9332,
    "username": "你的RPC用户名",
    "password": "你的RPC密码"
  },
  
  "genesis_tx": "脚本输出的创世交易哈希",
  "genesis_block": "脚本输出的创世区块哈希"
}
```

### 6. 安装 Node.js 依赖

```bash
cd /path/to/explorer-master
npm install --production
```

### 7. 首次同步

```bash
# 完整重新索引（第一次运行）
node scripts/sync.js index reindex
```

### 8. 启动浏览器

```bash
# 启动（集群模式，生产推荐）
npm start

# 或单实例（开发调试用）
node --stack-size=10000 bin/instance
```

### 9. 设置定时同步（重要！）

```bash
# 编辑 crontab
crontab -e

# 添加以下内容（每分钟同步，每2分钟同步市场数据）
*/1 * * * * cd /path/to/explorer-master && node scripts/sync.js index update > /dev/null 2>&1
*/2 * * * * cd /path/to/explorer-master && node scripts/sync.js market > /dev/null 2>&1
*/5 * * * * cd /path/to/explorer-master && node scripts/peers.js > /dev/null 2>&1

# 保存并退出
```

---

## 📖 详细步骤

### 第一步：环境准备

#### 1.1 安装 Node.js

**Ubuntu/Debian:**
```bash
curl -fsSL https://deb.nodesource.com/setup_12.x | sudo -E bash -
sudo apt-get install -y nodejs
```

**CentOS/RHEL:**
```bash
curl -fsSL https://rpm.nodesource.com/setup_12.x | sudo bash -
sudo yum install -y nodejs
```

**验证安装:**
```bash
node -v
npm -v
```

#### 1.2 安装 MongoDB

**Ubuntu/Debian:**
```bash
wget -qO - https://www.mongodb.org/static/pgp/server-4.2.asc | sudo apt-key add -
echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu bionic/mongodb-org/4.2 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.2.list
sudo apt-get update
sudo apt-get install -y mongodb-org
```

**CentOS/RHEL:**
```bash
sudo vi /etc/yum.repos.d/mongodb-org-4.2.repo
# 添加:
[mongodb-org-4.2]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/$releasever/mongodb-org/4.2/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://www.mongodb.org/static/pgp/server-4.2.asc

sudo yum install -y mongodb-org
```

**启动 MongoDB:**
```bash
sudo systemctl start mongod
sudo systemctl enable mongod
```

#### 1.3 配置 MongoDB 用户

```bash
mongo

> use explorerdb
> db.createUser({ user: "iquidus", pwd: "your_db_password", roles: ["readWrite"] })
> exit
```

### 第二步：配置 Dragonchain Explorer

#### 2.1 获取创世区块信息

```bash
# 方式 A: 使用脚本（推荐）
./get_genesis_info.sh localhost 9332 rpcuser rpcpassword

# 方式 B: 手动获取
# 获取创世区块哈希
curl --user rpcuser:rpcpassword --data-binary '{"jsonrpc":"1.0","id":"1","method":"getblockhash","params":[0]}' http://127.0.0.1:9332/

# 获取创世区块详情
curl --user rpcuser:rpcpassword --data-binary '{"jsonrpc":"1.0","id":"1","method":"getblock","params":["创世区块哈希",2]}' http://127.0.0.1:9332/
```

#### 2.2 编辑 settings.json

编辑 `settings.json`，填写以下必填项:

| 配置项 | 说明 |
|--------|------|
| `wallet.host` | Dragonchaind IP（通常 localhost） |
| `wallet.port` | RPC 端口（默认 9332） |
| `wallet.username` | RPC 用户名 |
| `wallet.password` | RPC 密码 |
| `genesis_tx` | 创世交易哈希 |
| `genesis_block` | 创世区块哈希 |

其他配置项（可选）:
- `theme`: 主题颜色（Cyborg, Darkly, Flatly 等）
- `port`: 浏览器监听端口（默认 3001）

### 第三步：安装和运行

#### 3.1 安装 Node.js 依赖

```bash
npm install --production
```

#### 3.2 首次同步数据

⚠️ **重要**: 第一次必须完整索引:
```bash
node scripts/sync.js index reindex
```

同步时间取决于链的高度。

#### 3.3 启动浏览器

**生产环境（集群模式）:**
```bash
npm start
```

**开发调试（单实例）:**
```bash
node --stack-size=10000 bin/instance
```

**停止:**
```bash
npm stop
```

#### 3.4 设置定时任务

这是最重要的一步，否则浏览器数据不会更新！

```bash
crontab -e
```

添加以下内容:
```
# 每分钟同步区块和交易
*/1 * * * * cd /path/to/explorer-master && node scripts/sync.js index update > /dev/null 2>&1

# 每2分钟同步市场数据（如不需要可忽略）
*/2 * * * * cd /path/to/explorer-master && node scripts/sync.js market > /dev/null 2>&1

# 每5分钟同步节点信息
*/5 * * * * cd /path/to/explorer-master && node scripts/peers.js > /dev/null 2>&1
```

保存并退出。

### 第四步：访问浏览器

浏览器默认运行在:
```
http://服务器IP:3001
```

---

## 🔧 常见问题

### Q1: 提示 "script is already running"

**问题**: 启动同步时提示脚本已在运行

**解决**:
```bash
# 删除临时 PID 文件
rm tmp/index.pid
rm tmp/db_index.pid
```

### Q2: "RangeError: Maximum call stack size exceeded"

**问题**: 同步时堆栈溢出

**解决**: 增加栈大小
```bash
node --stack-size=10000 scripts/sync.js index update
```

### Q3: 无法连接到 RPC

**问题**: 错误提示无法连接到 wallet

**解决**:
1. 确认 dragonchaind 正在运行
2. 确认开启了 `-server` 和 `-txindex`
3. 检查 `dragonchain.conf` 中的配置:
   ```
   server=1
   txindex=1
   rpcuser=你的用户名
   rpcpassword=你的密码
   rpcallowip=127.0.0.1
   ```
4. 检查防火墙

### Q4: MongoDB 连接失败

**问题**: 无法连接到 MongoDB

**解决**:
1. 确认 MongoDB 正在运行: `sudo systemctl status mongod`
2. 检查 `settings.json` 中的数据库配置
3. 确认数据库用户已创建

### Q5: 数据不同步

**问题**: 浏览器显示的数据是旧的

**解决**:
1. 确认 crontab 任务正确配置
2. 检查 cron 日志: `grep CRON /var/log/syslog`
3. 手动运行同步测试:
   ```bash
   cd /path/to/explorer-master
   node scripts/sync.js index update
   ```

### Q6: 如何更改主题？

**解决**: 编辑 `settings.json` 中的 `theme` 字段:
- `Cyborg` (深色主题，默认)
- `Darkly`
- `Flatly`
- `Superhero`
- 等...

---

## 📝 配置文件说明

### settings.json 主要配置项

| 配置项 | 说明 |
|--------|------|
| `title` | 浏览器标题 |
| `coin` | 币种名称 |
| `symbol` | 币种符号 |
| `theme` | UI 主题 |
| `port` | Web 服务端口 |
| `wallet.*` | RPC 连接配置 |
| `genesis_tx` | 创世交易哈希 |
| `genesis_block` | 创世区块哈希 |
| `confirmations` | 交易确认数 |
| `index.difficulty` | PoW/PoS |
| `supply` | 供应计算方式（COINBASE） |
| `nethash` | 算力获取方式（getnetworkhashps） |

---

## 🚀 部署到服务器

### 从本地传输到服务器

```bash
# 使用 scp 传输
scp -r /path/to/explorer user@your-server:/path/to/

# 或使用 rsync
rsync -avz /path/to/explorer user@your-server:/path/to/
```

### 服务器防火墙配置

```bash
# 如果需要从外部访问，开放 3001 端口
sudo ufw allow 3001
```

---

## 📞 技术支持

如遇问题，请检查:
1. [官方 Iquidus Explorer 文档](https://github.com/iquidus/explorer)
2. MongoDB 日志: `/var/log/mongodb/`
3. 浏览器输出（运行时的终端）

---

## ✅ 检查清单

部署后请确认:

- [ ] MongoDB 正在运行
- [ ] MongoDB 用户已创建
- [ ] Dragonchaind 正在运行
- [ ] Dragonchaind 已开启 -txindex
- [ ] settings.json 已配置完成
- [ ] 创世区块信息已填入
- [ ] 首次同步已完成
- [ ] 浏览器可以启动
- [ ] 可以访问 http://服务器IP:3001
- [ ] crontab 任务已配置
- [ ] 数据可以正常更新

---

祝你部署顺利！🎉
