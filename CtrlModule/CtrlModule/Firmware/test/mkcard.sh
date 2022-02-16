#!/bin/bash
if [ ! -e ./tmp/card.32 ]; then
	mkdir tmp &> /dev/null
	mount -t tmpfs tmpfs ./tmp
fi

mkcard()
{
dd if=/dev/zero of=./tmp/card.$1 bs=1024 count=65536
#dd if=/dev/zero of=./tmp/card.$1 bs=1024 count=98304

fdisk ./tmp/card.$1 << EOF
n
p
1


t
$2
w
EOF

losetup -P /dev/loop55 ./tmp/card.$1
mkfs.vfat -F $1 $3 /dev/loop55p1

mkdir t
mount /dev/loop55p1 t

dd if=/dev/zero of=./t/bigfile bs=1024 count=32768
cp ../../../../../acorn-electron/roms/* t

mkdir t/zx
cp -v ~/mygames/*.tap ./t/zx
cp -v ~/mygames/*.tzx ./t/zx

mkdir t/zx/roms
cp ../../../../../acorn-electron/roms/* t/zx/roms

mkdir t/zx/disks
cp ~/transfer/specdisks/sdcld*.opd t/zx/disks

mkdir t/empty
umount t

chmod 666 ./tmp/card.$1
losetup -d /dev/loop55
rmdir t
}

mkcard 32 0c "-s 2"
mkcard 16 06
#mkcard 12 01

mkdir t
