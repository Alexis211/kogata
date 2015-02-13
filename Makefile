DIRS = src/common src/kernel

all:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir;   \
	done

rebuild:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir rebuild;   \
	done

clean:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean;   \
	done

mrproper:
	for dir in $(DIRS); do \
		$(MAKE) -C $$dir mrproper;   \
	done
