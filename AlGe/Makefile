EXEC=morpionSol
CC=g++
SRC=$(wildcard *.cpp)
HD=$(wildcard *.h)
OBJ=$(SRC:.cpp=.o)
DEF=-DBESTLEARNING

all: $(EXEC)


debug: clean
	$(MAKE) $(MAKEFILE) DEBUG="-g"

prof: clean
	$(MAKE) $(MAKEFILE) PROF="-pg"


opt: clean
	$(MAKE) $(MAKEFILE) OPT="-O2 -DNDEBUG"

boost: 
	$(MAKE) $(MAKEFILE) BOOST_LIB="/usr/local/lib/libboost_program_options.a"



$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(DEBUG) $(PROF) $(OPT) -o $(EXEC) $(BOOST_LIB)

%.o: %.cpp $(HD)
	$(CC) $(DEBUG) $(PROF) $(OPT) $(DEF) -c $< -o $@ 

clean:
	rm -f *.o
	rm -f *~
	rm -f $(EXEC)
