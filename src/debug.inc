%ifndef DEBUG_INC
%define DEBUG_INC

%macro LOG 1
%ifdef DEBUG
  push esi

  mov esi, %%tmp
  call print_string
  jmp %%ok

%%tmp: db %1, '$'

%%ok:
  pop esi
%endif
%endmacro

%macro ASSERT 3
%ifdef DEBUG
  %1
  j%+2 %%ok

  LOG %3

  mov ah, 0x4c
  int 0x21

%%ok:
%endif
%endmacro

%macro PRINT8 1
%ifdef DEBUG
  push eax
  mov al, %1
  call printhex8
  pop eax
%endif
%endmacro

%macro PRINT16 1
%ifdef DEBUG
  push eax
  mov ax, %1
  xchg al,ah
  call printhex8
  xchg al,ah
  call printhex8
  pop eax
%endif
%endmacro

%macro PRINT32 1
%ifdef DEBUG
  push eax
  mov eax, %1
  push eax
  shr eax, 16
  PRINT16 ax
  pop eax
  PRINT16 ax
  pop eax
%endif
%endmacro

SEGMENT _TEXT USE32 CLASS=CODE

printhex8:
  pusha

  mov ah, 2
  mov dl, al
  shr dl, 4
  add dl, 0x30
  cmp dl, 0x39
  jle .L1
  add dl, 7
.L1:
  push eax
  int 0x21

  pop eax
  mov dl, al
  and dl, 0xf
  add dl, 0x30
  cmp dl, 0x39
  jle .L2
  add dl, 7
.L2:
  int 0x21

  popa
  ret

; (E)SI = pointer string to print
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

%endif
