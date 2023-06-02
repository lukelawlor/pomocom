# Makefile for Linux targeting Linux

CXX = g++
CXXFLAGS = -g -Wall -Wextra -Wpedantic -std=c++20
DEPFLAGs = -MMD -MP
LDFLAGS = -Wl,--copy-dt-needed-entries -lncursesw

BINNAME = pomocom
BINPATH = ./$(BINNAME)

SRC_DIR = ./src
BUILD_DIR = ./build/linux

CXX_EXTENSION_FIND = -name "*.cc" -or -name "*.cpp" -or -name "*.cxx"

SRCS = $(shell find $(SRC_DIR) $(CXX_EXTENSION_FIND))
OBJS = $(SRCS:$(SRC_DIR)/%=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

all: $(BINPATH)

$(BINPATH): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

.DELETE_ON_ERROR:
.PHONY: clean

clean:
	rm $(BINPATH)
	rm -rf $(BUILD_DIR)

-include $(DEPS)
