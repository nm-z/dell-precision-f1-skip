# Dell Precision A34 F1 skip

This x86-64 UEFI driver automatically supplies F1 to the Dell Precision T5810
and T7810 A34 firmware error screen. It needs no keyboard, operating-system
service, network connection, AMT configuration, or BIOS modification.

**Status:** Live-tested on a T5810 A34 through unattended warm and S5 cold
boots. The T7810 A34 error-handler binary is identical, but has not been tested
on physical T7810 hardware.

## Compatibility

- Dell Precision Tower 5810 or 7810
- BIOS A34 only
- UEFI boot
- Secure Boot disabled, or a binary signed by a key in the firmware trust store

Do not use this driver with another model or BIOS version. Remove it before a
BIOS update. It acknowledges every soft halt routed through the matching Dell
A34 event, not only fan and front-I/O warnings.

## How it works

Dell A34 queries a private 16-byte input protocol before reading a physical
keyboard. The firmware contains a consumer but no provider. This driver
installs that provider for protocol `292DF98D-748C-419E-8B6E-FECEBA3FCF63`.

Dell signals event group `1A4D8E9F-BF05-454B-8E01-B4AF9B86C086` immediately
before its error callback. The driver enables its provider at TPL 16 and queues
cleanup at TPL 8. During Dell's callback, `Query(1)` returns repeating
`TRUE, FALSE` pairs. Dell discards the first F1 read, accepts the next one, and
continues booting. Cleanup then disables the provider.

## Build

Install Clang, LLD, LLVM tools, GNU Make, and GNU-EFI headers. The validated
build used Clang/LLD 22.1.8 and GNU-EFI 4.0.4.

```sh
make clean
make
make inspect
sha256sum build/DellF1Bypass.efi
```

The expected SHA-256 is:

```text
c68fea0c4f79d36636e788860610e8f4766a6ac800e7154042479faa789a0afe
```

## Install

These commands write a persistent UEFI driver entry. Replace `/dev/sda`, `1`,
and `/boot/efi` with the target's boot disk, ESP partition number, and mounted
EFI System Partition.

```sh
sudo install -D -m 0644 DellF1Bypass.efi \
  /boot/efi/EFI/DellF1Bypass/DellF1Bypass.efi
sudo efibootmgr --create --driver --full-dev-path --index 0 \
  --disk /dev/sda --part 1 --label 'Dell F1 skip' \
  --loader '\EFI\DellF1Bypass\DellF1Bypass.efi'
sudo efibootmgr --driver --verbose
```

Confirm that the new number is first in `DriverOrder`, is active, and starts
with the platform's complete hardware path. An abbreviated
`HD(...)/File(...)` entry did not load on the tested T5810.

Reboot through the real warning screen without pressing a key. Reaching the
configured boot target is the only compatibility test that matters.

## Remove

Use `efibootmgr --driver` to obtain the four-digit entry number:

```sh
sudo efibootmgr --driver --bootnum NNNN --delete-bootnum
sudo rm /boot/efi/EFI/DellF1Bypass/DellF1Bypass.efi
sudo rmdir /boot/efi/EFI/DellF1Bypass 2>/dev/null || true
```

## Size and contents

- `f1.c`: 82 raw lines
- `Makefile`: 23 raw lines
- Total code and build definition: 105 raw lines
- `DellF1Bypass.efi`: 2,048 bytes
- No imports, firmware blobs, console hooks, private state variables, or network dependency

This project is not affiliated with or supported by Dell Technologies.
