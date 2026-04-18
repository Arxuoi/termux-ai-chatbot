# Termux AI Chatbot

AI Chatbot for Termux using OpenRouter API. Written in pure C.

## Features
- 🚀 Lightweight C server (no Python/Node.js)
- 🔒 API key stored in `.env` (never exposed)
- 💬 Clean web interface
- 🎨 Syntax highlighting support
- 📱 Optimized for Termux/Android

## Quick Start

```bash
# Clone repository
git clone https://github.com/Arxuoi/termux-ai-chatbot
cd termux-ai-chatbot

# Run installer
chmod +x scripts/install.sh
./scripts/install.sh

# Edit .env and add your OpenRouter API key
nano .env

# Start server
./chatbot
