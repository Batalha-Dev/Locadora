CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET   = locadora
TESTBIN  = testes_locadora

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp

$(TESTBIN): testes/testes.cpp
	$(CXX) $(CXXFLAGS) -o $(TESTBIN) testes/testes.cpp

# Compila e roda a suite de testes de consistencia
test: $(TESTBIN)
	./$(TESTBIN)

clean:
	del /Q $(TARGET).exe $(TESTBIN).exe 2>nul || rm -f $(TARGET) $(TESTBIN)

run: all
	./$(TARGET)
