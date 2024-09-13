# Getting started with MCUboot for Nuvoton Mbed CE enabled platform

[MCUboot](https://www.mcuboot.com/) is a secure bootloader for 32-bits micro-controllers.
There has been an [MCUboot port for Mbed OS](https://docs.mcuboot.com/readme-mbed.html).
[mbed-mcuboot-demo](https://github.com/AGlass0fMilk/mbed-mcuboot-demo) and
[mbed-mcuboot-blinky](https://github.com/AGlass0fMilk/mbed-mcuboot-blinky)
are created to demo MCUboot with Mbed OS.

Based on above work, this document gets you started with MCUboot for Nuvoton Mbed-Enabled platform.
It walks you through:

-   Constructing one reference MCUboot port for [Mbed CE](https://github.com/mbed-ce) build environment on Windows
-   Demonstrating firmware upgrade process with MCUboot for Nuvoton's platform

## Hardware requirements

-   [NuMaker-IoT-M467 board](https://www.nuvoton.com/board/numaker-iot-m467/)
-   [NuMaker-IoT-M487 board](https://www.nuvoton.com/products/iot-solution/iot-platform/numaker-iot-m487/)

Currently, Nuvoton-forked `mbed-ce-mcuboot-demo`/`mbed-ce-mcuboot-blinky` repositories support above boards.
Choose one of them for demo in the following.

### Support targets

-   NUMAKER_IOT_M467_FLASHIAP
-   NUMAKER_IOT_M467_FLASHIAP_TEST
-   NUMAKER_IOT_M467_FLASHIAP_DUALBANK
-   NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST
-   NUMAKER_IOT_M467_SPIF
-   NUMAKER_IOT_M467_SPIF_TEST
-   NUMAKER_IOT_M467_NUSD
-   NUMAKER_IOT_M467_NUSD_TEST
-   NUMAKER_IOT_M487_SPIF
-   NUMAKER_IOT_M487_SPIF_TEST
-   NUMAKER_IOT_M487_NUSD
-   NUMAKER_IOT_M487_NUSD_TEST

> **_NOTE:_** **DO NOT** use standard targets like NUMAKER_IOT_M467/NUMAKER_IOT_M487.
Only use targets in the support list.

> **_NOTE:_** Targets whose names have `TEST` are for test.
Their internal flash maps are designed to support firmware upgrade in-place, without real OTA download process.

> **_NOTE:_** Targets whose names have `FLASHIAP` locate secondary slot at internal flash, usually (immediately) in back of primary slot.

> **_NOTE:_** Targets whose names have `FLASHIAP_DUALBANK` locate secondary slot at second bank of internal flash. This is to enable **RWW** (**Read While Write**), that is, code on first bank can still be running (read operation) while update image is being written to second bank (write operation).

> **_NOTE:_** Targets whose names have `SPIF` locate secondary slot at external SPI flash.

> **_NOTE:_** Targets whose names have `NUSD` locate secondary slot at SD card with Nuvoton's SDH peripheral. So prepare one micro SD card for these targets.

## Software requirements

-   [Git](https://gitforwindows.org/)

-   [Python3](https://www.python.org/)

-   [CMake](https://cmake.org/)

-   [Ninja](https://github.com/ninja-build/ninja/releases)

-   [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

-   [Image tool](https://github.com/mcu-tools/mcuboot/blob/main/docs/imgtool.md)

-   [SRecord](http://srecord.sourceforge.net/)

For [VS Code development](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-VS-Code)
or [OpenOCD as upload method](https://github.com/mbed-ce/mbed-os/wiki/Upload-Methods#openocd),
install below additionally:

-   [NuEclipse](https://github.com/OpenNuvoton/Nuvoton_Tools#numicro-software-development-tools): Nuvoton's fork of Eclipse

-   Nuvoton forked OpenOCD: Shipped with NuEclipse installation package above.
    Checking openocd version `openocd --version`, it should fix to `0.10.022`.

## Walking through internal flash map

The following internal flash map is common for all targets.

-   MCUboot region: Area where MCUboot bootloader is located

-   Primary slot: Area where active firmware is located, consisting of image header, application, and image trailer (+ TLV)

    > **_NOTE:_** Refer to [image format](https://docs.mcuboot.com/design.html#image-format) for details.

-   KVStore region: Area reserved for [KVStore](https://os.mbed.com/docs/mbed-os/v6.15/apis/kvstore.html), Mbed OS's storage solution

-   Scratch region: Area reserved for MCUboot to execute firmware upgrade operation

```
    +--------------------------+ <-+ mcuboot.scratch-address + mcuboot.scratch-size
    | Scratch                  |
    +--------------------------+ <-+ mcuboot.scratch-address
    +--------------------------+ <-+ storage_tdb_internal.internal_base_address +
    | KVStore                  |     storage_tdb_internal.internal_size
    +--------------------------+ <-+ storage_tdb_internal.internal_base_address
    +--------------------------+
    | Optional OTA download    |
    | simulate slot            |
    +--------------------------+
    +--------------------------+
    | Optional secondary slot  |
    +--------------------------+
    +--------------------------+ <-+ mcuboot.primary-slot-address + mcuboot.slot-size
    | Primary slot             |
    |+------------------------+|
    || Image trailer (+ TLV)  ||
    |+------------------------+|
    ++------------------------+| <-+ target.mbed_app_start + target.mbed_app_size
    |}                        ||
    || Application            ||
    ||                        ||
    ++------------------------++ <-+ target.mbed_app_start
    |+------------------------+|
    || Image header           ||
    |+------------------------+|
    +--------------------------+ <-+ mcuboot.primary-slot-address
    +--------------------------+ <-+ 0 + target.restrict_size
    |                          |
    | MCUboot                  |
    |                          |
    +--------------------------+ <-+ 0
```

Immediately following primary slot are optional secondary slot and then optional OTA download simulate slot.

-   Secondary slot: Area where update firmware is located, consisting of image header, application, and image trailer (+ TLV).
    For target names without `FLASHIAP`, secondary slot is located at external storage.
    
-   OTA download simulate slot: Area reserved to support firmware upgrade in-place.
    This is achieved by pre-flashing update firmware in this slot, without real OTA download process.
    For target names without `TEST`, OTA download simulate slot is unnecessary.

> **_NOTE:_** Sizes and layouts of primary slot, secondary slot, and OTA download simulate slot are the same.

```
    +--------------------------+
    | OTA download simulate    |
    | slot        |            |
    |+------------------------+|
    || Image trailer (+ TLV)  ||
    |+------------------------+|
    ++------------------------+|
    |}                        ||
    || Application            ||
    ||                        ||
    ++------------------------++
    |+------------------------+|
    || Image header           ||
    |+------------------------+|
    +--------------------------+
    +--------------------------+
    | Secondary slot           |
    |+------------------------+|
    || Image trailer (+ TLV)  ||
    |+------------------------+|
    ++------------------------+|
    |}                        ||
    || Application            ||
    ||                        ||
    ++------------------------++
    |+------------------------+|
    || Image header           ||
    |+------------------------+|
    +--------------------------+
    +--------------------------+ <-+ mcuboot.primary-slot-address + mcuboot.slot-size
    | Primary slot             |
```

## Demonstrating firmware upgrade process with MCUboot

In the chapter, we choose the following targets for demonstrating firmware upgrade process with MCUboot.

-   `NUMAKER_IOT_M487_SPIF_TEST`

    locates secondary slot at external SPI flash and adjusts primary/secondary slot size to support firmware upgrade in-place.

-   `NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST`

    Locates secondary slot at second bank of internal flash and adjusts primary/secondary slot size to support firmware upgrade in-place.

### Building MCUboot bootloader

In git-bash, follow the instructions below to build MCUboot bootloader.

First, clone Nuvoton-forked `mbed-ce-mcuboot-demo` repository and switch to the branch `master`:
```
$ git clone https://github.com/mbed-nuvoton/mbed-ce-mcuboot-demo
$ cd mbed-ce-mcuboot-demo
$ git checkout -f master
```

Install dependent modules:
```
$ git submodule update --init
```
Or for fast install:
```
$ git submodule update --init --filter=blob:none
```

If `imgtool` hasn't installed yet, install it. This finalizes software requirements:
```
$ pip install -r mcuboot/scripts/requirements.txt
```

Build `mbed-ce-mcuboot-demo`:

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ mkdir build; cd build
    $ cmake .. -GNinja -DCMAKE_BUILD_TYPE=Develop -DMBED_TARGET=NUMAKER_IOT_M487_SPIF_TEST
    $ ninja
    $ cd ..
    ```

-   `NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST`

    ```
    $ mkdir build; cd build
    $ cmake .. -GNinja -DCMAKE_BUILD_TYPE=Develop -DMBED_TARGET=NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST
    $ ninja
    $ cd ..
    ```

### Building MCUboot-enabled application

In git-bash, follow the instructions below to build MCUboot-enabled application.

Clone Nuvoton-forked `mbed-ce-mcuboot-blinky` repository and switch to the branch `master`:
```
$ git clone https://github.com/mbed-nuvoton/mbed-ce-mcuboot-blinky
$ cd mbed-ce-mcuboot-blinky
$ git checkout -f master
```

Install dependent modules:
```
$ git submodule update --init
```
Or for fast install:
```
$ git submodule update --init --filter=blob:none
```

Configure MCUboot image format, which must be consistent with signing through imgtool:

-   MCUboot image has no trailer:

    **mbed_app.json5**:
    ```json5
    "has-image-trailer"                         : false,
    "has-image-confirmed"                       : false,
    ```

-   MCUboot image has trailer added through imgtool --pad parameter in signing:

    **mbed_app.json5**:
    ```json5
    "has-image-trailer"                         : true,
    "has-image-confirmed"                       : false,
    ```

-   MCUboot image has confirmed through imgtool --confirm parameter in signing:

    **mbed_app.json5**:
    ```json5
    "has-image-confirmed"                       : true,
    ```

> **_NOTE:_** imgtool `--confirm` implies imgtool `--pad`.

Build `mbed-ce-mcuboot-blinky`:

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ mkdir build; cd build
    $ cmake .. -GNinja -DCMAKE_BUILD_TYPE=Develop -DMBED_TARGET=NUMAKER_IOT_M487_SPIF_TEST
    $ ninja
    $ cd ..
    ```

-   `NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST`

    ```
    $ mkdir build; cd build
    $ cmake .. -GNinja -DCMAKE_BUILD_TYPE=Develop -DMBED_TARGET=NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST
    $ ninja
    $ cd ..
    ```

Sign `mbed-ce-mcuboot-blinky.bin` to `mbed-ce-mcuboot-blinky_V1.0.0_signed.bin` for first version:

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ imgtool sign \
    -k signing-keys.pem \
    --align 4 \
    -v 1.0.0+0 \
    --header-size 4096 \
    --pad-header \
    -S 0x33000 \
    build/mbed-ce-mcuboot-blinky.bin \
    build/mbed-ce-mcuboot-blinky_V1.0.0_signed.bin
    ```

-   `NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST`

    ```
    $ imgtool sign \
    -k signing-keys.pem \
    --align 4 \
    -v 1.0.0+0 \
    --header-size 4096 \
    --pad-header \
    --confirm \
    -S 0x38000 \
    build/mbed-ce-mcuboot-blinky.bin \
    build/mbed-ce-mcuboot-blinky_V1.0.0_signed.bin
    ```

> **_NOTE:_** `-k signing-keys.pem` is to specify signing key.

> **_NOTE:_** `--header-size 4096` and `--pad-header` are to specify reserved image header size,
which is (`target.mbed_app_start` - `mcuboot.primary-slot-address`).

> **_NOTE:_** `-S` is to specify primary/secondary slot size `mcuboot.slot-size`.

> **_NOTE:_** On `has-image-trailer` set to `true`, also add `--pad` to meet the configuration. This adds trailer to the MCUboot image so that it is immediately update-able when flashed to secondary slot, without need to invoke `boot_set_pending` at run-time.

> **_NOTE:_** On `has-image-confirmed` set to `true`, also add `--confirm` to meet the configuration. Implying above, this marks the image as confirmed and so image revert is disabled.

Then sign `mbed-ce-mcuboot-blinky.bin` to `mbed-ce-mcuboot-blinky_V1.0.1_signed.bin` for second version:

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ imgtool sign \
    -k signing-keys.pem \
    --align 4 \
    -v 1.0.1+0 \
    --header-size 4096 \
    --pad-header \
    -S 0x33000 \
    build/mbed-ce-mcuboot-blinky.bin \
    build/mbed-ce-mcuboot-blinky_V1.0.1_signed.bin
    ```

-   `NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST`

    ```
    $ imgtool sign \
    -k signing-keys.pem \
    --align 4 \
    -v 1.0.1+0 \
    --header-size 4096 \
    --pad-header \
    --confirm \
    -S 0x38000 \
    build/mbed-ce-mcuboot-blinky.bin \
    build/mbed-ce-mcuboot-blinky_V1.0.1_signed.bin
    ```

Combine `mbed-ce-mcuboot-demo.bin`, `mbed-ce-mcuboot-blinky_V1.0.0_signed.bin`, and `mbed-ce-mcuboot-blinky_V1.0.1_signed.bin`.

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ srec_cat \
    ../mbed-ce-mcuboot-demo/build/mbed-ce-mcuboot-demo.bin -Binary -offset 0x0 \
    build/mbed-ce-mcuboot-blinky_V1.0.0_signed.bin -Binary -offset 0x10000 \
    build/mbed-ce-mcuboot-blinky_V1.0.1_signed.bin -Binary -offset 0x43000 \
    -o build/mbed-ce-mcuboot-blinky_merged.hex -Intel
    ```

-   `NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST`

    ```
    $ srec_cat \
    ../mbed-ce-mcuboot-demo/build/mbed-ce-mcuboot-demo.bin -Binary -offset 0x0 \
    build/mbed-ce-mcuboot-blinky_V1.0.0_signed.bin -Binary -offset 0x10000 \
    build/mbed-ce-mcuboot-blinky_V1.0.1_signed.bin -Binary -offset 0xB8000 \
    -o build/mbed-ce-mcuboot-blinky_merged.hex -Intel
    ```

> **_NOTE:_** Assume `mbed-ce-mcuboot-demo`/`mbed-ce-mcuboot-blinky` repositories are cloned into the same directory,
so that we can reference `mbed-ce-mcuboot-demo` repository as above.

> **_NOTE:_** `-offset 0x0` is to specify start address of MCUboot bootloader mbed-ce-mcuboot-demo.bin.

> **_NOTE:_** `-offset 0x10000` is to specify start address of primary slot where `mbed-ce-mcuboot-blinky_V1.0.0_signed.bin` resides.

> **_NOTE:_** `-offset 0x43000` or `-offset 0xB8000` is to specify start address of OTA download simulate slot where `mbed-ce-mcuboot-blinky_V1.0.1_signed.bin` resides.

> **_NOTE:_** For pre-flash, it is unnecessary to combine secondary slot. Update firmware will update to it from OTA download simulate slot at run-time.

Finally, drag-n-drop `mbed-ce-mcuboot-blinky_merged.hex` onto target board for flashing.

### Monitoring firmware upgrade process through host console

Configure host terminal program with **115200/8-N-1**.

At first boot, notice:

1.  There is no firmware upgrade (`Swap type: none`).
1.  Firmware version is `V1.0.0` (`Hello version 1.0.0+0`).

```
[INFO][BL]: Starting MCUboot
[INFO][MCUb]: Primary image: ......
[INFO][MCUb]: Scratch: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
[INFO][MCUb]: Boot source: primary slot
[INFO][MCUb]: Swap type: none
[INFO][BL]: Booting firmware image at 0x11000
```

```
[INFO][main]: Hello version 1.0.0+0
[INFO][main]: > Press button to erase secondary slot
```

Press `BUTTON1` to erase secondary slot:
```
[INFO][main]: Secondary BlockDevice inited
[INFO][main]: Erasing secondary BlockDevice...
[INFO][main]: Secondary BlockDevice erased
```

Press `BUTTON1` to copy `mbed-ce-mcuboot-blinky_V1.0.1_signed.bin` from OTA download simulate slot to secondary slot:
```
[INFO][main]: > Press button to copy update image to secondary BlockDevice
[INFO][main]: FlashIAPBlockDevice inited
```

With neither `has-image-trailer` nor `has-image-confirmed` set to `true`, press `BUTTON1` to activate `mbed-ce-mcuboot-blinky_V1.0.1_signed.bin` in secondary slot (`boot_set_pending(false)`):
```
[INFO][main]: > Image copied to secondary BlockDevice, press button to activate
[INFO][main]: > Secondary image pending, reboot to update
```

Otherwise, go reboot:
```
[INFO][main]: > Image copied to secondary BlockDevice, reboot to update
```

Press `RESET` to reboot...

At second boot, notice:

1.  There is firmware upgrade for test-run (`Swap type: test`) or permanently (`Swap type: perm`).
1.  Firmware version updates to `V1.0.1` (`Hello version 1.0.1+0`).
1.  With `has-image-confirmed` not set to `true`, image is confirmed `boot_set_confirmed()`, so there won't be firmware upgrade rollback at next boot.
```
[INFO][BL]: Starting MCUboot
[INFO][MCUb]: Primary image: ......
[INFO][MCUb]: Scratch: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
[INFO][MCUb]: Boot source: primary slot
[INFO][MCUb]: Swap type: test or perm
[INFO][MCUb]: Starting swap using scratch algorithm.
[INFO][BL]: Booting firmware image at 0x11000
```

With `has-image-confirmed` not set to `true`, image is confirmed:
```
[INFO][main]: Boot confirmed
```

```
[INFO][main]: Hello version 1.0.1+0
[INFO][main]: > Press button to erase secondary slot
```

We don't re-play firmware upgrade process. Press `RESET` to reboot...

At third boot, notice:

1.  There is no firmware upgrade rollback (`Swap type: none`) due to confirmed at the very start (`has-image-confirmed` set to `true`) or at previous boot (`boot_set_confirmed()`).
1.  Firmware version keeps as updated `V1.0.1` (`Hello version 1.0.1+0`).
```
[INFO][BL]: Starting MCUboot
[INFO][MCUb]: Primary image: ......
[INFO][MCUb]: Scratch: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
[INFO][MCUb]: Boot source: none
[INFO][MCUb]: Swap type: none
[INFO][BL]: Booting firmware image at 0x11000

[INFO][main]: Hello version 1.0.1+0
[INFO][main]: > Press button to erase secondary slot
```

Firmware has upgraded from `V1.0.0` to `V1.0.1` permanently.

## Further reading

In the section, advanced topics are addressed here.

### Changing signing keys

Default signing keys, using RSA, are placed in `mbed-ce-mcuboot-demo` repository and synchronized to `mbed-ce-mcuboot-blinky` repository.
They are exclusively for development and cannot for production.
In the following, we guide how to change the signature algorithm to ECDSA:

1.  In `mbed-ce-mcuboot-demo` repository, generate an ECDSA key pair:
    ```
    $ imgtool keygen -k signing-keys.pem -t ecdsa-p256
    ```

1. Extract the public key into a C data structure:
    ```
    $ imgtool getpub -k signing-keys.pem > signing_keys.c
    ```

1. Synchronize `signing-keys.pem` and `signing_keys.c` to `mbed-ce-mcuboot-blinky` repository.

1. In both `mbed-ce-mcuboot-demo` and `mbed-ce-mcuboot-blinky` repositories, change signature algorithm to ECDSA:
    **mbed_app.json5**:
    ```json5
    "mcuboot.signature-algorithm": "SIGNATURE_TYPE_EC256",
    ```

1. Rebuild.

### Enable downgrade prevention

According to [MCUboot downgrade prevention](https://docs.mcuboot.com/design.html#downgrade-prevention),
-   **Software-based downgrade prevention** is only available when the overwrite-based image update strategy is used.
-   **Hardware-based downgrade prevention** is available without dependency on particular image update strategy.

In this port, **hardware-based downgrade prevention** is not supported.
In the following, we guide how to enable  **software-based downgrade prevention**:

1. In both `mbed-ce-mcuboot-demo` and `mbed-ce-mcuboot-blinky` repositories, change image update strategy to overwrite-only and enable downgrade prevention:
    **mbed_app.json5**:
    ```json5
    "mcuboot.overwrite-only": true,
    "mcuboot.downgrade-prevention": true,
    ```

1. Rebuild.

### Support new target

To support new target, follow the procedure:

1.  Following already supported targets, add new target into `mbed-ce-mcuboot-demo/custom_targets` and `mbed-ce-mcuboot-blinky/custom_targets`.

1.  Following already supported targets, add new target into `mbed-ce-mcuboot-demo/mbed_app.json` and `mbed-ce-mcuboot-blinky/mbed_app.json`.

1.  To change location of secondary slot, modify `get_secondary_bd()` defined in `mbed-ce-mcuboot-demo/default_bd.cpp` and `mbed-ce-mcuboot-blinky/main.cpp`.

1.  For `mcuboot.max-img-sectors`, to avoid MCUboot undocumented limitation,
    1.  When secondary slot is located at external flash, its sector size must be equal to or larger than that of internal flash.
    1.  Set `mcuboot.max-img-sectors` to (`mcuboot.slot-size` / internal flash sector size) or larger.

1.  For firmware rollback, though undocumented,
    it is suggested image trailer (+ TLV) size be large enough to hold (`mcuboot.max-img-sectors` * 12 + 2KiB).
    In most cases, 8KiB or larger will be OK.

    > **_NOTE:_** Image trailer (+ TLV) size is (`mcuboot.primary-slot-address` + `mcuboot.slot-size` - `target.mbed_app_start` - `target.mbed_app_size`).

1.  `mcuboot.scratch-size`, though undocumented, must be large enough to hold image trailer size.
    Following above, 8KiB or larger will be OK.
