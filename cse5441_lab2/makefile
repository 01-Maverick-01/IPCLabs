  # the compiler: icc for using intel compiler
  CC = icc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
  #CFLAGS  = #-g -Wall

  # linker flags
  LDFLAGS = -pthread


  # the build target executable:
  TARGET = lab2_pthreads

  all: $(TARGET)

  $(TARGET): $(TARGET).c
	$(CC) $(LDFLAGS) -o $(TARGET) $(TARGET).c /fs/project/PAS1653/transform.o
	$(CC) $(LDFLAGS) -o test test).c /fs/project/PAS1653/transform.o

  clean:
	$(RM) $(TARGET)