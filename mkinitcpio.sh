cd ${PROJECT_ROOT}/initramfs-${ARCH}

# Make initramfs.img
find . -print | cpio -o -H newc >${PROJECT_ROOT}/initramfs-${ARCH}.img

cd ${PROJECT_ROOT}/
