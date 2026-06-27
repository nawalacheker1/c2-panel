/**
 * C2 PANEL - WebSocket Client
 */

class C2WebSocket {
    constructor() {
        this.socket = null;
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 10;
        this.reconnectDelay = 3000;
        this.listeners = {};
        this.init();
    }

    init() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const url = `${protocol}//${window.location.host}/socket.io/?EIO=4&transport=websocket`;
        
        try {
            this.socket = new WebSocket(url);
            this.socket.onopen = this.onOpen.bind(this);
            this.socket.onmessage = this.onMessage.bind(this);
            this.socket.onclose = this.onClose.bind(this);
            this.socket.onerror = this.onError.bind(this);
        } catch (e) {
            console.error('[WebSocket] Failed to connect:', e);
            this.reconnect();
        }
    }

    onOpen(event) {
        console.log('[WebSocket] Connected');
        this.reconnectAttempts = 0;
        this.emit('connected', { timestamp: new Date().toISOString() });
    }

    onMessage(event) {
        try {
            const data = JSON.parse(event.data);
            
            // Skip ping/pong
            if (data.type === 'ping' || data.type === 'pong') {
                return;
            }
            
            // Handle different message types
            switch (data.type) {
                case 'new_alert':
                    this.emit('alert', data.data);
                    break;
                case 'agent_status':
                    this.emit('agent_status', data.data);
                    break;
                case 'task_update':
                    this.emit('task_update', data.data);
                    break;
                default:
                    this.emit('message', data);
            }
        } catch (e) {
            console.warn('[WebSocket] Invalid message:', event.data);
        }
    }

    onClose(event) {
        console.log('[WebSocket] Disconnected');
        this.reconnect();
    }

    onError(event) {
        console.error('[WebSocket] Error:', event);
    }

    reconnect() {
        if (this.reconnectAttempts >= this.maxReconnectAttempts) {
            console.error('[WebSocket] Max reconnect attempts reached');
            return;
        }

        this.reconnectAttempts++;
        const delay = this.reconnectDelay * this.reconnectAttempts;
        console.log(`[WebSocket] Reconnecting in ${delay/1000}s (attempt ${this.reconnectAttempts})`);
        
        setTimeout(() => {
            this.init();
        }, delay);
    }

    send(data) {
        if (this.socket && this.socket.readyState === WebSocket.OPEN) {
            this.socket.send(JSON.stringify(data));
        } else {
            console.warn('[WebSocket] Not connected, cannot send');
        }
    }

    emit(event, data) {
        this.send({ event, data });
    }

    on(event, callback) {
        if (!this.listeners[event]) {
            this.listeners[event] = [];
        }
        this.listeners[event].push(callback);
    }

    off(event, callback) {
        if (!this.listeners[event]) return;
        if (callback) {
            this.listeners[event] = this.listeners[event].filter(cb => cb !== callback);
        } else {
            delete this.listeners[event];
        }
    }

    emitEvent(event, data) {
        if (this.listeners[event]) {
            this.listeners[event].forEach(callback => callback(data));
        }
    }
}

// Singleton
window.C2WebSocket = new C2WebSocket();
