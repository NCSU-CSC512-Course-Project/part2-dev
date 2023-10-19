# Makefile for SeminalInputFeatureDetector
CXX = clang++
CXXFLAGS = -std=c++17
LINKER_FLAGS = -lclang

BIN_DIR = bin
SRC_DIR = src
OBJS_DIR = $(BIN_DIR)/objs

SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJS_DIR)/%.o, $(SRC))
EXE = $(BIN_DIR)/SeminalInputFeatureDetector

.PHONY: all main run

all: dirs main

run: all
	$(EXE)

main: $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) $(LINKER_FLAGS) -o $(EXE) 

dirs:
	mkdir -p $(BIN_DIR) $(OBJS_DIR)

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $< 