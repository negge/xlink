;%define DEBUG
%include 'debug.inc'

SEGMENT _MAIN USE32 CLASS=CODE

EXTERN main_

  ; Start with 16-bit code
  bits 16

stub32:
  ; AX = 0x1687 get DPMI real mode to protected mode entry point
  mov ax,0x1687
  int 0x2f

  ; Test for the presence of a DPMI host, AX = 0 means function successful
  dec ax
  js dpmi_ok

  ; Print an error message and exit
  LOG 'No DPMI host found.'

  ret

dpmi_ok:

  ASSERT {test bx, 1}, NZ, 'DPMI host missing 32-bit support.'

  ; ES:DI = segment:offset of the real-to-protected mode entry point
  push es
  push di

  ; AX = 0xffff if DPMI host exists
  ; BX = 1 if DPMI host supports 32-bit programs
  xchg ax, bx

  ; SP points to ES:DI on the stack
  mov di, sp

  ; SI = number of paragraphs required for DPMI host private data area
  ; Rather than allocate memory, set ES = CS + 0x200 (8kB past the load address)
  push cs
  add byte [di+bx], 2
  pop es

  ; Call the real-to-protected mode entry point with the following paramters:
  ;  AX = flags, bit 0 should be 1 for 32-bit applications
  ;  ES = real mode segment of DPMI host private data area
  call far [di]

  ASSERT {}, NC, 'Failed to enter DPMI protected mode.'

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

  ASSERT {}, NC, 'Failed to set 4 GB segment limit for ES.'

  push es
  pop ds

  ; AX = 0008
  inc ax

  ; CS = 16-bit selector with base of real mode CS and a 64KB limit
  mov bx, cs

  ; Load access rights into CH
  lar cx, bx
  ; Set page granular (10000000) and 32-bit (01000000) extended access rights
  mov cl, 0xc0
  xchg ch, cl

  ; Set the CS segment to allow 32-bit code
  ;  AX = 0009h
  ;  BX = selector
  ;  CX = descriptor access rights
  int 0x31

  ; Switch to 32-bit code
  bits 32

  ASSERT {}, NC, 'Failed to change code segment to 32-bit.'

  call main_

  mov ah, 0x4c
  int 0x21
