override CFLAGS+=-Wall -Wformat-security -D_FORTIFY_SOURCE=2 -O2

EXECS=aslr mmap-growth-direction
ARCH=$(shell dpkg --print-architecture)

# ppc64el/s390x/arm breaks when compiling -m32
ifneq ($(ARCH),ppc64el)
ifneq ($(ARCH),armhf)
ifneq ($(ARCH),arm64)
EXECS+=aslr32
endif
endif
endif

all: $(EXECS)

aslr: aslr.c
	$(CC) $(CFLAGS) -fPIC -pie -o $@ $< -ldl

# ppc64el/arm breaks when compiling -m32
ifneq ($(ARCH),ppc64el)
ifneq ($(ARCH),armhf)
ifneq ($(ARCH),arm64)
# s390x needs -m31 for building compat mode bins
  ifeq ($(ARCH),s390x)
  COMPAT_FLAG=-m31
  else
  COMPAT_FLAG=-m32
  endif

aslr32: aslr.c
	$(CC) $(CFLAGS) $(COMPAT_FLAG) -fPIC -pie -o $@ $< -ldl
endif
endif
endif

mmap-growth-direction: mmap-growth-direction.c
	$(CC) $(CFLAGS) -fPIC -pie -o $@ $<

clean:
	rm -f $(EXECS)
