INC = -I ../include -I /usr/include/libxml2
LIB = -lxml2 -lz -lm

CXXFLAGS = -Wall -g -fPIC -O3
LDFLAGS  =

DEST = ../lib/libcommon.a
OBJS = xbs_naddr.o xbs_io.o xbs_socket.o nam2val.o md5.o \
       slab_cache.o rbtree.o proto_h16.o bprof.o
#
# has depends on libxml2 log4cpp
# log.o xmlconfig.o 

all: $(DEST)

$(DEST): $(OBJS)
	ar cru $@ $(OBJS)

.cpp.o:
	g++ -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

clean:
	rm -f $(OBJS) $(DEST) *~ *.s *.ii *.i
distclean: clean
	rm $(DEST)

