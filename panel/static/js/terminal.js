/**
 * C2 PANEL - Terminal JavaScript
 */

class C2Terminal {
    constructor(agentId) {
        this.agentId = agentId;
        this.output = document.getElementById('terminalOutput');
        this.input = document.getElementById('terminalInput');
        this.history = [];
        this.historyIndex = -1;
        this.commands = {};
        this.isExecuting = false;
        this.init();
    }

    init() {
        if (!this.output || !this.input) return;

        this.input.addEventListener('keydown', this.handleKey.bind(this));
        this.input.focus();

        // Click to focus
        this.output.parentElement.addEventListener('click', () => {
            this.input.focus();
        });

        this.printLine('C2 Terminal v1.0', 'info');
        this.printLine(`Connected to Agent ${this.agentId}`, 'info');
        this.printLine('Type "help" for commands', 'info');
        this.printLine('─'.repeat(40), 'info');
        this.printLine('', 'output');
    }

    printLine(text, type = 'output') {
        if (!this.output) return;
        const line = document.createElement('div');
        line.className = `line ${type}`;
        line.textContent = text;
        this.output.appendChild(line);
        this.output.scrollTop = this.output.scrollHeight;
    }

    async executeCommand(command) {
        if (!command.trim()) return;

        this.printLine(`$ ${command}`, 'prompt');
        this.history.push(command);
        this.historyIndex = this.history.length;

        if (command === 'help') {
            this.showHelp();
            return;
        }

        if (command === 'clear') {
            this.clear();
            return;
        }

        if (command === 'status') {
            this.showStatus();
            return;
        }

        this.isExecuting = true;
        this.input.disabled = true;

        try {
            const response = await fetch(`/api/agent/${this.agentId}/command`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ command: command })
            });

            if (response.ok) {
                this.printLine('[✓] Command sent to agent', 'success');
            } else {
                this.printLine(`[✗] Failed: ${response.status} ${response.statusText}`, 'error');
            }
        } catch (e) {
            this.printLine(`[✗] Error: ${e.message}`, 'error');
        }

        this.isExecuting = false;
        this.input.disabled = false;
        this.input.focus();
    }

    showHelp() {
        this.printLine('Available commands:', 'info');
        this.printLine('  help     - Show this help', 'output');
        this.printLine('  clear    - Clear terminal', 'output');
        this.printLine('  status   - Show agent status', 'output');
        this.printLine('  <command> - Execute any command on agent', 'output');
        this.printLine('', 'output');
    }

    showStatus() {
        this.printLine(`Agent ID: ${this.agentId}`, 'info');
        this.printLine(`Status: Online`, 'success');
        this.printLine(`Terminal: Ready`, 'info');
        this.printLine('', 'output');
    }

    clear() {
        if (this.output) {
            this.output.innerHTML = '';
        }
    }

    handleKey(e) {
        if (e.key === 'Enter') {
            const command = this.input.value;
            this.input.value = '';
            this.executeCommand(command);
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
        } else if (e.key === 'l' && e.ctrlKey) {
            e.preventDefault();
            this.clear();
        }
    }

    updateStatus(status) {
        const statusEl = document.getElementById('terminalStatus');
        if (statusEl) {
            statusEl.textContent = status;
            statusEl.className = `status-${status}`;
        }
    }
}

// Initialize terminal when page loads
document.addEventListener('DOMContentLoaded', function() {
    const agentIdEl = document.querySelector('.agent-id');
    if (agentIdEl) {
        window.terminal = new C2Terminal(agentIdEl.textContent);
    }
});
