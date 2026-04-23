CC ?= gcc
CFLAGS  += -Wall -Wextra -Wformat -Wformat-security -Wno-unused-parameter -Wno-missing-field-initializers \
           -Os -fstack-protector-strong -Iinclude
CPPFLAGS += -D_GNU_SOURCE -D_FORTIFY_SOURCE=2
LDFLAGS += -lssl -lcrypto -lcurl -lm -lqrencode

SRC    := $(shell find src -name "*.c")
OBJ    := $(SRC:.c=.o)
TARGET := engsel

all: $(TARGET)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
