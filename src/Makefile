##############################
# mingw32-make             # Build without optimizations
# mingw32-make OPT=2       # Build with -O2 optimization
# mingw32-make OPT=3       # Build with -O3 optimization
# mingw32-make OPT=4       # Build with -Ofast optimization
# mingw32-make run         # Compiles and runs the program
# mingw32-make run OPT=2   # Compiles with -O2 and runs the program
# mingw32-make run OPT=3   # Compiles with -O3 and runs the program
# mingw32-make run OPT=4   # Compiles with -Ofast and runs the program
# mingw32-make server      # Build the WebSocket server
# mingw32-make server OPT=3 # Build server with -O3
# mingw32-make run-server   # Build and run the server
# mingw32-make clean       # Remove only .o files
# mingw32-make clean-all   # Remove .o and .exe files
##############################

# Compiler
CXX := g++

# Optimization Flags (set by user)
ifeq ($(OPT),2)
    OPTIMIZE := -O2
else ifeq ($(OPT),3)
    OPTIMIZE := -O3
else ifeq ($(OPT),4)
    OPTIMIZE := -Ofast
else
    OPTIMIZE := 
endif

# Compiler Flags
CXXFLAGS := -Wall -Wextra -std=c++17 -O3 -flto $(OPTIMIZE)

# Find all .cpp files in the current directory
SRCS := $(wildcard *.cpp)

# Convert .cpp files to .o files
OBJS := $(SRCS:.cpp=.o)

# Output Executable
TARGET := Domino.exe

# ====== WebSocket Server Configuration ======
WS_LIBS := -lws2_32 -lwsock32
WS_TARGET := DominoServer.exe
# Server sources: everything except main.cpp, plus WsServer.cpp
WS_SRCS := $(filter-out main.cpp, $(wildcard *.cpp))
WS_OBJS := $(patsubst %.cpp,ws_%.o, $(WS_SRCS))

# Default Build Rule
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilation Rule for .cpp files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ====== WebSocket Server Build Rules ======
server: $(WS_TARGET)

$(WS_TARGET): $(WS_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(WS_LIBS)

ws_%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run-server: server
	@echo "Running $(WS_TARGET)..."
	@$(WS_TARGET)

# Clean Rules
clean:
	del /Q *.o 2>nul

clean-all:
	del /Q *.o $(TARGET) $(WS_TARGET) 2>nul

# Run the compiled program
run: all
	@echo "Running $(TARGET)..."
	@$(TARGET)
