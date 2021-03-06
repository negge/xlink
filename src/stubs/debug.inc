%ifndef DEBUG_INC
%define DEBUG_INC

EXTERN print_string_16bit
EXTERN print_string_32bit
EXTERN print_hex4
EXTERN print_hex8_16bit
EXTERN print_hex16_16bit
EXTERN print_hex32_16bit
EXTERN print_hex8_32bit
EXTERN print_hex16_32bit
EXTERN print_hex32_32bit

%define quot '
%define stringify(x) quot %+ x %+ quot

%macro PRINT 1
  pusha
%if __BITS__ == 16
  mov dx, %%tmp
  call print_string_16bit
%else
  mov esi, %%tmp
  call print_string_32bit
%endif
  jmp %%ok

%%tmp: db %1, '$'

%%ok:
  popa
%endmacro

%macro LOG 1
%ifdef DEBUG
  PRINT {%1}
%endif
%endmacro

%macro ASSERT 3
%ifdef DEBUG
  %1
  j%+2 %%ok

  PRINT %3

  mov ah, 0x4c
  int 0x21

%%ok:
%endif
%endmacro

%macro REG8 1
%ifdef DEBUG
  PRINT {stringify(%1), ' = '}
  PRINT8 %1
  PRINT {13, 10}
%endif
%endmacro

%macro REG16 1
%ifdef DEBUG
  PRINT {stringify(%1), ' = '}
  PRINT16 %1
  PRINT {13, 10}
%endif
%endmacro

%macro REG32 1
%ifdef DEBUG
  PRINT {stringify(%1), ' = '}
  PRINT32 %1
  PRINT {13, 10}
%endif
%endmacro

%macro PRINT8 1
%if __BITS__ == 16
  push ax
  mov al, %1
  call print_hex8_16bit
  pop ax
%else
  push eax
  mov al, %1
  call print_hex8_32bit
  pop eax
%endif
%endmacro

%macro PRINT16 1
%if __BITS__ == 16
  push ax
  mov ax, %1
  call print_hex16_16bit
  pop ax
%else
  push eax
  mov ax, %1
  call print_hex16_32bit
  pop eax
%endif
%endmacro

%macro PRINT32 1
%if __BITS__ == 16
  push eax
  mov eax, %1
  call print_hex32_16bit
  pop eax
%else
  push eax
  mov eax, %1
  call print_hex32_32bit
  pop eax
%endif
%endmacro

%endif
