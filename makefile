cd ~/econtract-api

cat > Makefile << 'EOF'
CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -DCROW_ENABLE_SSL
INCLUDES = -I/usr/include/pqxx -I/usr/include/postgresql -I/usr/include -I/usr/local/include
LIBS = -lssl -lcrypto -lpqxx -lpq -lpthread

TARGET = econtract_api
SRC = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
EOF