#!/bin/bash

# Check if .env exists
if [ ! -f .env ]; then
    echo "[!] .env file not found. Creating from example..."
    cp .env.example .env
    echo "[!] Please edit .env and add your OpenRouter API key!"
    exit 1
fi

# Run the chatbot
./chatbot $@
