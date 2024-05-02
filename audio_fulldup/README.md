# audio_fulldup

This is a command-line program that opens up a full-duplex audio interface (both
play and record) to loop audio from the ADC to DAC. Various command options may
allow different kinds of processing in the path, including tone generation,
effects, etc.

## Prerequisites

This requires the kernel to be rebuilt enabling the CV1800b internal codec for
both input and output, and buildroot needs to be reconfigured to enable ALSA
runtime tools, including libasound. The default device names are assigned with
the assumption that the MAX98357a I2S output driver is available although it
is not used by default.

## Building

Use the normal build process for duo-examples. Note however that this target
requires libasound which is not included in the usual duo-app-sdk that is installed
by the envsetup.sh script. You must first run the envsetup.sh script and then
copy in the includes and libraries for ALSA support from the full buildroot SDK
(assuming you've already configured and built ALSA for your target device with
the SDK). The following commands should help:

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
