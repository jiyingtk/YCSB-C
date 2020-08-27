CC=g++
CFLAGS=-std=c++11 -c -g -Wall -fgnu-tm
INCLUDES=-I../ -I/home/kvgroup/tcj/leveldb/include -I/home/kvgroup/tcj/titan/build/rocksdb -I/home/kvgroup/tcj/titan/build/rocksdb/include -I/home/kvgroup/tcj/titan/include
SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)

all: $(SOURCES) $(OBJECTS)

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	$(RM) $(OBJECTS)

