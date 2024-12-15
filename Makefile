UNAME_S = $(shell uname -s)
WORK_DIR  = $(shell pwd)
BUILD_DIR = $(WORK_DIR)/build

GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
END='\033[0m'

CC = g++
CPPFLAGS = -std=c++20 -O0 -g
# CPPFLAGS += -Wno-pointer-arith -Wno-newline-eof -Wno-unused-parameter -Wno-gnu-statement-expression
# CPPFLAGS += -Wno-gnu-compound-literal-initializer -Wno-gnu-zero-variadic-macro-arguments
CPPFLAGS += -I$(WORK_DIR)/include
#CPPFLAGS += `pkg-config sdl --cflags`
#CPPFLAGS += -DDEBUG
# FIXME: imtui dependency linking
CPPFLAGS += -I "./hikSDK/include"
CPPFLAGS += `pkg-config opencv4 --cflags`
CPPFLAGS += -L "./hikSDK/lib/amd64"

LDFLAGS += -lGL -lm -lpthread -ldl -lrt -lFormatConversion -lMediaProcess -lMvCameraControl -lMVRender -lMvUsb3vTL
LDFLAGS += `pkg-config opencv4 --libs`

SRC = $(wildcard src/*.cpp) $(wildcard src/**/*.cpp)
OBJ = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRC))))
BIN = armor

.PHONY: all clean

all: dirs $(BIN)

dirs:
	mkdir -p $(BUILD_DIR)

run: all
	$(BUILD_DIR)/$(BIN)



$(BIN): $(OBJ)
	#echo -e + $(GREEN)LN$(END) $(BUILD_DIR)/$(BIN)
	$(CC) -o $(BUILD_DIR)/$(BIN) $(OBJ) $(CPPFLAGS) $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp
	#@mkdir -p $(dir $@)
	#@echo -e + $(GREEN)CC$(END) $<
	$(CC) -o $@ -c $< $(CPPFLAGS)

clean:
	rm -rf $(BUILD_DIR)/$(BIN) $(OBJ)

