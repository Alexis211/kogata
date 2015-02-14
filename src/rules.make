.PRECIOUS: %.o

AS = nasm
ASFLAGS = -felf -g

CC = i586-elf-gcc
CFLAGS += -ffreestanding -std=gnu99 -Wall -Wextra -Werror -Wno-unused-parameter -I . -I ./include -g
# CXX = i586-elf-g++
# CXFLAGS = -ffreestanding -O3 -Wall -Wextra -I . -I ./include -fno-exceptions -fno-rtti
LD = i586-elf-gcc
LDFLAGS += -ffreestanding -nostdlib -lgcc

all: $(OUT)

$(OUT): $(OBJ) $(LIB)
	OUT=$(OUT); $(LD) $(LDFLAGS) `if [ $${OUT##*.} = lib ]; then echo -r; fi` -o $@ $^

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# %.o: %.cpp
#	$(CXX) -c $< -o $@ $(CXFLAGS)

clean:
	rm *.o */*.o || true
mrproper: clean
	rm $(OUT) || true

rebuild: mrproper all
