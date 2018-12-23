%ifndef DEBUG_INC
%define DEBUG_INC

EXTERN print_hex4
EXTERN print_hex8
EXTERN print_hex16
EXTERN print_hex32
EXTERN print_string

%macro LOG 1
%ifdef DEBUG
%if __BITS__ == 16
  pusha

  mov ah, 9
  mov dx, %%tmp
  int 0x21
  jmp %%ok

%%tmp: db %1, '$'

%%ok:
  popa
%else
  push esi

  mov esi, %%tmp
  call print_string
  jmp %%ok

%%tmp: db %1, '$'

%%ok:
  pop esi
%endif
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
  call print_hex8
  pop eax
%endif
%endmacro

%macro PRINT16 1
%ifdef DEBUG
  push eax
  mov ax, %1
  call print_hex16
  pop eax
%endif
%endmacro

%macro PRINT32 1
%ifdef DEBUG
  push eax
  mov eax, %1
  call print_hex32
  pop eax
%endif
%endmacro

%endif
