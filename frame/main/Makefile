
INC = -I ../../common/include -I ../share/include
LIB = -lpthread

CXXFLAGS = -Wall -g -fPIC #-O3
LDFLAGS  =

OBJS = Log.o EventHandler.o Connection.o Connector.o Listener.o EventManager.o MtWorker.o Processor.o
DEST = ./libFrame.a

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
