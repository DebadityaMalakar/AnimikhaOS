bits 16
section .text.startup
extern __bss_start
extern __bss_sizel
extern kmain

global memory_map
global entries_counter
global start

PROTMODE_STACK  EQU 0x90000    ; Protected mode stack

start:
    ; Note: we are not in 32-bit protected mode at this point
    ; We are still in 16-bit real mode. Eventually when you
    ; call into 32-bit C code you will have to enter 32-bit
    ; protected mode first.
    cli                        ; Clear interrupts
    cld                        ; Clear direction flag
    xor ax, ax                 ; Zero out AX
    mov ds, ax                 ; Set DS to 0x0000
    mov es, ax                 ; Set ES to 0x0000
                               ; Real mode stack set in bootloader

    ; Check to see if we have a 386 CPU. Fail with an error if we don't
    ; Otherwise enable toe A20 line.
    mov si, not386_err         ; Default error message to no 386 processor
    call check_386             ; Is this a 32-bit processor?
    jz .error                  ; If not print error and stop
    mov si, noa20_err          ; Default error message to A20 enable error
    call a20_enable            ; Enable A20 line
    jz .error                  ; If the A20 line isn't enabled then print error and stop

    ; Zero BSS section
    mov ecx, __bss_sizel       ; ECX=# of longwords (DWORDS) to zero in BSS
    xor eax, eax               ; EAX=0 (value to store)
    mov edi, __bss_start       ; EDI=Start of BSS section
    rep stosd                  ; Zero BSS

    ; Call the memory detection function while in real mode
    call detect_memory

    jmp switch_protmode_32     ; Switch to 32-bit protected mode and
                               ;     and continue at label 'protmode32_entry'
.error:
    call print_string_16       ; Print error message
.end:
    cli                        ; Disable interrupts
.endloop:
    hlt                        ; Halt CPU
    jmp .endloop               ; Loop in case we get an NMI (non-maskable interrupt)


; Function: print_string_16
;           Display a string to the console on display page 0
;
; Inputs:   SI = Offset of address to print
; Clobbers: AX, BX, SI

print_string_16:
    mov ah, 0x0e               ; BIOS tty Print
    xor bx, bx                 ; Set display page to 0 (BL)
    jmp .getch
.repeat:
    int 0x10                   ; print character
.getch:
    lodsb                      ; Get character from string
    test al,al                 ; Have we reached end of string?
    jnz .repeat                ;     if not process next character
.end:
    ret

; Function: wait_8042_cmd
;           Wait until the Input Buffer Full bit in the keyboard controller's
;           status register becomes 0. After calls to this function it is
;           safe to send a command on Port 0x64
;
; Inputs:   None
; Clobbers: AX
; Returns:  None

KBC_STATUS_IBF_BIT EQU 1
wait_8042_cmd:
    in al, 0x64                ; Read keyboard controller status register
    test al, 1 << KBC_STATUS_IBF_BIT
                               ; Is bit 1 (Input Buffer Full) set?
    jnz wait_8042_cmd          ;     If it is then controller is busy and we
                               ;     can't send command byte, try again
    ret                        ; Otherwise buffer is clear and ready to send a command

; Function: wait_8042_data
;           Wait until the Output Buffer Empty (OBE) bit in the keyboard controller's
;           status register becomes 0. After a call to this function there is
;           data available to be read on port 0x60.
;
; Inputs:   None
; Clobbers: AX
; Returns:  None

KBC_STATUS_OBE_BIT EQU 0
wait_8042_data:
    in al, 0x64                ; Read keyboard controller status register
    test al, 1 << KBC_STATUS_OBE_BIT
                               ; Is bit 0 (Output Buffer Empty) set?
    jz wait_8042_data          ;     If not then no data waiting to be read, try again
    ret                        ; Otherwise data is ready to be read

; Function: a20_kbd_enable
;           Enable the A20 line via the keyboard controller
;
; Inputs:   None
; Clobbers: AX, CX
; Returns:  None

a20_kbd_enable:
    pushf
    cli                        ; Disable interrupts

    call wait_8042_cmd         ; When controller ready for command
    mov al, 0xad               ; Send command 0xad (disable keyboard).
    out 0x64, al

    call wait_8042_cmd         ; When controller ready for command
    mov al, 0xd0               ; Send command 0xd0 (read output port)
    out 0x64, al

    call wait_8042_data        ; Wait until controller has data
    in al, 0x60                ; Read data from keyboard
    mov cx, ax                 ;     CX = copy of byte read

    call wait_8042_cmd         ; Wait until controller is ready for a command
    mov al, 0xd1
    out 0x64, al               ; Send command 0xd1 (write output port)

    call wait_8042_cmd         ; Wait until controller is ready for a command
    mov ax, cx
    or al, 1 << 1              ; Write value back with bit 1 set
    out 0x60, al

    call wait_8042_cmd         ; Wait until controller is ready for a command
    mov al, 0xae
    out 0x64, al               ; Write command 0xae (enable keyboard)

    call wait_8042_cmd         ; Wait until controller is ready for command
    popf                       ; Restore flags including interrupt flag
    ret

