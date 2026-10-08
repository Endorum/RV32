#define csr_read(reg)({ unsigned long __v; asm volatile("csrr %0, " #reg : "=r"(__v)); __v; })
#define csr_write(reg, val) asm volatile("csrw " #reg ", %0" :: "rK"(val))
