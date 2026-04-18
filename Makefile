CC = clang
CFLAGS = -Wall -O3 -pthread -Iinclude
LDFLAGS = -lssl -lcrypto

TARGET = chatbot
SOURCES = src/main.c src/server.c src/openrouter.c src/utils.c src/web_handler.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "[+] Build successful: $(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "[+] Cleaned"

termux: all

install:
	cp $(TARGET) $$PREFIX/bin/
	chmod +x $$PREFIX/bin/$(TARGET)
	@echo "[+] Installed to Termux"

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean termux install run
