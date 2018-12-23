CPU 386

BITS 32

GLOBAL print_hex4
GLOBAL print_hex8
GLOBAL print_hex16
GLOBAL print_hex32
GLOBAL print_string

SEGMENT _TEXT4 USE16 CLASS=CODE

; Print a $ terminated string
;  (E)SI = pointer string to print
print_string:
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

SEGMENT _TEXT1 USE16 CLASS=CODE

; Print an 8-bit hexadecimal number
;  AL = number
print_hex8:
  push eax
  shr eax, 4
  call print_hex4
  pop eax
  call print_hex4
  ret

SEGMENT _TEXT2 USE16 CLASS=CODE

; Print a 16-bit hexadecimal number
;  AX = number
print_hex16:
  push eax
  xchg al, ah
  call print_hex8
  pop eax
  call print_hex8
  ret

SEGMENT _TEXT3 USE16 CLASS=CODE

; Print a 32-bit hexadecimal number
;  EAX = number
print_hex32:
  push eax
  shr eax, 16
  call print_hex16
  pop eax
  call print_hex16
  ret
