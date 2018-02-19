AS = as
LD = ld
CFLAGS =
LDFLAGS =

target = hello
srcs = hello.s
objs = $(srcs:.s=.o)


.PHONY: all
all: $(target)

$(target):$(objs) $(headers)
	$(LD) -o $@ $(objs) $(LDFLAGS)

$(objs):%.o:%.s
	@$(AS) -o $@ $<

clean:
	rm -f $(target) *.o

