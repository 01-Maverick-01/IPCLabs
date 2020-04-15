  # the compiler: icc for using intel compiler
  CC = nvcc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
  CFLAGS = -O

  # the build target executable:
  TARGET = lab5_cuda

  all: $(TARGET)

  $(TARGET): $(TARGET).cu
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cu

  clean:
	$(RM) $(TARGET)

