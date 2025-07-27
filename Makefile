# ----------------------------------------------------------------------
# Written by Raph.K.
#

GCC = gcc
GPP = g++
AR  = ar
WRC = windres

# Base PATH
BASE_PATH = .
SRC_PATH  = $(BASE_PATH)/src
RES_PATH  = $(BASE_PATH)/res

# TARGET settings
TARGET_PKG = DataRecoveryTool_W64
TARGET_DIR = ./bin
TARGET_OBJ = ./obj

# DEFINITIONS
DEFS  = -DUNICODE -D_UNICODE

# MSYS2 MinGW-W64 Compiler optiops for compatible Windows Console.
COPTS  = -mwindows -mconsole 
COPTS += -ffast-math -fexceptions -fopenmp -O3 -s

# CC FLAG
CFLAGS  = -I$(SRC_PATH)
CFLAGS += -I$(RES_PATH)
CFLAGS += $(DEFS)
CFLAGS += $(COPTS)

# LINK FLAG
LFLAGS += -static
LFLAGS += -lshlwapi

SRCS = $(wildcard $(SRC_PATH)/*.cpp)
OBJS = $(SRCS:$(SRC_PATH)/%.cpp=$(TARGET_OBJ)/%.o)


.PHONY: prepare clean

all: prepare continue

continue: $(TARGET_DIR)/$(TARGET_PKG)

prepare:
	@mkdir -p $(TARGET_DIR)
	@mkdir -p $(TARGET_OBJ)

clean:
	@echo "Cleaning built targets ..."
	@rm -rf $(TARGET_DIR)/$(TARGET_PKG)
	@rm -rf $(TARGET_INC)/*.h
	@rm -rf $(TARGET_OBJ)/*.o

$(OBJS): $(TARGET_OBJ)/%.o: $(SRC_PATH)/%.cpp
	@echo "Compiling $< ..."
	@$(GPP) $(CFLAGS) -c $< -o $@

$(TARGET_DIR)/$(TARGET_PKG): $(OBJS)
	@echo "Linking $@ ..."
	@$(GPP) $(TARGET_OBJ)/*.o $(CFLAGS) $(LFLAGS) -o $@
	@echo "done."
