TARGET_EXEC ?= pumpcontrol.out
OS := $(shell uname)
BUILD_DIR ?= ./build
LIB_DIR ?= ./lib
OS_ID := $(shell grep '^ID=' /etc/os-release | sed s/ID=//)

ENCRYPTION ?= on
ifeq ($(ENCRYPTION), on)
FLAG_NO_ENCRYPTION =
ENCRYPTION_LIB_FLAGS = -lcrypto -lwibucm
else
FLAG_NO_ENCRYPTION = -DNO_ENCRYPTION
ENCRYPTION_LIB_FLAGS = 
endif

SRC_DIRS = ./src \
	$(shell find $(LIB_DIR) -type d -path \*src -not -path \*test\*)

3DP_LIBS = $(shell find $(LIB_DIR) -name *.a)
3DP_DIRS = $(dir $(3DP_LIBS))

SRCS = $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)

ifeq ($(ENCRYPTION), on)
ifneq ("$(wildcard ./private_src/decrypt.cpp)","")
SRCS := $(filter-out ./src/decrypt.cpp,$(SRCS))
SRCS += ./private_src/decrypt.cpp
endif
else
SRCS := $(filter-out ./src/decrypt.cpp,$(SRCS))
SRCS := $(filter-out ./src/cryptobuffer.cpp,$(SRCS))
SRCS := $(filter-out ./src/cryptohelpers.cpp,$(SRCS))
endif

REALDRIVERS ?= on
ifeq ($(REALDRIVERS), on)
FLAG_NO_REALDRIVERS =
ifeq ($(OS_ID), raspbian)
GPIO_LIB_FLAG=-lpigpio
else
GPIO_LIB_FLAG=
endif
else
FLAG_NO_REALDRIVERS = -DNO_REALDRIVERS
GPIO_LIB_FLAG =
SRCS := $(filter-out ./lib/firmatacpp/%,$(SRCS))
SRCS := $(filter-out ./lib/serial/%,$(SRCS))
SRCS := $(filter-out ./src/pumpdriverfirmata.cpp,$(SRCS))
SRCS := $(filter-out ./src/pumpdrivershield.cpp,$(SRCS))
SRCS := $(filter-out ./src/iodrivergpio.cpp,$(SRCS))
endif

OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)
INC_DIRS = $(shell find $(SRC_DIRS) -type d)
INC_DIRS += $(3DP_DIRS)
INC_DIRS += ./lib
INC_DIRS += ./lib/firmatacpp/include
INC_DIRS += ./lib/serial/include
INC_FLAGS = $(addprefix -I,$(INC_DIRS))

ifeq ($(OS), Darwin)
# Run MacOS commands 
LDFLAGS := -g -L/usr/local/opt/openssl/lib -L/usr/local/Cellar/boost/1.63.0/lib/ -lcrypto -lboost_system -lboost_regex -lboost_program_options -framework IOKit -framework CoreFoundation -framework WibuCmMacX
INC_DIRS += /usr/local/opt/openssl/include
INC_DIRS += /usr/local/Cellar/boost/1.63.0/include/
else
# check for Linux and run other commands
LDFLAGS := -g -lboost_system -lboost_regex -lboost_program_options -lpthread $(ENCRYPTION_LIB_FLAGS) $(GPIO_LIB_FLAG) -Wl,-E
endif


DOWNLOAD_FILES := $(shell find $(LIB_DIR) -name *.download)
DOWNLOADED_FILES := $(DOWNLOAD_FILES:%.download=%.downloaded)

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -std=c++11 -Wall -g -DOS_$(OS_ID) -DELPP_THREAD_SAFE $(FLAG_NO_ENCRYPTION) $(FLAG_NO_REALDRIVERS)

%.downloaded: %.download
	$(MKDIR_P) $(dir $<)/downloaded/$(basename $(notdir $<))/src
	curl  -L $(shell cat $<) --output $(dir $<)/downloaded/$(basename $(notdir $<))/src/$(notdir $(shell cat $<))
	touch $@

all: $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	@echo SRC Dirs: $(SRC_DIRS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c $(DOWNLOADED_FILES)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp $(DOWNLOADED_FILES)
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: realclean clean all list-srcs protect

realclean:
	$(RM) -r $(BUILD_DIR)
	$(RM) -r $(LIB_DIR)/downloaded
	$(RM) $(LIB_DIR)/*.downloaded

clean:
	$(RM) -r $(BUILD_DIR)

list-srcs:
	@echo $(SRCS)
	
protect: all ./private_src/pumpcontrol.wbc
	AxProtectorLin @./private_src/pumpcontrol.wbc

.PHONY: importLibs
importLibs: $(DOWNLOADED_FILES)

build: $(BUILD_DIR)/$(TARGET_EXEC)

-include $(DEPS)

MKDIR_P ?= mkdir -p
