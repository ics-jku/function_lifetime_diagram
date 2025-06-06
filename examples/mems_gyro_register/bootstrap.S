.globl _start
.globl main
.globl level_1_interrupt_handler

.equ SYSCALL_ADDR,          0x02010000
.equ PLIC_ENABLED_IRQ_ADDR, 0x40002000

.macro SYS_EXIT, exit_code
li  a7, 93
li  a0, \exit_code
li  t0, SYSCALL_ADDR
csrr a6, mhartid
sw   a6, 0(t0)
.endm

_start:
la t0, level_0_interrupt_handler
csrw mtvec, t0
li t1, 0x888
csrw mie, t1
csrsi mstatus, 8
li t0, PLIC_ENABLED_IRQ_ADDR
li t1, -1
sw t1, 0(t0)
sw t1, 4(t0)
jal main

#call exit (SYS_EXIT=93) with exit code 0 (argument in a0)
SYS_EXIT 0

/*
 * Interrupt handler for non-nested interrupts. Only selected registers are stored/re-stored, i.e. those not preserved on function calls.
 */
#define STORE sw
#define LOAD lw
#define REGBYTES 4

level_0_interrupt_handler:
// store execution context on the stack (register content)
addi sp, sp, -REGBYTES * 32
STORE	x1, 0x0(sp)
STORE	x4, 3 * REGBYTES(sp)
STORE	x5, 4 * REGBYTES(sp)
STORE	x6, 5 * REGBYTES(sp)
STORE	x7, 6 * REGBYTES(sp)
STORE	x10, 9 * REGBYTES(sp)
STORE	x11, 10 * REGBYTES(sp)
STORE	x12, 11 * REGBYTES(sp)
STORE	x13, 12 * REGBYTES(sp)
STORE	x14, 13 * REGBYTES(sp)
STORE	x15, 14 * REGBYTES(sp)
STORE	x16, 15 * REGBYTES(sp)
STORE	x17, 16 * REGBYTES(sp)
STORE	x28, 27 * REGBYTES(sp)
STORE	x29, 28 * REGBYTES(sp)
STORE	x30, 29 * REGBYTES(sp)
STORE	x31, 30 * REGBYTES(sp)

// load interrupt/trap reason and call external C function to handle interfaceStart
csrr t2, mcause
srli t3, t2, 31
bnez t3, handle_irq

li t0, SYSCALL_ADDR
csrr t1, mhartid
csrr a7, mcause
sw t1, 0(t0)
csrr t0, mepc
addi t0, t0, 4
csrw mepc, t0
sw a0, 9 * REGBYTES(sp)

j done_trap
handle_irq:
mv a0, t2
jal level_1_interrupt_handler
done_trap:

// re-store the saved context
LOAD	x1, 0x0(sp)
LOAD	x4, 3 * REGBYTES(sp)
LOAD	x5, 4 * REGBYTES(sp)
LOAD	x6, 5 * REGBYTES(sp)
LOAD	x7, 6 * REGBYTES(sp)
LOAD	x10, 9 * REGBYTES(sp)
LOAD	x11, 10 * REGBYTES(sp)
LOAD	x12, 11 * REGBYTES(sp)
LOAD	x13, 12 * REGBYTES(sp)
LOAD	x14, 13 * REGBYTES(sp)
LOAD	x15, 14 * REGBYTES(sp)
LOAD	x16, 15 * REGBYTES(sp)
LOAD	x17, 16 * REGBYTES(sp)
LOAD	x28, 27 * REGBYTES(sp)
LOAD	x29, 28 * REGBYTES(sp)
LOAD	x30, 29 * REGBYTES(sp)
LOAD	x31, 30 * REGBYTES(sp)
addi	sp, sp, REGBYTES * 32
mret