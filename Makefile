CC=gcc

SRC=kb.c tune-atsc.c
HED=kb.h tune-atsc.h
OBJ=kb.o tune-atsc.o

BIND=/usr/local/bin/
INCLUDE=-I../v4l-updatelee/linux/include

TARGET=tune-atsc

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLG) $(OBJ) -o $(TARGET) $(CLIB) -lm

$(OBJ): $(HED)

install: all
	cp $(TARGET) $(BIND)

uninstall:
	rm $(BIND)$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET) *~ ._*

%.o: %.c
	$(CC) $(INCLUDE) -c $< -o $@
