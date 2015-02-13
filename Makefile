DIRS = src/common/libkogata src/common/libc src/common/libalgo src/kernel src/lib/libkogata src/apps/init

all:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1;   \
	done

rebuild:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir rebuild || exit 1;   \
	done

clean:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean;   \
	done

mrproper:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir mrproper;   \
	done
