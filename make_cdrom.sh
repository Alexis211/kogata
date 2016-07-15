#!/bin/sh

if [ ! -e cdrom/boot/grub/stage2_eltorito ]; then
	mkdir -p cdrom/boot/grub
	echo "Please copy grub's stage2_eltorito to cdrom/boot/grub."
	exit -1
fi

# Copy system files to CDROM

cp build/kernel.bin cdrom/boot; strip cdrom/boot/kernel.bin
cp build/kernel.map cdrom/boot
cp build/sysbin/init.bin cdrom/boot; strip cdrom/boot/init.bin

mkdir -p cdrom/sys/bin
for BIN in giosrv.bin login.bin terminal.bin shell.bin; do
	cp build/sysbin/$BIN cdrom/sys/bin
	strip cdrom/sys/bin/$BIN
done

mkdir -p cdrom/bin
for BIN in lua.bin luac.bin; do
	cp build/bin/$BIN cdrom/bin
	strip cdrom/bin/$BIN
done

mkdir -p cdrom/sys/fonts
cp build/fonts/*.bf cdrom/sys/fonts
cp build/fonts/pcvga.bf cdrom/sys/fonts/default.bf

mkdir -p cdrom/sys/keymaps
cp build/keymaps/*.km cdrom/sys/keymaps
cp build/keymaps/fr.km cdrom/sys/keymaps/default.km

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

# Generate CDROM image

genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -input-charset ascii \
	-A kogata-os -o cdrom.iso cdrom
