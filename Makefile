TOP_PATH = $(shell pwd)
SRC_PATH = $(TOP_PATH)/src
TARGET = ftm
INSTALL_DIR = /usr/sbin
CC = gcc
AR = ar
MAKE = make
CFLAGS = -g
LIBNL_INCLUDE = -I /usr/include/libnl3
LIBNL_LIB = -lnl-3 -lnl-genl-3
OBJS = initiator.o responder.o nl.o
OBJS_PATHS = $(foreach obj,$(OBJS),$(SRC_PATH)/$(basename $(obj))/$(obj))

export LIBNL_INCLUDE CC AR

define make_sub_rules
$(SRC_PATH)/$(basename $(1))/$(1): \
$(wildcard $(SRC_PATH)/$(basename $(1))/*.c) \
$(wildcard $(SRC_PATH)/$(basename $(1))/*.h) 
endef

define make_sub_cmd
cd $(SRC_PATH)/$(basename $(1)) && $(MAKE)
endef

$(TARGET): $(OBJS_PATHS) $(SRC_PATH)/main.c
	$(CC) $(CFLAGS) $(SRC_PATH)/main.c $(OBJS_PATHS) \
	$(LIBNL_INCLUDE) $(LIBNL_LIB) -o $(TOP_PATH)/$(TARGET)
	@echo
	@echo Build finished.

$(call make_sub_rules,initiator.o)
	$(call make_sub_cmd,initiator.o)

$(call make_sub_rules,responder.o)
	$(call make_sub_cmd,responder.o)

$(call make_sub_rules,nl.o)
	$(call make_sub_cmd,nl.o)

.PHONY: clean
clean:
	find . -name *.o -type f -exec rm -rf {} \;

install: $(TARGET)
	cp -f $< $(INSTALL_DIR)

uninstall: $(INSTALL_DIR)/$(TARGET)
	rm -f $<