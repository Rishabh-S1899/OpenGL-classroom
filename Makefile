# # --------------------------------------------------
# #          Makefile for OpenGL Project
# # --------------------------------------------------

# # 1. Compiler and Flags
# # --------------------------------------------------
# CXX = g++
# CXXFLAGS = -std=c++11 -g -Wall -DGLM_ENABLE_EXPERIMENTAL

# # --------------------------------------------------
# # 2. Project Files and Directories
# # --------------------------------------------------
# TARGET = main.exe

# # Directories to search for source files
# VPATH = common external/glfw-3.1.2/deps

# INCLUDES = -I. -Icommon -Iexternal/glfw-3.1.2/deps

# # Source files
# CPP_SRCS = $(wildcard *.cpp common/*.cpp)
# C_SRCS   = $(wildcard external/glfw-3.1.2/deps/*.c)

# # Object files
# OBJS = $(CPP_SRCS:.cpp=.o) $(C_SRCS:.c=.o)

# # --------------------------------------------------
# # 3. Libraries to Link
# # --------------------------------------------------
# LDLIBS = -lglfw3 -lglew32 -lopengl32 -lgdi32

# # --------------------------------------------------
# # 4. Build Rules
# # --------------------------------------------------
# all: $(TARGET)

# $(TARGET): $(OBJS)
# 	$(CXX) -o $@ $^ $(LDLIBS)
# 	@echo "Build complete. Executable created: $(TARGET)"

# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# %.o: %.c
# 	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# clean:
# 	@echo "Cleaning up project files..."
# 	rm -f $(OBJS) $(TARGET)
# 	@echo "Cleanup complete."

# .PHONY: all clean


# --------------------------------------------------
#          Makefile for OpenGL Project (fixed)
# --------------------------------------------------

# 1. Compiler and Flags
# --------------------------------------------------
CXX = g++
CXXFLAGS = -std=c++11 -g -Wall -DGLM_ENABLE_EXPERIMENTAL

# --------------------------------------------------
# 2. Project Files and Directories
# --------------------------------------------------
TARGET = main.exe

# Directories to search for source files (keep same as old setup)
VPATH = common external/glfw-3.1.2/deps

# Include paths: add 'include' (for glad headers) and keep old includes
INCLUDES = -I. -Iinclude -Icommon -Iexternal/glfw-3.1.2/deps -Iexternal/glfw-3.1.2/include

# Source files
CPP_SRCS = main.cpp $(wildcard common/*.cpp)
# pick up any C sources (including glad.c if present at project root)
C_SRCS   = $(wildcard *.c) $(wildcard external/glfw-3.1.2/deps/*.c)

# Object files
OBJS = $(CPP_SRCS:.cpp=.o) $(C_SRCS:.c=.o)

# --------------------------------------------------
# 3. Libraries to Link (Windows / MinGW style like your old file)
# --------------------------------------------------
# If you use GLAD you do NOT need -lglew32. If you still want to keep backward
# compatibility and you have GLEW installed, keep -lglew32. If not, remove it.
LDLIBS = -lglfw3 -lglew32 -lopengl32 -lgdi32

# If your libraries are in a nonstandard directory set LIB_DIR here:
# LIB_DIR = -L/path/to/libs
LIB_DIR =

# --------------------------------------------------
# 4. Build Rules
# --------------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIB_DIR) $(LDLIBS)
	@echo "Build complete. Executable created: $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "Cleaning up project files..."
	rm -f $(OBJS) $(TARGET)
	@echo "Cleanup complete."

.PHONY: all clean
