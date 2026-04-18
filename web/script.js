class ChatApp {
    constructor() {
        this.apiUrl = '';
        this.isConnected = true;
        this.isTyping = false;
        this.currentModel = 'openai/gpt-3.5-turbo';
        
        this.elements = {
            chatContainer: document.getElementById('chatContainer'),
            messageInput: document.getElementById('messageInput'),
            sendBtn: document.getElementById('sendBtn'),
            statusDot: document.getElementById('statusDot'),
            statusText: document.getElementById('statusText'),
            modelSelect: document.getElementById('modelSelect')
        };
        
        this.init();
    }
    
    init() {
        this.elements.sendBtn.addEventListener('click', () => this.sendMessage());
        this.elements.messageInput.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' && !e.shiftKey) {
                e.preventDefault();
                this.sendMessage();
            }
        });
        
        this.elements.messageInput.addEventListener('input', () => {
            this.elements.messageInput.style.height = 'auto';
            this.elements.messageInput.style.height = 
                Math.min(this.elements.messageInput.scrollHeight, 150) + 'px';
        });
        
        this.elements.modelSelect.addEventListener('change', (e) => {
            this.currentModel = e.target.value;
        });
        
        this.loadModels();
    }
    
    async loadModels() {
        try {
            const response = await fetch('/api/models');
            const data = await response.json();
            
            if (data.models) {
                this.elements.modelSelect.innerHTML = data.models.map(m => 
                    `<option value="${m.id}" ${m.id === data.current ? 'selected' : ''}>${m.name}</option>`
                ).join('');
                this.currentModel = data.current;
            }
        } catch (error) {
            console.error('Failed to load models:', error);
        }
    }
    
    async sendMessage() {
        const message = this.elements.messageInput.value.trim();
        if (!message || this.isTyping) return;
        
        this.elements.messageInput.value = '';
        this.elements.messageInput.style.height = 'auto';
        
        this.addMessage(message, 'user');
        this.showTypingIndicator();
        
        this.elements.sendBtn.disabled = true;
        this.isTyping = true;
        
        try {
            const response = await fetch('/api/chat', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ 
                    message: message,
                    model: this.currentModel 
                })
            });
            
            const data = await response.json();
            this.removeTypingIndicator();
            
            if (data.response) {
                this.addMessage(data.response, 'bot');
            } else if (data.error) {
                this.addMessage('Error: ' + data.error, 'bot');
            }
        } catch (error) {
            this.removeTypingIndicator();
            this.addMessage('Error: Could not connect to server', 'bot');
            this.setConnectionStatus(false);
        }
        
        this.isTyping = false;
        this.elements.sendBtn.disabled = false;
        this.elements.messageInput.focus();
    }
    
    addMessage(text, sender) {
        const messageDiv = document.createElement('div');
        messageDiv.className = `message ${sender}`;
        
        const contentDiv = document.createElement('div');
        contentDiv.className = 'message-content';
        contentDiv.textContent = text;
        
        const timeDiv = document.createElement('div');
        timeDiv.className = 'message-time';
        timeDiv.textContent = new Date().toLocaleTimeString([], { 
            hour: '2-digit', 
            minute: '2-digit' 
        });
        
        messageDiv.appendChild(contentDiv);
        messageDiv.appendChild(timeDiv);
        
        this.elements.chatContainer.appendChild(messageDiv);
        this.scrollToBottom();
    }
    
    showTypingIndicator() {
        const typingDiv = document.createElement('div');
        typingDiv.className = 'message bot typing';
        typingDiv.id = 'typingIndicator';
        
        const contentDiv = document.createElement('div');
        contentDiv.className = 'message-content';
        contentDiv.innerHTML = `
            <span class="typing-dot"></span>
            <span class="typing-dot"></span>
            <span class="typing-dot"></span>
        `;
        
        typingDiv.appendChild(contentDiv);
        this.elements.chatContainer.appendChild(typingDiv);
        this.scrollToBottom();
    }
    
    removeTypingIndicator() {
        const indicator = document.getElementById('typingIndicator');
        if (indicator) indicator.remove();
    }
    
    scrollToBottom() {
        this.elements.chatContainer.scrollTop = 
            this.elements.chatContainer.scrollHeight;
    }
    
    setConnectionStatus(connected) {
        this.isConnected = connected;
        this.elements.statusDot.className = connected ? 'status-dot' : 'status-dot disconnected';
        this.elements.statusText.textContent = connected ? 'Connected' : 'Disconnected';
    }
}

document.addEventListener('DOMContentLoaded', () => {
    window.chatApp = new ChatApp();
});
