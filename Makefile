# Makefile for Linux targeting Linux

include config.mk

CXXFLAGS = -O2 -g -Wall -Wextra -Wpedantic -std=c++20
DEPFLAGS = -MMD -MP
LDFLAGS = -Wl,--copy-dt-needed-entries -lncursesw

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
.PHONY: clean install uninstall

clean:
	rm -rf $(BUILD_DIR)
	rm $(BINPATH)

install: all
	mkdir -p $(INSTALL_DIR)
	cp $(BINPATH) $(INSTALL_DIR)/$(BINNAME)
	strip $(INSTALL_DIR)/$(BINNAME)
	mkdir -p ~/.config/pomocom
	cp ./config/* ~/.config/pomocom/

uninstall:
	rm $(INSTALL_DIR)/$(BINNAME)
	rm -r ~/.config/pomocom

-include $(DEPS)
