INITIATOR_SUFFIX = temp config start types
INITIATOR_FULL = $(patsubst %,initiator_%,$(INITIATOR_SUFFIX))
INITIATOR_OBJS = $(patsubst %,%.o,$(INITIATOR_FULL))

initiator.o: $(INITIATOR_OBJS)
	ar rc initiator.o $(INITIATOR_OBJS)

initiator_temp.o: initiator.c initiator.h
	$(CC) -c -o initiator_temp.o $(LIBNL_INCLUDE) initiator.c

initiator_start.o: initiator_start.c initiator_start.h
	$(CC) -c $(LIBNL_INCLUDE) initiator_start.c

initiator_config.o: initiator_config.c initiator_config.h
	$(CC) -c $(LIBNL_INCLUDE) initiator_config.c

initiator_types.o: initiator_types.c initiator_types.h
	$(CC) -c $(LIBNL_INCLUDE) initiator_types.c