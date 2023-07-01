# COMPILATOR #
COMP = mpicc

# SOURCE #
SRC = ./src

# INCLUDES #
# Flag -I: include
INC = -I ./headers

FLAGS = -Wall -Werror

# OUTPUT (Program) #
OUTPUT = psrs

# FILL W/ DESIRED OBJECT FILES HERE #
OBJECTS: mpi_routines.o psrs.o slice.o

# Flag -o: output
all: $(OBJECTS)
	$(COMP) $(SRC)/main.c $(OBJECTS) -o $(OUTPUT) $(INC) $(FLAGS)

# Instructions to make *.o #
%.o: $(SRC)/%.c
	$(COMP) -c $< $(INC) $(FLAGS) -o $@

run:
	./$(OUTPUT)

clean:
	rm -f *.o

zip:
	zip -r psrs.zip Makefile ./src ./headers