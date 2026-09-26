CXX = g++
CXXFLAGS = -g -pedantic -Wall -Wextra -O2
LDFLAGS = -pthread

SRC_DIR = src
BUILD_DIR = build
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TARGET = dns

.PHONY: all run test test-args test-filter clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(wildcard $(SRC_DIR)/*.hpp) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: test-args test-filter

test-args: $(TARGET)
	@./tests/argument_tests.sh

test-filter: $(TARGET)
	@./tests/filter_tests.sh

run: $(TARGET)
	./$(TARGET) -s 8.8.8.8 -p 4400 -f tests/blocklists/blocked_domains -v

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
