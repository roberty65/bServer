include  ../../Makefile.rules

INC = -I ../include
# -I /usr/include/libxml2
LIB =
# -lxml2 -lz -lm


DEST = ../lib/libcommon.a
OBJS = xbs_naddr.o xbs_io.o xbs_socket.o nam2val.o md5.o \
       slab_cache.o rbtree.o proto_h16.o bprof.o ConfigProperty.o \
       MemoryBuffer.o
#
# has depends on libxml2 log4cpp
# log.o xmlconfig.o 

DEPS = $(patsubst %.o, .%.d, $(OBJS))

.PHONY: all clean distclean mkdirs
all: mkdirs $(DEST)

$(DEST): $(OBJS)
	ar cru $@ $(OBJS)

.cpp.o:
	g++ -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

mkdirs:
	mkdir -p ../lib
clean:
	rm -f $(OBJS) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEST) $(DEPS)
-include $(DEPS)

