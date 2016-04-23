include  ../../Makefile.rules

COM_INC=../../common/include
COM_SRC=../../common/src/
COM_LIB=../../common/lib/

FSH_INC=../share/include
FSH_SRC=../share/src/
FSH_LIB=../share/lib/

FRM_SRC=../main/

INC = -I $(COM_INC) -I $(FSH_INC) -I $(FRM_SRC)
LIB =  -L $(COM_LIB) -lcommon -lpthread

DEST = ./testEcho
OBJS = testEcho.o

DST2 = ./testBenchmark
OBJ2 = testBenchmark.o

DST3 = ./testForward
OBJ3 = testForward.o

DST4 = ./testMessage
OBJ4 = testMessage.o

DEPS = $(patsubst %.o, .%.d, $(OBJS) $(OBJ2) $(OBJ3) $(OBJ4))

.PHONY: all clean distclean
all: $(DEST) $(DST2) $(DST3) $(DST4)

$(DEST): $(OBJS)
	g++ -o $@ $(LDFLAGS) $(OBJS) $(LIB)
$(DST2): $(OBJ2)
	g++ -o $@ $(LDFLAGS) $(OBJ2) $(LIB)
$(DST3): $(OBJ3)
	g++ -o $@ $(LDFLAGS) $(OBJ3) $(LIB)
$(DST4): $(OBJ4)
	g++ -o $@ $(LDFLAGS) $(OBJ4) $(LIB) -L $(FSH_LIB) -lProcessor
.cpp.o:
	g++ -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

clean:
	rm -f $(OBJS) $(OBJ2) $(OBJ3) $(OBJ4) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEST) $(DST2) $(DST3) $(DST4) $(DEPS)
-include $(DEPS)
