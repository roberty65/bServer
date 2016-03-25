COM_INC=../../common/include
COM_SRC=../../common/src/
COM_LIB=../../common/lib/

FSH_INC=../share/include
FSH_SRC=../share/src/
FSH_LIB=../share/lib/

FRM_SRC=../main/

INC = -I $(COM_INC) -I $(FSH_INC) -I $(FRM_SRC)
LIB =  -L $(COM_LIB) -lcommon -lpthread

CXXFLAGS = -Wall -g -fPIC -O3
LDFLAGS  =

DEST = ./testEcho
OBJS = testEcho.o

DST2 = ./testBenchmark
OBJ2 = testBenchmark.o

DST3 = ./testForward
OBJ3 = testForward.o

DST4 = ./testMessage
OBJ4 = testMessage.o

all: $(DEST) $(DST2) $(DST3) $(DST4)

$(DEST): $(OBJS)
	g++ -o $@ $(LDFLAGS) $(OBJS) $(DEPS) $(LIB)
$(DST2): $(OBJ2)
	g++ -o $@ $(LDFLAGS) $(OBJ2) $(DEPS) $(LIB)
$(DST3): $(OBJ3)
	g++ -o $@ $(LDFLAGS) $(OBJ3) $(DEPS) $(LIB)
$(DST4): $(OBJ4)
	g++ -o $@ $(LDFLAGS) $(OBJ4) $(DEPS) $(LIB) -L $(FSH_LIB) -lProcessor
.cpp.o:
	g++ -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

clean:
	rm -f $(OBJS) $(DST2) $(OBJ2) $(DST3) $(OBJ3) $(DST4) $(OBJ4) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEST)
