obj-m += counter.o
CC = gcc
all:    mod bin
clean:  modclean binclean
bin:
	${CC} -o ctl ctl.c
binclean:
	rm -f ctl
mod:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
modclean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean