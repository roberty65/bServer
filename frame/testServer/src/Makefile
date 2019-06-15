include  ../../../Makefile.rules

COM_INC=../../../common/include
COM_SRC=../../../common/src/
COM_LIB=../../../common/lib/

FSH_INC=../../share/include
FSH_SRC=../../share/src/
FSH_LIB=../../share/lib/

FRM_SRC=../../main/

INC = -I $(COM_INC) -I $(FSH_INC) -I $(FRM_SRC)
LIB = -L $(FRM_SRC) -lFrame -L $(FSH_LIB) -lProcessor -L $(COM_LIB) -lcommon -lpthread

DEST = ../bin/testServerd
OBJS = main.o Echo.o

DST2 = ../bin/mtServer
OBJ2 = mtServer.o

DST3 = ../bin/otEchod
OBJ3 = otEchod.o

DEPS = $(patsubst %.o, .%.d, $(OBJS) $(OBJ2) $(OBJ3))

.PHONY: all clean distclean mkdirs
all: mkdirs $(DEST) $(DST2) $(DST3)

$(DEST): $(OBJS)
	@mkdir -p ../bin
	g++ -o $@ $(CXXFLAGS) $(LDFLAGS) $(OBJS) $(LIB)
$(DST2): $(OBJ2)
	g++ -o $@ $(CXXFLAGS) $(OBJ2) $(LDFLAGS) -L $(COM_LIB) -lcommon -lpthread
$(DST3): $(OBJ3)
	g++ -o $@ $(CXXFLAGS) $(OBJ3) $(LDFLAGS) -L $(COM_LIB) -lcommon
Echo.o: ../../../serverd/src/Echo.cpp ../../../serverd/src/Echo.h
	g++ -c -o $@ $(INC) $(CXXFLAGS) $<
.cpp.o:
	g++ -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

mkdirs:
	mkdir -p ../bin
clean:
	rm -f $(OBJS) $(OBJ2) $(OBJ3) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEPS) $(DEST) $(DST2) $(DST3) ../logs/server.log
-include $(DEPS)
