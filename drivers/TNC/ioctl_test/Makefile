# makefiles 
CC	:= gcc 


CFLAGS	:=  -O2 -Wall -I../common/

LDFLAGS	:= 

SRCS := main.c file_io.c

OBJ := ioctl_test

all: clean $(OBJ)

ioctl_test:
	$(CC) $(CFLAGS) $(SRCS) -o $@ 

#proc_test.o: proc_init.o
#	$(LD) -r $(LDFLAGS) $^ -o $@	

#$(module_E): $(objs_E)
#	$(LD) -r $(LDFLAGS) $^ -o $@
#	cp $@ /lib/modules/2.4.22/kernel/net/ipv4/

#$(module_V): $(objs_V)
#	$(LD) -r $(LDFLAGS) $^ -o $@
#	cp $@ /lib/modules/2.4.22/kernel/net/ipv4/


clean: 
	rm -f *.o *~ core $(OBJ)

