#!/bin/sh

if [ ! -e cdrom/boot/grub/stage2_eltorito ]; then
	mkdir -p cdrom/boot/grub
	echo "Please copy grub's stage2_eltorito to cdrom/boot/grub."
	exit -1
fi

# Copy system files to CDROM

cp src/kernel/kernel.bin cdrom/boot; strip cdrom/boot/kernel.bin
cp src/kernel/kernel.map cdrom/boot
cp src/sysbin/init/init.bin cdrom/boot; strip cdrom/boot/init.bin

mkdir -p cdrom/sys/bin
cp src/sysbin/giosrv/giosrv.bin cdrom/sys/bin
cp src/sysbin/login/login.bin cdrom/sys/bin

for BIN in cdrom/sys/bin/*.bin; do strip $BIN; done

mkdir -p cdrom/sys/fonts
cp res/fonts/*.bf cdrom/sys/fonts

cp README.md cdrom

# Setup config files

mkdir -p cdrom/config/default

echo "root:/sys" > cdrom/config/default/sysdir

cat > cdrom/boot/grub/menu.lst <<EOF
timeout 10
default 0

title   kogata OS
kernel  /boot/kernel.bin root=io:/disk/atapi0 root_opts=l init=root:/boot/init.bin config=default
module  /boot/kernel.map
EOF

# Generate CDROm image

genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -input-charset ascii \
	-A kogata-os -o cdrom.iso cdrom
