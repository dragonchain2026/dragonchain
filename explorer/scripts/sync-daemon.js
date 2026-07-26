#!/usr/bin/env node
/**
 * Dragonchain Explorer Continuous Sync Daemon
 *
 * Runs sync.js index update in an infinite loop.
 * Manages its own lock file, auto-cleans on crash/exit.
 * Designed for PM2 process management.
 *
 * Usage: node scripts/sync-daemon.js
 */

const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');

const INTERVAL = 60000; // 60 seconds between sync rounds
const LOCK_FILE = path.join(__dirname, '..', 'tmp', 'index.pid');
const SCRIPT = path.join(__dirname, 'sync.js');

let running = false;

function log(msg) {
    console.log(`[sync-daemon ${new Date().toISOString()}] ${msg}`);
}

function cleanLock() {
    try { fs.unlinkSync(LOCK_FILE); } catch(e) { /* ok */ }
}

function runSync() {
    return new Promise((resolve) => {
        if (running) {
            log('Previous sync still running, skipping this round');
            return resolve(false);
        }
        running = true;

        // Remove any stale lock first
        cleanLock();

        log('Starting sync update...');
        const child = spawn('node', ['--stack-size=10000', SCRIPT, 'index', 'update'], {
            cwd: path.join(__dirname, '..'),
            stdio: ['ignore', 'pipe', 'pipe']
        });

        let output = '';

        child.stdout.on('data', (data) => {
            const text = data.toString().trim();
            if (text) {
                output += text + '\n';
                // Show progress for long syncs
                if (text.includes('update complete') || text.match(/^\d{5,}:/)) {
                    log(`  ${text.substring(0, 100)}`);
                }
            }
        });

        child.stderr.on('data', (data) => {
            output += data.toString().trim() + '\n';
        });

        child.on('close', (code) => {
            running = false;
            cleanLock();

            if (code === 0 && output.includes('update complete')) {
                log('Sync round completed');
                resolve(true);
            } else if (output.includes('Script already running')) {
                log('Lock existed - cleaned, next round will succeed');
                resolve(false);
            } else {
                log(`Sync exited with code ${code}`);
                resolve(false);
            }
        });

        child.on('error', (err) => {
            running = false;
            cleanLock();
            log(`Sync error: ${err.message}`);
            resolve(false);
        });

        // Safety timeout: kill sync if it runs more than 4 hours
        setTimeout(() => {
            if (!child.killed) {
                log('Sync timeout (4h), killing...');
                child.kill('SIGTERM');
                running = false;
                cleanLock();
                resolve(false);
            }
        }, 4 * 3600 * 1000);
    });
}

// Graceful shutdown
function shutdown() {
    log('Shutting down...');
    cleanLock();
    process.exit(0);
}

process.on('SIGTERM', shutdown);
process.on('SIGINT', shutdown);
process.on('uncaughtException', (err) => {
    log(`Uncaught: ${err.message}`);
    cleanLock();
    process.exit(1);
});

// Main loop
async function main() {
    log('Daemon started (interval=' + (INTERVAL/1000) + 's)');
    cleanLock();

    while (true) {
        await runSync();
        await new Promise(r => setTimeout(r, INTERVAL));
    }
}

main();
