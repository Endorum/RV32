holds contracts that concern both the Machine as well as the Software, 
- memory_map.h well holds the memory map...

ROM_START 0x00000000
ROM_SIZE  0x10000    

HD_START 0x10000000
HD_SIZE  0x10000000 

RAM_START 0x20000000
RAM_SIZE  0x10000000 

CLINT_START 0x40000000 
CLINT_SIZE  0x10000     

UART_START  0x50000000
UART_SIZE   0x100       

RAM Map

FIRMWARE:
RAM_FIRM_START = RAM_START
RAM_FIRM_SIZE  = 0x40000 // 256 KiB

FREE:
RAM_FREE_START = RAM_FIRM_START + RAM_FIRM_SIZE
RAM_FREE_SIZE  = 0x10000 // 64 KiB

KERNEL:
RAM_KERN_START = RAM_FREE_START + RAM_FREE_SIZE
RAM_KERN_SIZE  = 0x100000 // 1 MiB

USERSPACE:
RAM_USER_START = RAM_KERN_START + RAM_KERN_SIZE
RAM_USER_SIZE  = (RAM_START + RAM_SIZE) - RAM_USER_START

The Firmware lies at ROM 0x0.
runs from ROM, RAM_FIRM holds data und its stack

The Kernel codes lies at the HD 0x0
and later gets "loaded" into RAM_KERN_START and is called by the firmware/bootloader
The Kernel image is recognized and checked this way:
HD + 0 = Magic number ("RVOS" = 52 56 4F 53)
HD + 4 = Size (32 Bit word)
HD + 8 = start of image -> gets copied to RAM_KERN_START

Kernel entry is at the first word of the image.
Kernel can be started by executing the first instruction at RAM_KERN_START

The Code of the programmes lie on the HD and are loaded into RAM from the Kernel





Boot Sequenz:

Reset -> CPU jumps to 0x0 (reset vec)  ->   Firmware (Shell)                -> Kernel
         does some minimal prep loads       minimal shell to check memory       
         and starts the firmware            etc. has the cmd "boot", 
                                            finds and loads kernel img
                                            from the HD to RAM. Starts kernel

