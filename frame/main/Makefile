include  ../../Makefile.rules

INC = -I ../share/include -I ../../common/include
LIB = -lpthread

OBJS = EventHandler.o Connection.o Connector.o Listener.o EventManager.o MtWorker.o Processor.o
DEST = ./libFrame.a

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
	echo
clean:
	rm -f $(OBJS) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEST) $(DEPS)
-include $(DEPS)
