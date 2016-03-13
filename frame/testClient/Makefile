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

DST2 = ./testTraffic
OBJ2 = testTraffic.o

DST3 = ./testForward
OBJ3 = testForward.o

all: $(DEST) $(DST2) $(DST3)

$(DEST): $(OBJS)
	g++ -o $@ $(OBJS) $(DEPS) $(LIB)
$(DST2): $(OBJ2)
	g++ -o $@ $(OBJ2) $(DEPS) $(LIB)
$(DST3): $(OBJ3)
	g++ -o $@ $(OBJ3) $(DEPS) $(LIB)

.cpp.o:
	g++ -fPIC -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

clean:
	rm -f $(OBJS) $(DST2) $(OBJ2) $(DST3) $(OBJ3) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEST)
