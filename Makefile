VPATH = src:src/nl:src/initiator:src/responder
OBJS = initiator.o responder.o nl.o
TOP_PATH = $(shell pwd)
SRC_PATH = $(TOP_PATH)/src
TMP_PATH = $(TOP_PATH)/tmp
OBJS_PATHS = $(foreach obj,$(OBJS),$(SRC_PATH)/$(basename $(obj))/$(obj))
TARGET = ftm
CC = gcc
MAKE = make
CFLAGS = -g
LIBNL_INCLUDE = -I /usr/include/libnl3
LIBNL_LIB = -lnl-3 -lnl-genl-3

export VPATH LIBNL_INCLUDE SRC_PATH TOP_PATH TMP_PATH CC

$(TARGET): $(OBJS_PATHS) $(SRC_PATH)/main.c
	$(CC) $(CFLAGS) $(SRC_PATH)/main.c $(OBJS_PATHS) $(LIBNL_INCLUDE) $(LIBNL_LIB) -o $(TOP_PATH)/$(TARGET)
	@echo Build finished. Run \'make clean\' to clean up.

$(SRC_PATH)/initiator/initiator.o: $(wildcard $(SRC_PATH)/initiator/*.c) $(wildcard $(SRC_PATH)/initiator/*.h)
	cd $(SRC_PATH)/initiator && $(MAKE)

$(SRC_PATH)/responder/responder.o: $(wildcard $(SRC_PATH)/responder/*.c) $(wildcard $(SRC_PATH)/responder/*.h)
	cd $(SRC_PATH)/responder && $(MAKE)

$(SRC_PATH)/nl/nl.o: $(wildcard $(SRC_PATH)/nl/*.c) $(wildcard $(SRC_PATH)/nl/*.h)
	cd $(SRC_PATH)/nl && $(MAKE)

.PHONY: clean
clean:
	find . -name *.o -type f -exec rm -rf {} \;

.PHONY: install
install: $(TARGET)
	mv -f $(TARGET) /usr/sbin