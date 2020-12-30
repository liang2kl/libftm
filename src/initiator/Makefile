INITIATOR_SUFFIX = initiator start config types
INITIATOR_OBJS = $(patsubst %,initiator_%.o,$(filter-out initiator $(INITIATOR_SUFFIX))) + initiator.o

initiator.o: $(INITIATOR_OBJS)
	$(AR) rc initiator.a $^

$(INITIATOR_OBJS): %.o: %.c %.h
	$(CC) -c $(LIBNL_INCLUDE) $< -o $@