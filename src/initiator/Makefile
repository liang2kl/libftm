INITIATOR_SUFFIX = start config types
INITIATOR_OBJS = $(patsubst %,initiator_%.o,$(INITIATOR_SUFFIX))

initiator.o: initiator_temp.o $(INITIATOR_OBJS)
	$(AR) rc initiator.o $(INITIATOR_OBJS) initiator_temp.o

$(INITIATOR_OBJS): %.o: %.c %.h
	$(CC) -c $(LIBNL_INCLUDE) $< -o $@

initiator_temp.o: initiator.c initiator.h
	$(CC) -c $(LIBNL_INCLUDE) initiator.c -o initiator_temp.o