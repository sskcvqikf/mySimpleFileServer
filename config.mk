CXX = g++

PTHREAD = -pthread

BUILD_DIR = .

LIBS_DIR = ./lib
INCLUDE_DIR=./include

ifeq (${DEBUG}, gdb)
MODE = -ggdb
endif
ifeq (${DEBUG}, lldb)
MODE = -g
endif
ifndef DEBUG
MODE = -o2 -flto
endif

CXXFLAGS = -std=c++17 -Wall ${MODE} ${PTHREAD} -I${INCLUDE_DIR}

BUILD_DIR_GUARD = @mkdir -p $(BUILD_DIR)

GOAL = main

FILES = main
OBJS = $(FILES:%=$(BUILD_DIR)/%.o)
