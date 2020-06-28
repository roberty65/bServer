include ../../Makefile.rules

COM_INC=../../common/include
COM_SRC=../../common/src/
COM_LIB=../../common/lib/

FSH_INC=../../frame/share/include
FSH_SRC=../../frame/share/src/
FSH_LIB=../../frame/share/lib/

FRM_SRC=../../frame/main/

INC = -I $(FRM_SRC) -I $(FSH_INC) -I $(COM_INC)
LIB = -L $(FRM_SRC) -lFrame -L $(FSH_LIB) -lProcessor -L $(COM_LIB) -lcommon -ldl -lpthread -Wl,--dynamic-list=export.syms

DEST = ../bin/bServerd
OBJS = main.o

LIBO = ../lib/libEcho.so
OBJ2 = Echo.o 

DEPS = $(patsubst %.o, .%.d, $(OBJS) $(OBJ2))

.PHONY: all clean distclean mkdirs
all: mkdirs $(DEST) $(LIBO)

$(DEST): $(OBJS)
	#g++ -o $@ -rdynamic $(CXXFLAGS) $(LDFLAGS) $(OBJS) $(LIB)
	g++ -o $@ $(OBJS) $(LIB)

$(LIBO): $(OBJ2)
	g++ -o $@ -shared $(CXXFLAGS) $(LDFLAGS) $(OBJ2) #-ldl

.cpp.o:
	g++ -c -o $@ $(INC) $(CXXFLAGS) $<
.c.o:
	gcc -c -o $@ $(INC) $(CXXFLAGS) $<

mkdirs:
	@mkdir -p ../bin ../lib ../logs
clean:
	rm -f $(OBJS) $(OBJ2) *~ *.s *.ii *.i
distclean: clean
	rm -f $(DEPS) $(DEST) $(LIBO) ../logs/*.log
-include $(DEPS)
