CLANG := clang
LINK := lld-link
READOBJ := llvm-readobj
TARGET := build/DellF1Skip.efi
OBJECT := build/f1.obj
CFLAGS := --target=x86_64-pc-win32-coff -std=c17 -O2 -Wall -Wextra -Werror \
	-ffreestanding -fno-builtin -fno-stack-protector -fshort-wchar \
	-mno-red-zone -mno-mmx -mno-sse -I/usr/include/efi -I/usr/include/efi/x86_64
LDFLAGS := /entry:EfiMain /subsystem:efi_boot_service_driver /machine:x64 \
	/nodefaultlib /dll /noimplib /base:0 /dynamicbase /fixed:no /nxcompat \
	/opt:noref /timestamp:0
.PHONY: all clean inspect
all: $(TARGET)
build:
	mkdir -p build
$(OBJECT): f1.c | build
	$(CLANG) $(CFLAGS) -c $< -o $@
$(TARGET): $(OBJECT)
	$(LINK) $(LDFLAGS) /out:$@ $<
inspect: $(TARGET)
	$(READOBJ) --file-headers --sections --coff-imports --coff-basereloc $<
clean:
	rm -f $(OBJECT) $(TARGET)
