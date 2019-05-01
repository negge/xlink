CPU 386

GLOBAL print_string_16bit
GLOBAL print_string_32bit
GLOBAL print_hex4
GLOBAL print_hex8_16bit
GLOBAL print_hex16_16bit
GLOBAL print_hex32_16bit
GLOBAL print_hex8_32bit
GLOBAL print_hex16_32bit
GLOBAL print_hex32_32bit

SEGMENT _TEXT5 USE16 CLASS=CODE

print_string_16bit:
; Print a string using DOS INT 21h, subfunction 9h
;  DX = pointer to $ terminated string
  pusha
  push ds
  push cs
  pop ds

  mov ah, 9
  int 0x21

  pop ds
  popa
  ret

SEGMENT _TEXT4 USE16 CLASS=CODE

; Print a $ terminated string
;  (E)SI = pointer string to print
print_string_32bit:
  pusha
  push ds
  push cs
  pop ds

.L1:
  lodsb
  cmp al, '$'
  je .L2

  mov dl, al
  mov ah, 2
  int 0x21

  jmp .L1
.L2:
  pop ds
  popa
  ret

SEGMENT _TEXT0 USE16 CLASS=CODE

; Print a 4-bit hexadecimal number
;  AL = number
print_hex4:
  pusha
  mov ah, 2
  mov dl, al
  and dl, 0xf
  add dl, 0x30
  cmp dl, 0x39
  jle .L1
  add dl, 7
.L1:
  int 0x21
  popa
  ret

BITS 16

SEGMENT _TEXT6 USE16 CLASS=CODE

; Print an 8-bit hexadecimal number
;  AL = number
print_hex8_16bit:
  push ax
  shr ax, 4
  call print_hex4
  pop ax
  call print_hex4
  ret

SEGMENT _TEXT7 USE16 CLASS=CODE

; Print a 16-bit hexadecimal number
;  AX = number
print_hex16_16bit:
  push ax
  xchg al, ah
  call print_hex8_16bit
  pop ax
  call print_hex8_16bit
  ret

SEGMENT _TEXT8 USE16 CLASS=CODE

; Print a 32-bit hexadecimal number
;  EAX = number
print_hex32_16bit:
  push eax
  shr eax, 16
  call print_hex16_16bit
  pop eax
  call print_hex16_16bit
  ret

BITS 32

SEGMENT _TEXT1 USE16 CLASS=CODE

; Print an 8-bit hexadecimal number
;  AL = number
print_hex8_32bit:
  push eax
  shr eax, 4
  call print_hex4
  pop eax
  call print_hex4
  ret

SEGMENT _TEXT2 USE16 CLASS=CODE

; Print a 16-bit hexadecimal number
;  AX = number
print_hex16_32bit:
  push eax
  xchg al, ah
  call print_hex8_32bit
  pop eax
  call print_hex8_32bit
  ret

SEGMENT _TEXT3 USE16 CLASS=CODE

; Print a 32-bit hexadecimal number
;  EAX = number
print_hex32_32bit:
  push eax
  shr eax, 16
  call print_hex16_32bit
  pop eax
  call print_hex16_32bit
  ret
