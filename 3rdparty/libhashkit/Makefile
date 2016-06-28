SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

libhashkit.a : $(OBJS)
	ar cru $@ $(OBJS)
	ranlib $@

%.o : %.c
	gcc -std=gnu99 -c $< -I..

clean : 
	rm -fv *.o libhashkit.a

