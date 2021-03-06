CC=g++
CFLAGS=-std=c++11 -g -Wall -pthread -I./
LDFLAGS= -lpthread -ltbb -lhiredis -L/home/kvgroup/tcj/leveldb/build -lleveldb -lsnappy -L/home/kvgroup/tcj/titan/build -ltitan -L/home/kvgroup/tcj/titan/build/rocksdb -lrocksdb
# DEBUG
# LDFLAGS= -lpthread -ltbb -lhiredis -L/home/kvgroup/tcj/leveldb/build -lleveldb -lsnappy -L/home/kvgroup/tcj/titan/build -Bstatic -ltitan -L/home/kvgroup/tcj/titan/build/rocksdb -lrocksdb
SUBDIRS=core db redis
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
EXEC=ycsbc

all: $(SUBDIRS) $(EXEC)

$(SUBDIRS):
	$(MAKE) -C $@

$(EXEC): $(wildcard *.cc) $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
	$(RM) $(EXEC)

.PHONY: $(SUBDIRS) $(EXEC)

