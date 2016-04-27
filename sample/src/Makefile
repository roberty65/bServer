BSERVER_HOME=../../
BSERVER_FRAME_INC=$(BSERVER_HOME)frame/share/include
BSERVER_COMMON_INC=$(BSERVER_HOME)common/include

INC = -I $(BSERVER_FRAME_INC) -I $(BSERVER_COMMON_INC)
LIB = 

CFLAGS = -Wall -fPIC
CXXFLAGS = -Wall -fPIC
LDFLAGS  =

DEST = ../lib/libEcho.so
OBJS = Echo.o 

.PHONY: all clean distclean mkdirs
all: mkdirs $(DEST) $(LIBO)

$(DEST): $(OBJS)
	g++ -o $@ -shared $(CXXFLAGS) $(LDFLAGS) $(OBJS)

.cpp.o:
	g++ -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

mkdirs:
	mkdir -p ../bin ../lib ../logs
clean:
	rm -f $(OBJS) $(OBJ2) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEPS) $(DEST) $(LIBO) ../logs/*.log
