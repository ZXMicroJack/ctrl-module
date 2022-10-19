targets=testminfat12 testminfat16 testminfat32 teststorage testswap testmisc testtape testdiskamstradcpc testdisksamcoupe testdisk256

default: $(targets)

testminfat32: testminfat.c ../minfat.c common.h ../minfat.h
	gcc -g -DFAT=32 -m32 -o $@ $<

testminfat16: testminfat.c ../minfat.c common.h ../minfat.h
	gcc -g -DFAT=16 -m32 -o $@ $<

testminfat12: testminfat.c ../minfat.c common.h ../minfat.h
	gcc -g -DFAT=12 -m32 -o $@ $<

teststorage: teststorage.c ../storage.c common.h ../minfat.h
	gcc -g -DFAT=32 -m32 -o $@ $<

testtape: testtape.c ../tape.c ../tape.h ./testtape.c
	gcc -g -DFAT=32 -m32 -o $@ $<

testswap: testswap.c ../swap.c ../swap.h common.h
	gcc -g -DFAT=32 -m32 -o $@ $<

testmisc: testmisc.c ../misc.c ../misc.h common.h
	gcc -g -DFAT=32 -m32 -o $@ $<

testdisksamcoupe: testdisk.c ../disk.c ../diskraw.c ../diskecpc.c ../minfat.c ../storage.c ../misc.c ../misc.h common.h
	gcc -g -DBLOCK_SIZE=512 -DFAT=32 -DSAMCOUPE -m32 -o $@ $<

testdiskamstradcpc: testdisk.c ../disk.c ../diskraw.c ../diskecpc.c ../minfat.c ../storage.c ../misc.c ../misc.h common.h
	gcc -g -DBLOCK_SIZE=512 -DFAT=32 -DNR_DISKS=1 -DAMSTRADCPC -m32 -o $@ $<
	
testdisk256: testdisk.c ../disk.c ../diskraw.c ../diskecpc.c ../minfat.c ../storage.c ../misc.c ../misc.h common.h
	gcc -g -DBLOCK_SIZE=256 -DFAT=32 -m32 -o $@ $<

clean: force
	rm $(targets)

.PHONY: force
