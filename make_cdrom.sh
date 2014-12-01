#!/bin/sh

mkdir -p cdrom/boot/grub

if [ ! -e cdrom/boot/grub/stage2_eltorito ]; then
	echo "Please copy grub's stage2_eltorito to cdrom/boot/grub."
	exit -1
fi

cp menu_cdrom.lst cdrom/boot/grub/menu.lst
cp kernel/kernel.bin cdrom; strip cdrom/kernel.bin

genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -input-charset ascii \
	-A macroscope -o cdrom.iso cdrom
