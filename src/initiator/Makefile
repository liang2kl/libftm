INITIATOR_SUFFIX = start config types
INITIATOR_OBJS = $(patsubst %,initiator_%.o,$(INITIATOR_SUFFIX))

initiator.o: initiator_temp.o $(INITIATOR_OBJS)
	$(AR) rc initiator.o $^

$(INITIATOR_OBJS): %.o: %.cpp %.h
	$(CC) -c $(LIBNL_INCLUDE) $< -o $@

initiator_temp.o: initiator.cpp initiator.h
	$(CC) -c $(LIBNL_INCLUDE) initiator.cpp -o initiator_temp.o