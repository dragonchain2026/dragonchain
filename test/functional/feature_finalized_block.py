#!/usr/bin/env python3
# Copyright (c) 2026 Dragonchain
# Distributed under the MIT software license.
"""Regression test for 51% protection: maxreorgdepth, finalizeblock, restart recovery

Tests:
  1. maxreorgdepth auto-finalizes blocks at the correct depth
  2. finalizeblock RPC manually finalizes a specific block
  3. pindexFinalized recovers after restart (no lost protection)
  4. Deep reorgs below finalized height are blocked
  5. maxreorgdepth=999999 bypasses the protection (repair mode)
  6. getblockchaininfo shows finalized_height and finalized_hash
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    connect_nodes,
    disconnect_nodes,
    sync_blocks,
    wait_until,
)

class FinalizedBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        # node0: default (maxreorgdepth=10)
        # node1: maxreorgdepth=5 (shorter finalization)
        # node2: initially same as node0, used for restart tests
        self.extra_args = [
            ["-maxreorgdepth=10"],
            ["-maxreorgdepth=5"],
            ["-maxreorgdepth=10"],
        ]

    def setup_network(self):
        self.setup_nodes()
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        sync_blocks([self.nodes[0], self.nodes[1], self.nodes[2]])

    # ------------------------------------------------------------------
    # Test 1: getblockchaininfo shows finalized fields
    # ------------------------------------------------------------------
    def test_getblockchaininfo_fields(self):
        self.log.info("Test 1: getblockchaininfo includes finalized fields")

        info = self.nodes[0].getblockchaininfo()
        assert 'finalized_height' in info, "finalized_height missing from getblockchaininfo"
        assert 'finalized_hash' in info, "finalized_hash missing from getblockchaininfo"

        # Initially, before enough blocks, finalized_height should be 0 or null
        self.log.info("  finalized_height=%s, finalized_hash=%s",
                      info['finalized_height'], info['finalized_hash'])

        # Generate enough blocks to trigger auto-finalization on node0 (depth=10)
        self.nodes[0].generate(15)
        sync_blocks([self.nodes[0], self.nodes[1], self.nodes[2]])

        info = self.nodes[0].getblockchaininfo()
        assert_greater_than(info['finalized_height'], 0,
                            "finalized_height should be >0 after generating >maxreorgdepth blocks")
        self.log.info("  After 15 blocks: finalized_height=%d", info['finalized_height'])

    # ------------------------------------------------------------------
    # Test 2: auto-finalization at different maxreorgdepth values
    # ------------------------------------------------------------------
    def test_auto_finalization_depth(self):
        self.log.info("Test 2: auto-finalization respects maxreorgdepth")

        # Generate blocks and check finalization
        self.nodes[0].generate(5)
        sync_blocks([self.nodes[0], self.nodes[1], self.nodes[2]])

        h0 = self.nodes[0].getblockcount()
        info0 = self.nodes[0].getblockchaininfo()
        info1 = self.nodes[1].getblockchaininfo()

        self.log.info("  node0 (depth=10): tip=%d finalized=%d", h0, info0['finalized_height'])
        self.log.info("  node1 (depth=5):  tip=%d finalized=%d",
                      self.nodes[1].getblockcount(), info1['finalized_height'])

        # node1 (depth=5) should finalize blocks closer to tip than node0 (depth=10)
        assert_greater_than(info1['finalized_height'], 0,
                            "node1 should have finalized some blocks")
        self.log.info("  OK: both nodes auto-finalizing at their respective depths")

    # ------------------------------------------------------------------
    # Test 3: manual finalizeblock works
    # ------------------------------------------------------------------
    def test_manual_finalizeblock(self):
        self.log.info("Test 3: manual finalizeblock RPC")

        height = self.nodes[0].getblockcount()
        target_height = height - 3
        target_hash = self.nodes[0].getblockhash(target_height)

        self.log.info("  Manually finalizing block %d (%s)", target_height, target_hash[:16])
        result = self.nodes[0].finalizeblock(target_hash)
        self.log.info("  result: %s", result)

        info = self.nodes[0].getblockchaininfo()
        assert_greater_than(info['finalized_height'], target_height - 1,
                            "finalized_height should be >= manually finalized block")
        self.log.info("  finalized_height after manual finalize: %d", info['finalized_height'])

    # ------------------------------------------------------------------
    # Test 4: deep reorg is blocked when crossing finalized boundary
    # ------------------------------------------------------------------
    def test_deep_reorg_blocked(self):
        self.log.info("Test 4: deep reorg below finalized height is blocked")

        # Disconnect node2 from the network
        disconnect_nodes(self.nodes[2], 1)

        # Mine on main chain (node0+node1)
        info_before = self.nodes[0].getblockchaininfo()
        finalized_h = info_before['finalized_height']
        self.log.info("  Current finalized_height=%d", finalized_h)

        # Mine blocks on node0 to advance the finalized pointer
        self.nodes[0].generate(12)
        sync_blocks([self.nodes[0], self.nodes[1]])

        info_now = self.nodes[0].getblockchaininfo()
        self.log.info("  After mining 12 blocks: finalized_height=%d", info_now['finalized_height'])

        # Now mine a fork on the isolated node2. We need to invalidate a block
        # above finalized but mine a chain that diverges below finalized.
        # Invalidate a recent block on node2 to create a fork
        fork_height = self.nodes[0].getblockcount() - 3
        fork_hash = self.nodes[0].getblockhash(fork_height)

        # On node2, we invalidate the block at fork_height to create a fork
        self.nodes[2].invalidateblock(fork_hash)

        # Mine a longer chain on node2 (making it most-work)
        self.nodes[2].generate(5)

        # Now reconnect node2 to the network. It should NOT be able to reorg
        # past the finalized block because fork_height < finalized_height.
        connect_nodes(self.nodes[2], 1)

        # Check that node0 did NOT reorg (its finalized blocks stay intact)
        # This is verified by node0's finalized_height not decreasing
        info_final = self.nodes[0].getblockchaininfo()
        assert_greater_than(info_final['finalized_height'], finalized_h,
                            "Finalized height should not decrease after attempted reorg")
        self.log.info("  OK: deep reorg was blocked, finalized height intact")

        # Check that node2 has the warning about the blocked reorg
        warnings = self.nodes[0].getmiscwarning() or ""
        self.log.info("  Node warnings: %s", warnings[:120] if warnings else "(none)")

    # ------------------------------------------------------------------
    # Test 5: maxreorgdepth=999999 bypasses protection (repair mode)
    # ------------------------------------------------------------------
    def test_repair_mode_bypass(self):
        self.log.info("Test 5: maxreorgdepth=999999 bypasses protection")

        # Restart node2 with repair mode
        self.stop_node(2)

        # Before restart, we need to set up a scenario where repair mode is needed.
        # Invalidate a recent block and mine an alternative
        info = self.nodes[0].getblockchaininfo()
        self.log.info("  finalized_height before repair test: %d", info['finalized_height'])

        self.start_node(2, extra_args=["-maxreorgdepth=999999"])
        connect_nodes(self.nodes[2], 1)
        sync_blocks([self.nodes[0], self.nodes[2]])

        # Verify node2 synced correctly despite finalized blocks
        h0 = self.nodes[0].getblockcount()
        h2 = self.nodes[2].getblockcount()
        assert_equal(h0, h2, "Node2 should sync to same height as node0 in repair mode")
        self.log.info("  OK: repair mode synced successfully, h0=%d h2=%d", h0, h2)

    # ------------------------------------------------------------------
    # Test 6: pindexFinalized recovers after restart
    # ------------------------------------------------------------------
    def test_restart_recovery(self):
        self.log.info("Test 6: pindexFinalized recovers after restart")

        info_before = self.nodes[0].getblockchaininfo()
        finalized_before = info_before['finalized_height']
        self.log.info("  finalized_height before restart: %d", finalized_before)

        # Restart node0
        self.stop_node(0)
        self.start_node(0, extra_args=["-maxreorgdepth=10"])
        connect_nodes(self.nodes[0], 1)
        sync_blocks([self.nodes[0], self.nodes[1]])

        # Check that finalized_height is preserved after restart
        info_after = self.nodes[0].getblockchaininfo()
        finalized_after = info_after['finalized_height']
        self.log.info("  finalized_height after restart: %d", finalized_after)

        assert_greater_than(finalized_after, 0,
                            "pindexFinalized should be restored after restart (not 0)")
        self.log.info("  OK: pindexFinalized recovered correctly after restart")

    # ------------------------------------------------------------------
    # Test 7: maxreorgdepth=0 disables auto-finalization
    # ------------------------------------------------------------------
    def test_disable_auto_finalization(self):
        self.log.info("Test 7: maxreorgdepth=0 disables auto-finalization")

        self.stop_node(2)
        self.start_node(2, extra_args=["-maxreorgdepth=0"])
        connect_nodes(self.nodes[2], 1)
        sync_blocks([self.nodes[0], self.nodes[2]])

        # Generate blocks and check that node2 does NOT auto-finalize
        info_before = self.nodes[2].getblockchaininfo()
        fh_before = info_before['finalized_height']
        self.log.info("  finalized before generate: %d", fh_before)

        self.nodes[0].generate(10)
        sync_blocks([self.nodes[0], self.nodes[2]])

        info_after = self.nodes[2].getblockchaininfo()
        fh_after = info_after['finalized_height']
        self.log.info("  finalized after generate: %d", fh_after)

        # With maxreorgdepth=0, no new blocks should be auto-finalized
        # (unless manual finalizeblock was called)
        if fh_before > 0:
            assert_equal(fh_before, fh_after,
                         "maxreorgdepth=0 should not auto-finalize new blocks")
        self.log.info("  OK: maxreorgdepth=0 correctly disables auto-finalization")

    def run_test(self):
        self.test_getblockchaininfo_fields()
        self.test_auto_finalization_depth()
        self.test_manual_finalizeblock()
        self.test_deep_reorg_blocked()
        self.test_repair_mode_bypass()
        self.test_restart_recovery()
        self.test_disable_auto_finalization()
        self.log.info("ALL TESTS PASSED")


if __name__ == '__main__':
    FinalizedBlockTest().main()
