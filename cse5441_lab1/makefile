  # the compiler: icc for using intel compiler
  CC = icc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
  #CFLAGS  = -g -Wall


  # the build target executable:
  TARGET = pc_serial

  all: $(TARGET)

  $(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c /fs/project/PAS1653/transform.o

  clean:
	$(RM) $(TARGET)