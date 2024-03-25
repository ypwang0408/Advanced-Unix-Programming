clear
cd ~/Lab2/maze
make
cd ~/Lab2/tmp
cp -r ../maze .
rm -f rootfs.cpio.bz2
find ./* | cpio -H newc -o > rootfs.cpio 
bzip2 -z rootfs.cpio 
mv rootfs.cpio.bz2 ../dist/. 
cd ..