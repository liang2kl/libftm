VPATH = src:src/nl:src/initiator:src/responder
OBJS = main.o initiator.a responder.o nl.o
TOP_PATH = $(shell pwd)
SRC_PATH = $(TOP_PATH)/src
TMP_PATH = $(TOP_PATH)/tmp
OBJS_PATHS = $(patsubst %,$(TMP_PATH)/%, $(OBJS))
TARGET = ftm.out
CC = gcc
MAKE = make
CFLAGS = -g
LIBNL_INCLUDE = -I /usr/include/libnl3
LIBNL_LIB = -lnl-3 -lnl-genl-3

export VPATH LIBNL_INCLUDE SRC_PATH TOP_PATH TMP_PATH CC

$(TARGET): mktmp $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(OBJS_PATHS) $(LIBNL_INCLUDE) $(LIBNL_LIB) -o $(TOP_PATH)/$(TARGET)
	@echo Build finished. Run 'make clean' to clean up.

mktmp:
	-mkdir $(TMP_PATH)
	-mkdir $(TMP_PATH)/initiator

main.o: main.c
	$(CC) -c -o $(TMP_PATH)/main.o $(LIBNL_INCLUDE) $(SRC_PATH)/main.c

initiator.a:
	cd $(SRC_PATH)/initiator && $(MAKE)

responder.o:
	cd $(SRC_PATH)/responder && $(MAKE)

nl.o:
	cd $(SRC_PATH)/nl && $(MAKE)

.PHONY: clean
clean:
	-rm -r $(TMP_PATH)
