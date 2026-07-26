#!/bin/bash

# Dragonchain Explorer - 获取创世区块信息脚本
# 使用方法: ./get_genesis_info.sh [rpc-host] [rpc-port] [rpc-user] [rpc-password]

RPC_HOST=${1:-localhost}
RPC_PORT=${2:-9332}
RPC_USER=${3:-rpcuser}
RPC_PASS=${4:-rpcpassword}

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
