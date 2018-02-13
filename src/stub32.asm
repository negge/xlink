SEGMENT _MAIN USE32 CLASS=CODE

EXTERN main_
GLOBAL _xlink_base

  ; Start with 16-bit code
  bits 16

stub32:
  ; AX = 0x1687 get DPMI real mode to protected mode entry point
  mov ax,0x1687
  int 0x2f

  ; Test for the presence of a DPMI host, AX = 0 means function successful
  dec ax
  js dpmi_ok

  ret

dpmi_ok:

  ; ES:DI = segment:offset of the real-to-protected mode entry point
  push es
  push di

  ; AX = 0xffff if DPMI host exists
  ; BX = 1 if DPMI host supports 32-bit programs
  xchg ax, bx

  ; SI = number of paragraphs required for DPMI host private data area
  mov dx, cs
  inc dh
  push dx
  ; This sets ES = CS + 0x100 or 4k past the entry point
  ; TODO: ensure this doesn't collide with other BSS data
  pop es

  ; Call the real-to-protected mode entry point with the following paramters:
  ;  AX = flags, bit 0 should be 1 for 32-bit applications
  ;  ES = real mode segment of DPMI host private data area
  mov di, sp
  call far [di]

  ; BX = 0xffff
  push bx
  push bx
  pop cx
  pop dx

  ; ES = selector to program's PSP with a 100H byte limit
  mov bx, es

  ; AH = 0
  mov al, 0x8

  ; Sets the segment limit to 4GB
  ;  AX = 0008h
  ;  BX = selector
  ;  CX:DX = 32-bit segment limit
  int 0x31

  push es
  pop ds

  ; AH = 0
  mov al, 0x6

  ; Gets the segment base address
  ;  AX = 0006h
  ;  BX = selector
  int 0x31

  push cx
  push dx
  pop dword [_xlink_base]

  ; AH = 0
  mov al, 0x9

  ; CS = 16-bit selector with base of real mode CS and a 64KB limit
  mov bx, cs

  ; CX = 0xffff
  inc cx
  mov cl, bl
  shl cl, 5
  or cx, 0xC09A

  ; Set the CS segment to allow 32-bit code
  ;  AX = 0009h
  ;  BX = selector
  ;  CX = descriptor access rights
  int 0x31

  ; Switch to 32-bit code
  bits 32

  call main_

  mov ah, 0x4c
  int 0x21

SEGMENT _BSS USE32 CLASS=BSS

_xlink_base:
  RESD 1
