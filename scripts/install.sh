#!/bin/bash

echo "[*] Installing Termux AI Chatbot..."

# Update packages
pkg update -y && pkg upgrade -y

# Install dependencies
pkg install clang make openssl libcurl -y

# Build
make clean
make termux

# Create .env file if not exists
if [ ! -f .env ]; then
    cp .env.example .env
    echo "[!] Please edit .env file and add your OpenRouter API key"
fi

echo "[+] Installation complete!"
echo "[*] Run: ./chatbot"
