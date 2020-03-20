  # the compiler: icc for using intel compiler
  CC = icc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
  CFLAGS = -g -Wall

  # linker flags
  LDFLAGS = -fopenmp


  # the build target executable:
  TARGET = lab3_omp

  all: $(TARGET)

  $(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(TARGET).c /fs/project/PAS1653/transform.o
#	$(CC) $(CFLAGS) $(LDFLAGS) -o test test.c /fs/project/PAS1653/transform.o

  clean:
	$(RM) $(TARGET)
