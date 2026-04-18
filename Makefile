# ============================================
# TERMUX AI CHATBOT MAKEFILE
# ============================================

CC = clang
CFLAGS = -Wall -O3 -pthread -Iinclude
LDFLAGS = -lcurl

TARGET = chatbot

# Source files
SOURCES = src/main.c \
          src/server.c \
          src/openrouter.c \
          src/utils.c \
          src/web_handler.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# ============================================
# TARGETS
# ============================================

.PHONY: all clean termux run install help

all: $(TARGET)

$(TARGET): $(SOURCES)
	@echo "[*] Compiling Termux AI Chatbot..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)
	@echo "[+] Build successful: $(TARGET)"
	@echo "[*] Run: ./$(TARGET)"

# Compile individual objects (not used directly but kept for reference)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "[*] Cleaning..."
	rm -f $(TARGET) $(OBJECTS)
	@echo "[+] Clean complete"

# Build for Termux (same as all)
termux:
	@echo "[*] Building for Termux..."
	$(MAKE) clean
	$(MAKE) all

# Run the chatbot
run: $(TARGET)
	./$(TARGET) $(PORT)

# Install to Termux PATH
install: $(TARGET)
	@echo "[*] Installing to Termux..."
	cp $(TARGET) $$PREFIX/bin/
	chmod +x $$PREFIX/bin/$(TARGET)
	@echo "[+] Installed! Run 'chatbot' from anywhere"

# Uninstall
uninstall:
	@echo "[*] Uninstalling..."
	rm -f $$PREFIX/bin/$(TARGET)
	@echo "[+] Uninstalled"

# Show help
help:
	@echo "=========================================="
	@echo "    TERMUX AI CHATBOT - BUILD SYSTEM"
	@echo "=========================================="
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build chatbot"
	@echo "  make termux   - Clean and build for Termux"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make run      - Run chatbot (default port 5000)"
	@echo "  make install  - Install to Termux PATH"
	@echo "  make uninstall- Remove from Termux PATH"
	@echo "  make help     - Show this help"
	@echo ""
	@echo "Usage after build:"
	@echo "  ./chatbot              - Run on port 5000"
	@echo "  ./chatbot 8080         - Run on custom port"
	@echo ""
	@echo "Environment (.env file):"
	@echo "  OPENROUTER_API_KEY     - Your OpenRouter API key"
	@echo "  OPENROUTER_MODEL       - Model name (default: openai/gpt-3.5-turbo)"
	@echo "  PORT                   - Server port (default: 5000)"
	@echo ""

# Check dependencies
check-deps:
	@echo "[*] Checking dependencies..."
	@command -v clang >/dev/null 2>&1 || { echo "[!] clang not found. Run: pkg install clang"; exit 1; }
	@test -f $(PREFIX)/include/curl/curl.h || { echo "[!] libcurl not found. Run: pkg install libcurl"; exit 1; }
	@echo "[+] All dependencies OK"

# Create .env file if not exists
setup-env:
	@if [ ! -f .env ]; then \
		echo "[*] Creating .env file..."; \
		echo "OPENROUTER_API_KEY=sk-or-v1-YOUR-KEY-HERE" > .env; \
		echo "OPENROUTER_MODEL=openai/gpt-3.5-turbo" >> .env; \
		echo "PORT=5000" >> .env; \
		echo "[+] .env created. Please edit it with your API key!"; \
	else \
		echo "[*] .env file already exists"; \
	fi

# Full setup
setup: check-deps setup-env
	@echo "[*] Running build..."
	$(MAKE) clean
	$(MAKE) all
	@echo ""
	@echo "[+] Setup complete!"
	@echo "[*] Edit .env file if you haven't added your API key"
	@echo "[*] Run: ./chatbot"
