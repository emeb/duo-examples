# audio_fulldup

This is a command-line program that opens up a full-duplex audio interface (both
play and record) to loop audio from the ADC to DAC. Various command options may
allow different kinds of processing in the path, including tone generation,
effects, etc.

## Building

Use the normal build process for duo-examples. Note however that this target
requires libasound which is not included in the usual duo-app-sdk that is installed
by the envsetup.sh script. You must first run the envsetup.sh script and then
copy in the includes and libraries for ALSA support. The following commands
should help:

### includes
```
cp -r duo-buildroot-sdk/buildroot-2021.05/output/milkv-duo-lite_musl_riscv64/host/riscv64-buildroot-linux-musl/sysroot/usr/include/alsa duo-sdk/rootfs/usr/include
cp -r duo-buildroot-sdk/buildroot-2021.05/output/milkv-duo-lite_musl_riscv64/host/riscv64-buildroot-linux-musl/sysroot/usr/include/sound duo-sdk/rootfs/usr/include
cp -r duo-buildroot-sdk/buildroot-2021.05/output/milkv-duo-lite_musl_riscv64/host/riscv64-buildroot-linux-musl/sysroot/usr/include/asoundlib.h duo-sdk/rootfs/usr/include
cp -r duo-buildroot-sdk/buildroot-2021.05/output/milkv-duo-lite_musl_riscv64/host/riscv64-buildroot-linux-musl/sysroot/usr/include/sys/asoundlib.h duo-sdk/rootfs/usr/include/sys
```

### libraries
```
cp -r duo-buildroot-sdk/buildroot-2021.05/output/milkv-duo-lite_musl_riscv64/host/riscv64-buildroot-linux-musl/sysroot/usr/lib/libasound.* duo-sdk/rootfs/lib
```