; Function: a20_fast_enable
;           Enable the A20 line via System Control Port A
;
; Inputs:   None
; Clobbers: AX
; Returns:  None

a20_fast_enable:
    in al, 0x92                ; Read System Control Port A
    test al, 1 << 1
    jnz .finished              ; If bit 1 is set then A20 already enabled
    or al, 1 << 1              ; Set bit 1
    and al, ~(1 << 0)          ; Clear bit 0 to avoid issuing a reset
    out 0x92, al               ; Send Enabled A20 and disabled Reset to control port
.finished:
    ret

; Function: a20_bios_enable
;           Enable the A20 line via the BIOS function Int 15h/AH=2401
;
; Inputs:   None
; Clobbers: AX
; Returns:  None

a20_bios_enable:
    mov ax, 0x2401             ; Int 15h/AH=2401 enables A20 on BIOS with this feature
    int 0x15
    ret

; Function: a20_check
;           Determine if the A20 line is enabled or disabled
;
; Inputs:   None
; Clobbers: AX, CX, ES
; Returns:  ZF=1 if A20 enabled, ZF=0 if disabled

a20_check:
    pushf                      ; Save flags so Interrupt Flag (IF) can be restored
    push ds                    ; Save volatile registers
    push si
    push di

    cli                        ; Disable interrupts
    xor ax, ax
    mov ds, ax
    mov si, 0x600              ; 0x0000:0x0600 (0x00600) address we will test

    mov ax, 0xffff
    mov es, ax
    mov di, 0x610              ; 0xffff:0x0610 (0x00600) address we will test
                               ; The physical address pointed to depends on whether
                               ; memory wraps or not. If it wraps then A20 is disabled

    mov cl, [si]               ; Save byte at 0x0000:0x0600
    mov ch, [es:di]            ; Save byte at 0xffff:0x0610

    mov byte [si], 0xaa        ; Write 0xaa to 0x0000:0x0600
    mov byte [es:di], 0x55     ; Write 0x55 to 0xffff:0x0610

    xor ax, ax                 ; Set return value 0
    cmp byte [si], 0x55        ; If 0x0000:0x0600 is 0x55 and not 0xaa
    je .disabled               ;     then memory wrapped because A20 is disabled

    dec ax                     ; A20 Disable, set AX to -1
.disabled:
    ; Cleanup by restoring original bytes in memory. This must be in reverse
    ; order from the order they were originally saved
    mov [es:di], ch            ; Restore data saved data to 0xffff:0x0610
    mov [si], cl               ; Restore data saved data to 0x0000:0x0600

    pop di                     ; Restore non-volatile registers
    pop si
    pop ds
    popf                       ; Restore Flags (including IF)
    test al, al                ; Return ZF=1 if A20 enabled, ZF=0 if disabled
    ret

; Function: a20_enable
;           Enable the A20 line
;
; Inputs:   None
; Clobbers: AX, BX, CX, DX
; Returns:  ZF=0 if A20 not enabled, ZF=1 if A20 enabled

a20_enable:
    push es
    call a20_check             ; Is A20 already enabled?
    jnz .a20_on                ;     If so then we're done ZF=1

    call a20_bios_enable       ; Try enabling A20 via BIOS
    call a20_check             ; Is A20 now enabled?
    jnz .a20_on                ;     If so then we're done ZF=1

    call a20_kbd_enable        ; Try enabling A20 via keyboard controller
    call a20_check             ; Is A20 now enabled?
    jnz .a20_on                ;     If so then we're done ZF=1

    call a20_fast_enable       ; Try enabling A20 via fast method
    call a20_check             ; Is A20 now enabled?
    jnz .a20_on                ;     If so then we're done ZF=1
.a20_err:
    xor ax, ax                 ; If A20 disabled then return with ZF=0
.a20_on:
    pop es
    ret

; Function: check_386
;           Check if this processor is at least a 386
;
; Inputs:   None
; Clobbers: AX
; Returns:  ZF=0 if Processor earlier than a 386, ZF=1 if processor is 386+

check_386:
    xor ax, ax                 ; Zero EFLAGS
    push ax
    popf                       ; Push zeroed flags
    pushf
    pop ax                     ; Get the currently set flags
    and ax, 0xf000             ; if high 4 bits of FLAGS are not set then
    cmp ax, 0xf000             ;     CPU is an 8086/8088/80186/80188
    je .error                  ;     and exit with ZF = 0
    mov ax, 0xf000             ; Set the high 4 bits of FLAGS to 1
    push ax
    popf                       ; Update the FLAGS register
    pushf                      ; Get newly set FLAGS into AX
    pop ax
    and ax, 0xf000             ; if none of the high 4 bits are set then
    jnz .noerror               ;     CPU is an 80286. Return success ZF = 1
                               ;     otherwise CPU is a 386+
.error:
    xor ax, ax                 ; Set ZF = 0 (Earlier than a 386)
