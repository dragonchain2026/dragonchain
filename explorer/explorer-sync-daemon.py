#!/usr/bin/env python3
"""
Continuous sync daemon for Dragonchain Explorer.
Calls node scripts/sync.js index update in a loop to keep data current.
"""
import subprocess, time, sys, signal, os

INTERVAL = 60  # seconds between sync rounds

def log(msg):
    print(f"[sync-daemon] {time.strftime('%Y-%m-%d %H:%M:%S')} {msg}", flush=True)

def run_sync():
    """Run one sync update round. Returns True if successful."""
    log("Starting sync update...")
    proc = subprocess.Popen(
        ["node", "--stack-size=10000", "scripts/sync.js", "index", "update"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        cwd="/www/wwwroot/explorer"
    )
    output_lines = []
    last_log_time = time.time()

    # Stream output in real-time, log every 60 seconds
    for line in proc.stdout:
        line = line.strip()
        if line:
            output_lines.append(line)
            # Log progress every 60 seconds during long syncs
            if time.time() - last_log_time > 60:
                # Print last block-synced line if available
                for l in reversed(output_lines):
                    if l and l[0].isdigit():
                        log(f"  Progress: {l[:80]}")
                        break
                last_log_time = time.time()

    proc.wait()
    output = "\n".join(output_lines)

    # Show last 5 lines
    for line in output_lines[-5:]:
        log(f"  {line}")
    if proc.returncode == 0 and "update complete" in output:
        log("Sync round completed successfully")
        return True
    elif "Script already running" in output:
        log("Sync already running (lock exists), will retry later")
        return True
    else:
        log(f"Sync failed with code {proc.returncode}")
        return False

def main():
    log("Daemon started (interval=" + str(INTERVAL) + "s)")

    def shutdown(sig, frame):
        log(f"Received signal {sig}, shutting down")
        # Clean up lock file
        lock = "/www/wwwroot/explorer/tmp/index.pid"
        try:
            os.remove(lock)
        except:
            pass
        sys.exit(0)

    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)

    while True:
        try:
            run_sync()
        except subprocess.TimeoutExpired:
            log("Sync timed out (>1 hour), will retry")
        except Exception as e:
            log(f"Unexpected error: {e}")

        log(f"Sleeping {INTERVAL}s until next check...")
        time.sleep(INTERVAL)

if __name__ == "__main__":
    main()
