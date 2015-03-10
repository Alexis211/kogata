#!/bin/sh

if [ ! -e cdrom/boot/grub/stage2_eltorito ]; then
	mkdir -p cdrom/boot/grub
	echo "Please copy grub's stage2_eltorito to cdrom/boot/grub."
	exit -1
fi

# Copy system files to CDROM

cp src/kernel/kernel.bin cdrom; strip cdrom/kernel.bin
cp src/apps/init/init.bin cdrom; strip cdrom/init.bin

cp README.md cdrom

# Setup config files

cat > cdrom/boot/grub/menu.lst <<EOF
timeout 10
default 0

title   kogata OS
kernel  /kernel.bin root=io:/disk/atapi0 root_opts=l init=root:/init.bin

title   kogata OS without root
kernel  /kernel.bin init=io:/mod/init.bin
module  /init.bin
EOF

# Generate CDROm image

genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -input-charset ascii \
	-A kogata-os -o cdrom.iso cdrom
