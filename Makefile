CC := gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
LDFLAGS ?=
TARGET := minishell
SOURCES := minishell.c

.PHONY: all clean check smoke

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $(TARGET)

check: all smoke

smoke: all
	./scripts/smoke-test.sh ./$(TARGET)

clean:
	rm -f $(TARGET) *.o *.out

