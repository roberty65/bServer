include  ../../../Makefile.rules

INC = -I ../../../common/include -I ../include
LIB = -lpthread

OBJS = Block.o Message.o
DEST = ../lib/libProcessor.a

DEPS = $(patsubst %.o, .%.d, $(OBJS))

.PHONY: all clean distclean mkdirs
all: $(DEST)

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
