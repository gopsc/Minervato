
# set the project version
VERSION := 0.26.1

# set the application name
APP_NAME := lci

# set the install directories
INSTL_DIRS := /usr/local/bin

# set the compiler
CC := g++

# set the linked libraries
LIBS := --std=c++20

# set all header directories
INC_DIRS := -I . 

# set the installing headers
INSTL_INCS := $(wildcard ./*.h)

# set flags of the compiler
CFLAGS := -O3 #-Wall -Werror

# set all .PHONY
.PHONY: all

# set the output list 
all: $(APP_NAME)

#-----------------------------------
$(APP_NAME): main.o ansi.o utf8.o Color.o ku500.o \
	fsm.o CommonThread.o KeyboardEvent.o \
	Drawable.o FrameBuf.o Area.o CmdArea.o InputBox.o
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -lpthread -o $@

main.o: src/main.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

ansi.o: src/ansi.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

utf8.o: src/utf8.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

Color.o: src/Color.cpp
	$(CC) $(CFALGS) $^ $(INC_DIRS) $(LIBS) -c

ku500.o: src/ku500.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

fsm.o: src/fsm.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

CommonThread.o: src/CommonThread.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

KeyboardEvent.o: src/KeyboardEvent.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

Drawable.o: src/Drawable.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

FrameBuf.o: src/FrameBuf.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

Area.o: src/Area.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

CmdArea.o: src/CmdArea.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c

InputBox.o: src/InputBox.cpp
	$(CC) $(CFLAGS) $^ $(INC_DIRS) $(LIBS) -c


clear:
	rm *.o $(APP_NAME)
	
install:
	cp $(APP_NAME) $(INSTL_DIRS)/$(APP_NAME)

uninstall :
	rm $(INSTL_DIRS)/$(APP_NAME)