.noerror:
    ret

; Load the GDT and fix up each entry by swapping the high 8 bits of
; the base with the access byte. The GDT entries created by MAKE_GDT_DESC
; can't be used directly since the base is encoded in such a way that
; operations other than + or - aren't performed on it..
load_gdt:
    mov cx, NUMGDTENTRIES
    mov si, gdt.start
.fixentry:
    mov al, [si + 5]           ; Get high 8 bits of the base
    mov bl, [si + 7]           ; Get the Access byte
    mov [si + 5], bl           ; Save the swapped values back
    mov [si + 7], al
    add si, 8                  ; Advance to next entry
    loop .fixentry             ; Repeat until all entries processed

    lgdt [gdt.gdtr]            ; Load the GDT record
    ret

; Memory detection function using INT 15h, E820h
detect_memory:
    pushad                      ; Save all registers
    xor ebx, ebx                ; ebx must be 0 to start
    mov edx, 0x534D4150         ; Place "SMAP" into edx
    mov edi, memory_map         ; Set EDI to the start of our memory_map buffer

.repeat:
    mov eax, 0xE820             ; eax = 0xE820
    mov ecx, 24                 ; Ask for 24 bytes
    xor esi, esi                ; Zero out ESI
    int 0x15                    ; Call BIOS
    jc .failed                  ; Carry set means "end of list already reached"
    cmp eax, 0x534D4150         ; On success, eax must have been reset to "SMAP"
    jne .failed

    ; We got a valid entry, store size of entries counter at entries_counter
    ; and increment it
    inc dword [entries_counter]

    ; Go to next entry
    add edi, 24                 ; Move to the next entry
    test ebx, ebx               ; If ebx = 0, we're done
    jnz .repeat                 ; Get next entry

.done:
    popad                       ; Restore all registers
    ret

.failed:
    mov dword [entries_counter], 0  ; If failed, store 0 as number of entries
    jmp .done

; Function: switch_protmode_32
;           Switch processor to 32-bit protected
;           - Enable Interrupts (IF=1)
;           - Paging not enabled
;           - Disable interrupts on the Master and Slave PICs
;           - Flush any pending external interrupts
;           - Jump to 32-bit protected mode at label `protmode32_entry`
;
; Clobbers: N/A
; Returns:  Jumps to label 'protmode32_entry', doesn't return

switch_protmode_32:
    ; Load and fixup the GDT entries
    call load_gdt              ; Load GDT and perform fixups

    ; Enter 32-bit protected mode without paging enabled
    mov eax, cr0               ; Get current CR0
    or eax, 0x00000001         ; Enable protected mode bit
    mov cr0, eax               ; Update CR0
    jmp CODE32_PL0_SEL:protmode32_entry
                               ; Start executing code in 32-bit protected mode
                               ;     Also flushes the instruction prefetch queue

bits 32
protmode32_entry:
    mov eax, DATA32_PL0_SEL    ; Segment registers set with 32-bit DATA selector
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax
    mov esp, PROTMODE_STACK    ; Set protected mode stack to 0x90000

    call kmain                 ; Call the C++ kernel entry point kmain

.hltloop:
    hlt
    jmp .hltloop               ; Infinite loop with interrupts on

section .data
noa20_err db "A20 line couldn't be enabled", 10, 13, 0
not386_err  db "Processor is not a 386+", 10, 13, 0


; Macro to build an initial GDT descriptor entry
;     4 parameters: base, limit, access, flags
;
; The generated descriptor entry will need to be fixed up at runtime
; before interrupts are enabled
;
%macro MAKE_GDT_DESC 4
    dw (%2 & 0xffff)
    dd %1
    db ((%4 & 0x0F) << 4) | ((%2 >> 16) & 0x0f)
    db (%3 & 0xff)
%endmacro

; GDT structure
align 8
gdt:
.start:
.null:       MAKE_GDT_DESC 0, 0, 0, 0
                               ; Null descriptor
.code32_pl0: MAKE_GDT_DESC 0, 0x000FFFFF, 10011011b, 1100b
                               ; 32-bit code, PL0, acc=1, r/x, gran=page
                               ; Lim=0xffffffff
.data32_pl0: MAKE_GDT_DESC 0, 0x000FFFFF, 10010011b, 1100b
                               ; 32-bit data, PL0, acc=1, r/w, gran=page
                               ; Lim=0xffffffff
.end:
NUMGDTENTRIES equ ((gdt.end - gdt.start) / 8)

; GDT record
align 4
    dw 0                       ; Padding align dd GDT in gdtr on 4 byte boundary
.gdtr:
    dw .end - .start - 1       ; limit (Size of GDT - 1)
    dd .start                  ; base of GDT

CODE32_PL0_SEL EQU gdt.code32_pl0 - gdt.start
DATA32_PL0_SEL EQU gdt.data32_pl0 - gdt.start

section .bss
memory_map resb 256 * 24        ; Reserve space for 256 entries, each 24 bytes
entries_counter resd 1          ; Reserve space for the number of entries
