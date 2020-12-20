INITIATOR_SUFFIX = temp config start types
INITIATOR_FULL = $(patsubst %,initiator_%,$(INITIATOR_SUFFIX))
INITIATOR_OBJS = $(patsubst %,%.o,$(INITIATOR_FULL))
INITIATOR_OBJS_PATHS = $(patsubst %,$(TMP_PATH)/initiator/%.o,$(INITIATOR_FULL))

initiator.a: $(INITIATOR_OBJS)
	ar rc initiator.a $(INITIATOR_OBJS_PATHS);\
	mv initiator.a $(TMP_PATH)

initiator_temp.o: initiator.c initiator.h
	$(CC) -c -o initiator_temp.o $(LIBNL_INCLUDE) initiator.c
	mv initiator_temp.o $(TMP_PATH)/initiator

initiator_start.o: initiator_start.c initiator_start.h
	$(CC) -c $(LIBNL_INCLUDE) initiator_start.c
	mv initiator_start.o $(TMP_PATH)/initiator

initiator_config.o: initiator_config.c initiator_config.h
	$(CC) -c $(LIBNL_INCLUDE) initiator_config.c
	mv initiator_config.o $(TMP_PATH)/initiator

initiator_types.o: initiator_types.c initiator_types.h
	$(CC) -c $(LIBNL_INCLUDE) initiator_types.c
	mv initiator_types.o $(TMP_PATH)/initiator