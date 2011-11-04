CC=g++
CFLAGS=-c -Wall
LDFLAGS=-O2 -lm
SOURCES=Bot.cc MyBot.cc State.cc edt.cc Location.cc combat.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=MyBot

#Uncomment the following to enable debugging
CFLAGS += -g
#CFLAGS += -pg
#LDFLAGS += -pg
#CFLAGS += -DDEBUG

CFLAGS+=-O3 -funroll-loops
all: $(OBJECTS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o: *.h Makefile
	$(CC) $(CFLAGS) $< -o $@

clean: 
	-rm -f ${EXECUTABLE} ${OBJECTS} *.d
	-rm -f debug.txt

.PHONY: all clean

