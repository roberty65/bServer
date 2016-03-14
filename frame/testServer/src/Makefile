COM_INC=../../../common/include
COM_SRC=../../../common/src/
COM_LIB=../../../common/lib/

FSH_INC=../../share/include
FSH_SRC=../../share/src/
FSH_LIB=../../share/lib/

FRM_SRC=../../main/

INC = -I $(COM_INC) -I $(FSH_INC) -I $(FRM_SRC)
LIB = -L $(FRM_SRC) -lFrame -L $(FSH_LIB) -lProcessor -L $(COM_LIB) -lcommon -lpthread

CXXFLAGS = -Wall -g -fPIC -O3
LDFLAGS  =

DEST = ../bin/testServerd
OBJS = main.o Echo.o 

DST2 = ../bin/mtServer
OBJ2 = mtServer.o

all: $(DEST) $(DST2)

$(DEST): $(OBJS)
	g++ -o $@ $(OBJS) $(LIB)
$(DST2): $(OBJ2)
	g++ -o $@ $(OBJ2) -L $(COM_LIB) -lcommon -lpthread
.cpp.o:
	g++ -fPIC -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

clean:
	rm -f $(OBJS) $(OBJ2) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEST) $(DST2) ../logs/server.log
