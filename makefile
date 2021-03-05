GCCPARAMS = -m32 -nostdlib -fno-builtin -fno-exceptions -fno-leading-underscore
ASPARAMS = --32
LDPARAMS = -melf_i386

objects = loader.o kernel.o util.o console.o gdt.o memory.o port.o timer.o keyboard.o \
          idt.o interrupt.o interruptVector.o switch.o process.o thread.o concurrency.o \
		  ide.o fs.o syscall.o


%.o : %.cpp
	gcc $(GCCPARAMS) -c -o $@ $<

%.o : %.c
	gcc $(GCCPARAMS) -c -o $@ $<

%.o : %.s
	as $(ASPARAMS) -o $@ $<

orcas.bin : linker.ld $(objects)
	ld $(LDPARAMS) -T $< -o $@ $(objects)

install : orcas.bin
	sudo cp $< /boot/orcas.bin

orcas.iso : orcas.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp $< iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' > iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> iso/boot/grub/grub.cfg
	echo ' multiboot /boot/orcas.bin' >> iso/boot/grub/grub.cfg
	echo ' boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$@ iso
	rm -rf iso

run: orcas.iso
	(killall VirtualBox && sleep 1) || true
	VirtualBox --startvm "My Operating System" &