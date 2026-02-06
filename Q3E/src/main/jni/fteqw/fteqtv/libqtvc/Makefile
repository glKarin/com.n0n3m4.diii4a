
OBJS = msvc_sucks.o glibc_sucks.o

all: libqtvc.a

libqtvc.a: $(OBJS)
	rm -f $@
	$(AR) r $@ $^
	$(RANLIB) $@

clean:
	rm -f $(OBJS) libqtvc.a

