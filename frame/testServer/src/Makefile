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

all: $(DEST)

$(DEST): $(OBJS)
	g++ -o $@ $(OBJS) $(DEPS) $(LIB)

.cpp.o:
	g++ -fPIC -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

clean:
	rm -f $(OBJS) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEST) ../logs/server.log
