  # the compiler: icc for using intel compiler
  CC = mpicc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
  CFLAGS = -Wall -g

  # linker flags
  LDFLAGS = -fopenmp


  # the build target executable:
  TARGET = lab4_hierarchical

  all: $(TARGET)

  $(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(TARGET).c /fs/project/PAS1653/transform.o

  clean:
	$(RM) $(TARGET)
