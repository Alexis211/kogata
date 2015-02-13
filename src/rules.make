.PRECIOUS: %.o

AS = nasm
ASFLAGS = -felf -g

CC = i586-elf-gcc
CFLAGS += -ffreestanding -O2 -std=gnu99 -Wall -Wextra -Werror -I . -I ./include -g -Wno-unused-parameter
# CXX = i586-elf-g++
# CXFLAGS = -ffreestanding -O3 -Wall -Wextra -I . -I ./include -fno-exceptions -fno-rtti
LD = i586-elf-gcc
LDFLAGS += -ffreestanding -O2 -nostdlib -lgcc

all: $(OUT)

%.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIB)

%.lib: $(OBJ)
	$(LD) $(LDFLAGS) -r -o $@ $^ $(LIB)

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# %.o: %.cpp
#	$(CXX) -c $< -o $@ $(CXFLAGS)

clean:
	rm */*.o || true
mrproper: clean
	rm $(OUT) || true

rebuild: mrproper all
