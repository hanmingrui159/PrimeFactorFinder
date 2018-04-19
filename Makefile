FLAGS = -Wall -g -std=gnu99 
DEPENDENCIES = filter.h

pfact : pfact.o
	gcc ${FLAGS} -o $@ $^ -lm

%.o: %.c ${DEPENDENCIES}
	gcc ${FLAGS} -c $<

clean: 
	rm -f *.o pfact

