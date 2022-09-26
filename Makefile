src = $(wildcard *.c)
objs = $(patsubst %.c,%.o,$(src))
app : $(objs)
	gcc $(objs) -o app
$(objs) : $(src)
	gcc -c $(src)

.PHONY : clean
clean :
	-rm app $(objs)
