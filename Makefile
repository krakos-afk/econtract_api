CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wno-deprecated-declarations
INCLUDES = -I/usr/include -I/usr/local/include -Isrc
LIBS = -lssl -lcrypto -lpqxx -lpq -lpthread

TARGET = econtract_api
BUILD_DIR = build

# Source files
SRCS = src/main.cpp \
       src/crypto/crypto_manager.cpp \
       src/blockchain/block.cpp \
       src/database/database_manager.cpp \
       src/services/contract_service.cpp \
       src/api/contract_api.cpp

# Object files
OBJS = $(SRCS:src/%.cpp=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LIBS)

# Compile source files
$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Run the server
run: $(TARGET)
	./$(TARGET)

# Rebuild
rebuild: clean all

# Show project structure
structure:
	@echo "Project Structure:"
	@find src -type f -name "*.cpp" -o -name "*.hpp" | sort

.PHONY: all clean run rebuild structure
