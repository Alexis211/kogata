#!/bin/sh

TY="$1"

alias cp='cp -v'

echo "Building cdrom type: $TY"

if [ "$TY" != "dev" -a "$TY" != "rel" ]; then
	print "Invalid build type: $TY, expected dev or rel"
	exit -1
fi

if [ "$TY" = "dev" ]; then
	STRIP="strip --strip-debug"
else
	STRIP="strip -s -R .comment -R .gnu.version"
fi

if [ ! -e cdrom/boot/grub/stage2_eltorito ]; then
	mkdir -p cdrom/boot/grub
	echo "Please copy grub's stage2_eltorito to cdrom/boot/grub."
	exit -1
fi

# Copy system files to CDROM

cp build/$TY/kernel.bin cdrom/boot; $STRIP cdrom/boot/kernel.bin
cp build/$TY/sysbin/init.bin cdrom/boot; $STRIP cdrom/boot/init.bin

mkdir -p cdrom/sys/bin
for BIN in giosrv.bin login.bin terminal.bin shell.bin lua.bin luac.bin lx.bin; do
	if [ -e build/$TY/sysbin/$BIN ]; then
		cp build/$TY/sysbin/$BIN cdrom/sys/bin
		$STRIP cdrom/sys/bin/$BIN
	else
		echo "Skipping binary $BIN: not found!"
	fi
done

mkdir -p cdrom/sys/fonts
cp build/fonts/*.bf cdrom/sys/fonts
cp build/fonts/pcvga.bf cdrom/sys/fonts/default.bf

mkdir -p cdrom/sys/keymaps
cp build/keymaps/*.km cdrom/sys/keymaps
cp build/keymaps/fr.km cdrom/sys/keymaps/default.km

mkdir -p cdrom/sys/lua
cp -r src/syslua/* cdrom/sys/lua

mkdir -p cdrom/sys/app
cp -r src/sysapp/* cdrom/sys/app

cp README.md cdrom

# Setup config files

mkdir -p cdrom/config/default

echo "root:/sys" > cdrom/config/default/sysdir

cat > cdrom/boot/grub/menu.lst <<EOF
timeout 10
default 0

title   kogata OS, C terminal
kernel  /boot/kernel.bin root=io:/disk/atapi0 root_opts=l init=root:/boot/init.bin config=default

title   kogata OS, Lua init
kernel  /boot/kernel.bin root=io:/disk/atapi0 root_opts=l init=root:/boot/init.bin config=default lx_init_app=login loop_exec=false
EOF

# Generate CDROM image

genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -input-charset ascii \
	-A kogata-os -o cdrom.$TY.iso cdrom
