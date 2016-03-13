
INC = -I ../../common/include -I ../include
LIB = -lpthread

CXXFLAGS = -Wall -g -fPIC -O3
LDFLAGS  =

OBJS = Block.o Message.o
DEST = ../lib/libProcessor.a

all: $(DEST)

$(DEST): $(OBJS)
	ar cru $@ $(OBJS)

.cpp.o:
	g++ -fPIC -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

clean:
	rm -f $(OBJS) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEST)
