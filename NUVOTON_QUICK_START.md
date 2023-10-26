# Getting started with MCUboot for Nuvoton Mbed-Enabled platform

[MCUboot](https://www.mcuboot.com/) is a secure bootloader for 32-bits micro-controllers.
There has been an [MCUboot port for Mbed OS](https://docs.mcuboot.com/readme-mbed.html).
[mbed-mcuboot-demo](https://github.com/AGlass0fMilk/mbed-mcuboot-demo) and
[mbed-mcuboot-blinky](https://github.com/AGlass0fMilk/mbed-mcuboot-blinky)
are created to demo MCUboot with Mbed OS.

Based on above work, this document gets you started with MCUboot for Nuvoton Mbed-Enabled platform.
It walks you through:

-   Constructing one reference MCUboot port for Mbed OS build environment on Windows
-   Demonstrating firmware upgrade process with MCUboot for Nuvoton's platform

## Hardware requirements

-   NuMaker-IoT-M467 board
-   [NuMaker-IoT-M487 board](https://os.mbed.com/platforms/NUMAKER-IOT-M487/)

Currently, Nuvoton-forked `mbed-mcuboot-demo`/`mbed-mcuboot-blinky` repositories support above boards.
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

Below list necessary tools and their versions against which Mbed MCUboot programs can build successfully with this document:

-   [Git](https://gitforwindows.org/) 2.33.0 (Native Windows)

-   [Python3](https://www.python.org/) 3.8.7 (Native Windows)

-   [Mbed CLI 1](https://os.mbed.com/docs/mbed-os/v6.15/build-tools/mbed-cli-1.html) 1.10.5 (Native Windows)

-   [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) 10.3-2021.07 (Native Windows) or
    Arm Compiler 6.12  (Native Windows)

-   [Image tool](https://github.com/mcu-tools/mcuboot/blob/main/docs/imgtool.md) 1.7.2 (Native Windows)

-   [SRecord](http://srecord.sourceforge.net/) 1.63 (Native Windows)

### Configuring proxy for the tools

If your network is behind the firewall, configure proxy for the tools which need to communicate with outside.
If not, just skip this step.

-   Python3 pip: In git-bash, run:
```
$ export http_proxy=http://<USER>:<PASSWORD>@<PROXY-SERVER>:<PORT>/
$ export https_proxy=$http_proxy
```

-   Git: Luckily, Git also honors the environmental variables `http_proxy`/`https_proxy`, which have been set appropriately above.

### Confirming all the tools are ready

This step is to confirm if all tools have been ready.
If you are confident, just skip this step.
In git-bash, check if all tools are present and of right versions:
```
$ which git; git version
/mingw64/bin/git
git version 2.33.0.windows.2

$ which python; python --version; which pip; pip --version
/c/Python38/python
Python 3.8.7
/c/Python38/Scripts/pip
pip 21.3.1 from c:\python38\lib\site-packages\pip (python 3.8)

$ which mbed; mbed --version
/c/Python38/Scripts/mbed
1.10.5

$ which arm-none-eabi-gcc; arm-none-eabi-gcc --version
/C/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.07/bin/arm-none-eabi-gcc
arm-none-eabi-gcc.exe (GNU Arm Embedded Toolchain 10.3-2021.07) 10.3.1 20210621 (release)
Copyright (C) 2020 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

$ which armclang; armclang --version
/C/Keil_v5/ARM/ARMCLANG/bin/armclang
Product: MDK Plus 5.28
Component: ARM Compiler 6.12
Tool: armclang [5d624a00]

$ which imgtool; imgtool version
/c/Python38/Scripts/imgtool
1.7.2

$ which srec_cat; srec_cat --version
/path-to-srecord/srec_cat
srec_cat version 1.63.D001
```

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

First, clone Nuvoton-forked `mbed-mcuboot-demo` repository and switch to the branch `nuvoton_port`:
```
$ git clone https://github.com/OpenNuvoton/mbed-mcuboot-demo
$ cd mbed-mcuboot-demo
$ git checkout -f nuvoton_port
```

Install dependent modules:
```
$ mbed deploy
```

If `imgtool` hasn't installed yet, install it. This finalizes software requirements:
```
$ pip install -r mcuboot/scripts/requirements.txt
```

Build `mbed-mcuboot-demo`:

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ mbed compile -m NUMAKER_IOT_M487_SPIF_TEST -t GCC_ARM
    ```

-   `NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST`

    ```
    $ mbed compile -m NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST -t GCC_ARM
    ```

### Building MCUboot-enabled application

In git-bash, follow the instructions below to build MCUboot-enabled application.

Clone Nuvoton-forked `mbed-mcuboot-blinky` repository and switch to the branch `nuvoton_port`:
```
$ git clone https://github.com/OpenNuvoton/mbed-mcuboot-blinky
$ cd mbed-mcuboot-blinky
$ git checkout -f nuvoton_port
```

Install dependent modules:
```
$ mbed deploy
```

Configure MCUboot image format, which must be consistent with signing through imgtool:

-   MCUboot image has no trailer:

    **mbed_app.json**:
    ```json
    "has-image-trailer"                         : false,
    "has-image-confirmed"                       : false,
    ```

-   MCUboot image has trailer added through imgtool --pad parameter in signing:

    **mbed_app.json**:
    ```json
    "has-image-trailer"                         : true,
    "has-image-confirmed"                       : false,
    ```

-   MCUboot image has confirmed through imgtool --confirm parameter in signing:

    **mbed_app.json**:
    ```json
    "has-image-confirmed"                       : true,
    ```

> **_NOTE:_** imgtool `--confirm` implies imgtool `--pad`.

Build `mbed-mcuboot-blinky`:

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ mbed compile -m NUMAKER_IOT_M487_SPIF_TEST -t GCC_ARM
    ```

-   `NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST`

    ```
    $ mbed compile -m NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST -t GCC_ARM
    ```

Sign `mbed-mcuboot-blinky.bin` to `mbed-mcuboot-blinky_V1.0.0_signed.bin` for first version:

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ imgtool sign \
    -k signing-keys.pem \
    --align 4 \
    -v 1.0.0+0 \
    --header-size 4096 \
    --pad-header \
    -S 0x33000 \
    BUILD/NUMAKER_IOT_M487_SPIF_TEST/GCC_ARM/mbed-mcuboot-blinky.bin \
    BUILD/NUMAKER_IOT_M487_SPIF_TEST/GCC_ARM/mbed-mcuboot-blinky_V1.0.0_signed.bin
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
    BUILD/NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST/GCC_ARM/mbed-mcuboot-blinky.bin \
    BUILD/NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST/GCC_ARM/mbed-mcuboot-blinky_V1.0.0_signed.bin
    ```

> **_NOTE:_** `-k signing-keys.pem` is to specify signing key.

> **_NOTE:_** `--header-size 4096` and `--pad-header` are to specify reserved image header size,
which is (`target.mbed_app_start` - `mcuboot.primary-slot-address`).

> **_NOTE:_** `-S` is to specify primary/secondary slot size `mcuboot.slot-size`.

> **_NOTE:_** On `has-image-trailer` set to `true`, also add `--pad` to meet the configuration. This adds trailer to the MCUboot image so that it is immediately update-able when flashed to secondary slot, without need to invoke `boot_set_pending` at run-time.

> **_NOTE:_** On `has-image-confirmed` set to `true`, also add `--confirm` to meet the configuration. Implying above, this marks the image as confirmed and so image revert is disabled.

Then sign `mbed-mcuboot-blinky.bin` to `mbed-mcuboot-blinky_V1.0.1_signed.bin` for second version:

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ imgtool sign \
    -k signing-keys.pem \
    --align 4 \
    -v 1.0.1+0 \
    --header-size 4096 \
    --pad-header \
    -S 0x33000 \
    BUILD/NUMAKER_IOT_M487_SPIF_TEST/GCC_ARM/mbed-mcuboot-blinky.bin \
    BUILD/NUMAKER_IOT_M487_SPIF_TEST/GCC_ARM/mbed-mcuboot-blinky_V1.0.1_signed.bin
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
    BUILD/NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST/GCC_ARM/mbed-mcuboot-blinky.bin \
    BUILD/NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST/GCC_ARM/mbed-mcuboot-blinky_V1.0.1_signed.bin
    ```

Combine `mbed-mcuboot-demo.bin`, `mbed-mcuboot-blinky_V1.0.0_signed.bin`, and `mbed-mcuboot-blinky_V1.0.1_signed.bin`.

-   `NUMAKER_IOT_M487_SPIF_TEST`

    ```
    $ srec_cat \
    ../mbed-mcuboot-demo/BUILD/NUMAKER_IOT_M487_SPIF_TEST/GCC_ARM/mbed-mcuboot-demo.bin -Binary -offset 0x0 \
    BUILD/NUMAKER_IOT_M487_SPIF_TEST/GCC_ARM/mbed-mcuboot-blinky_V1.0.0_signed.bin -Binary -offset 0x10000 \
    BUILD/NUMAKER_IOT_M487_SPIF_TEST/GCC_ARM/mbed-mcuboot-blinky_V1.0.1_signed.bin -Binary -offset 0x43000 \
    -o BUILD/NUMAKER_IOT_M487_SPIF_TEST/GCC_ARM/mbed-mcuboot-blinky_merged.hex -Intel
    ```

-   `NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST`

    ```
    $ srec_cat \
    ../mbed-mcuboot-demo/BUILD/NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST/GCC_ARM/mbed-mcuboot-demo.bin -Binary -offset 0x0 \
    BUILD/NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST/GCC_ARM/mbed-mcuboot-blinky_V1.0.0_signed.bin -Binary -offset 0x10000 \
    BUILD/NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST/GCC_ARM/mbed-mcuboot-blinky_V1.0.1_signed.bin -Binary -offset 0xB8000 \
    -o BUILD/NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST/GCC_ARM/mbed-mcuboot-blinky_merged.hex -Intel
    ```

> **_NOTE:_** Assume `mbed-mcuboot-demo`/`mbed-mcuboot-blinky` repositories are cloned into the same directory,
so that we can reference `mbed-mcuboot-demo` repository as above.

> **_NOTE:_** `-offset 0x0` is to specify start address of MCUboot bootloader mbed-mcuboot-demo.bin.

> **_NOTE:_** `-offset 0x10000` is to specify start address of primary slot where `mbed-mcuboot-blinky_V1.0.0_signed.bin` resides.

> **_NOTE:_** `-offset 0x43000` or `-offset 0xB8000` is to specify start address of OTA download simulate slot where `mbed-mcuboot-blinky_V1.0.1_signed.bin` resides.

> **_NOTE:_** For pre-flash, it is unnecessary to combine secondary slot. Update firmware will update to it from OTA download simulate slot at run-time.

Finally, drag-n-drop `mbed-mcuboot-blinky_merged.hex` onto target board for flashing.

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

Press `BUTTON1` to copy `mbed-mcuboot-blinky_V1.0.1_signed.bin` from OTA download simulate slot to secondary slot:
```
[INFO][main]: > Press button to copy update image to secondary BlockDevice
[INFO][main]: FlashIAPBlockDevice inited
```

With neither `has-image-trailer` nor `has-image-confirmed` set to `true`, press `BUTTON1` to activate `mbed-mcuboot-blinky_V1.0.1_signed.bin` in secondary slot (`boot_set_pending(false)`):
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

Default signing keys, using RSA, are placed in `mbed-mcuboot-demo` repository and synchronized to `mbed-mcuboot-blinky` repository.
They are exclusively for development and cannot for production.
In the following, we guide how to change the signature algorithm to ECDSA:

1.  In `mbed-mcuboot-demo` repository, generate an ECDSA key pair:
    ```
    $ imgtool keygen -k signing-keys.pem -t ecdsa-p256
    ```

1. Extract the public key into a C data structure:
    ```
    $ imgtool getpub -k signing-keys.pem > signing_keys.c
    ```

1. Synchronize `signing-keys.pem` and `signing_keys.c` to `mbed-mcuboot-blinky` repository.

1. In both `mbed-mcuboot-demo` and `mbed-mcuboot-blinky` repositories, change signature algorithm to ECDSA:
    **mbed_app.json**:
    ```json
    "mcuboot.signature-algorithm": "SIGNATURE_TYPE_EC256",
    ```

1. Rebuild.

### Enable downgrade prevention

According to [MCUboot downgrade prevention](https://docs.mcuboot.com/design.html#downgrade-prevention),
-   **Software-based downgrade prevention** is only available when the overwrite-based image update strategy is used.
-   **Hardware-based downgrade prevention** is available without dependency on particular image update strategy.

In this port, **hardware-based downgrade prevention** is not supported.
In the following, we guide how to enable  **software-based downgrade prevention**:

1. In both `mbed-mcuboot-demo` and `mbed-mcuboot-blinky` repositories, change image update strategy to overwrite-only and enable downgrade prevention:
    **mbed_app.json**:
    ```json
    "mcuboot.overwrite-only": true,
    "mcuboot.downgrade-prevention": true,
    ```

1. Rebuild.

### Support new target

To support new target, follow the procedure:

1.  Following already supported targets, add new target into `mbed-mcuboot-demo/custom_targets.json` and `mbed-mcuboot-blinky/custom_targets.json`.

1.  Following already supported targets, add new target into `mbed-mcuboot-demo/mbed_app.json` and `mbed-mcuboot-blinky/mbed_app.json`.

1.  To change location of secondary slot, modify `get_secondary_bd()` defined in `mbed-mcuboot-demo/default_bd.cpp` and `mbed-mcuboot-blinky/main.cpp`.

1.  For `mcuboot.max-img-sectors`, to avoid MCUboot undocumented limitation,
    1.  When secondary slot is located at external flash, its sector size must be equal to or larger than that of internal flash.
    1.  Set `mcuboot.max-img-sectors` to (`mcuboot.slot-size` / internal flash sector size) or larger.

1.  For firmware rollback, though undocumented,
    it is suggested image trailer (+ TLV) size be large enough to hold (`mcuboot.max-img-sectors` * 12 + 2KiB).
    In most cases, 8KiB or larger will be OK.

    > **_NOTE:_** Image trailer (+ TLV) size is (`mcuboot.primary-slot-address` + `mcuboot.slot-size` - `target.mbed_app_start` - `target.mbed_app_size`).

1.  `mcuboot.scratch-size`, though undocumented, must be large enough to hold image trailer size.
    Following above, 8KiB or larger will be OK.
