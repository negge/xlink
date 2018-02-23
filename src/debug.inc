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

SEGMENT _TEXT USE32 CLASS=CODE

; DL = character to print
putc:
  push eax

  mov ah,2
  int 0x21

  pop eax
  ret

; (E)SI = pointer string to print
print_string:
  pusha

.L1:
  mov al, [cs:esi]
  inc esi
  cmp al, '$'
  je .L2

  mov dl, al
  mov ah, 2
  int 0x21

  jmp .L1
.L2:
  popa
  ret

%endif