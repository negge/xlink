;%define DEBUG
%include 'debug.inc'

%ifndef XLINK_STUB_NAME
%define XLINK_STUB_NAME stub32
%endif

%ifndef XLINK_STUB_INIT
%define XLINK_STUB_INIT 0
%endif

%ifndef XLINK_STUB_PACK
%define XLINK_STUB_PACK 0
%endif

%ifndef XLINK_STUB_EXIT
%define XLINK_STUB_EXIT 0
%endif

%ifndef XLINK_STUB_BASE
%define XLINK_STUB_BASE 0
%endif

%ifndef XLINK_STUB_FAST
%define XLINK_STUB_FAST 0
%endif

%ifndef XLINK_STUB_CEIL
%define XLINK_STUB_CEIL 0
%endif

struc stack
  .edi: resd 1
  .esi: resd 1
  .ebp: resd 1
  .esp: resd 1
  .ebx: resd 1
  .edx: resd 1
  .ecx: resd 1
  .eax: resd 1
  .size:
endstruc

CPU 386

GLOBAL XLINK_STUB_NAME
GLOBAL _XLINK_heap

%if XLINK_STUB_INIT
EXTERN init_
%endif

%if XLINK_STUB_PACK
%if !XLINK_STUB_INIT
GLOBAL XLINK_heap_offset
%endif
GLOBAL XLINK_header_size

EXTERN hash_table_segs
EXTERN hash_table_words

EXTERN ec_segs
EXTERN ec_bits
%else
EXTERN main_
%endif

%if XLINK_STUB_BASE
EXTERN _XLINK_base
%endif

SEGMENT _MAIN USE32 CLASS=CODE

  ; Start with 16-bit code
  bits 16

XLINK_STUB_NAME:

%if XLINK_STUB_INIT
  call init_
%endif

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

%if XLINK_STUB_PACK || XLINK_STUB_BASE
  push cs
%endif

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
  add byte [di+bx], 4 ; TODO: Make xlink set this past the end of 16-bit BSS
  pop es

  ; Call the real-to-protected mode entry point with the following paramters:
  ;  AX = flags, bit 0 should be 1 for 32-bit applications
  ;  ES = real mode segment of DPMI host private data area
  call far [di]

  ASSERT {}, NC, 'Failed to enter DPMI protected mode.'

  ; BX = 0xffff
  mov dx, bx

%if XLINK_STUB_PACK

  push ax

  ; AL = 1
  mov ah, 5
  mov bx, hash_table_segs

  ; Call allocate memory block
  ;  AX = 0501h
  ;  BX:CX = Size of memory block to allocate in bytes
  int 0x31

  ASSERT {}, NC, 'Failed to allocate memory'

  mov ax, 8
  push ax

  ; BX:CX = linear address of allocated memory block
  push bx
  push cx

%else

  ; AH = 0
  mov al, 8

%endif

  ; ES = selector to program's PSP with a 100H byte limit
  mov bx, es

  mov cx, dx

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

%if XLINK_STUB_PACK

  ; Sets the segment limit to 4GB
  ;  AX = 0008h
  ;  BX = selector
  ;  CX:DX = 32-bit segment limit
  int 0x31

%endif

  ASSERT {}, NC, 'Failed to set 4 GB segment limit for CS.'

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

%if !XLINK_STUB_PACK

%if XLINK_STUB_BASE
  ; Pop the real-to-protected mode entry point (and ignore it)
  pop eax

  ; Pop the real mode 0:cs and convert to a zero based address
  pop eax
  shl eax, 4
  mov [_XLINK_base], eax
%endif

%if !XLINK_STUB_EXIT
  call main_

  mov ah, 0x4c
  int 0x21
%endif

GLOBAL XLINK_heap_base

XLINK_heap_base EQU 0

SEGMENT _DATA USE32 CLASS=DATA

_XLINK_heap: dd XLINK_heap_base

%else
  ; Set ESI to the start of the EC header (minus models in first EC segment)
  mov esi, ec_segs - 0x9

  ; Pop the zero based address of the allocated memory
  pop eax

  ; Pop the address 0x10008
  pop edi

  ; Pop the real-to-protected mode entry point (and ignore it)
  pop ebx

  ; Pop the real mode 0:cs and convert to a zero based address
  pop ebx
  shl ebx, 4

%if XLINK_STUB_BASE
  mov [edi - 8], ebx
%endif

  ; Compute and store the hashtable address relative to ES
  sub eax, ebx
%if !XLINK_STUB_INIT
  ;mov [esi + _XLINK_heap - stub32_end + 0x0], eax
  db 0x89, 0x46
XLINK_heap_offset: db _XLINK_heap - stub32_end
%else
  mov [_XLINK_heap], eax
%endif

  ; Start by clearing the memory
  ;  edi = 0x10008
  mov ecx, edi
  xor eax, eax

  ; Manually clear the first 8 bytes
  stosd
  stosd

  ; EDI = 0x10008 + 0x8 is the entry point of the binary
  push edi
  ; EBP is the starting bit index and must be 0
  xor ebp, ebp
  ; EAX is the starting range and must be 1
  inc eax
  pushad
  dec eax
  ; Clear 256kB of memory
  rep stosd

  ; Hash table clear routine expects carry to be set
  stc
  jmp @clear_hash_table



@done_model:

  ; if EBX == 0 or EBX > 0x7fffffff (EBX = 0 or -1 if we've decoded a bit)
  add ebx, ebx
  popad
  ; ZF = 0 and SF = OF
  jg @decode_bit

@init_byte:
  rcl byte [edi], 1
  jnc @next_bit
  inc edi
  jmp @init_byte



; Load a bit into the range coder
;  EBP = bit index
;  EAX = range
;  ECX = base
@need_bit:

  ; Read the bit into carry flag
  bt [ec_bits], ebp

  ; Update base: ECX = (base << 1) + bit
  adc ecx, ecx

  ; Update range: EAX = range << 1
  shl eax, 1

  inc ebp

; Decode a range coded bit using computed counts
;  EAX = range
;  ECX = base
;  EBX = counts for 0
;  EDX = counts for 1
;  EBP = next bit index
@decode_bit:

  ; While EAX < 0x8000000, need a bit
  test eax, eax
  jns @need_bit

  ; Compute c0 + c1
  ;  EBX = c0
  ;  EDX = c1
  add ebx, edx

  ; EAX = range
  push eax

  ; Compute s = range*p0 = range*c0/(c0 + c1)
  ;  EAX = range
  ;  EDX = c0
  ;  EBX = c0 + c1
  mul edx
  div ebx

  ; [ESP] = range
  pop edx

  ; Decoded bit = 0 if base < s
  cmp ecx, eax
  ; Set ebx = 0 or -1
  sbb ebx, ebx

  jc @next_bit

  ; EAX = s
  ; EDX = range
  xchg eax, edx

  ; ECX = base
  ; EDX = s
  sub ecx, edx

  ; EAX = range
  ; EDX = s
  sub eax, edx

  ;jmp @next_bit



@next_bit:

  ; ebp = bit index
  ; eax = range
  ; ecx = value
  ; ebx = c0 count
  ; edx = c1 count
  ; edi = output bytes
  ; esi = EC segment
  pushad

  ; ESI = start of EC segment
  ; [ESI] encodes the length of the EC segment as -EDI when done
  lodsd

  ; EAX = -(start address + number of bytes)
  ; EDI = address of currently decoding byte
  add eax, edi

  ; EAX = 0 when we are done with this EC segment
  jz @clear_hash_table

  ; Reset the counts (initial value changes adaptation rate)
  push 2
  pop edx
  mov [esp + stack.edx], edx
  ;mov [esp + stack.ebx], edx
  mov [esp + edx*8], edx

  ; [ESI] encodes the number of models and their weights
  lodsd
  xor ebp, ebp
@next_model:
  dec ebp
@double_weight:
  inc ebp
  add eax, eax
  jc @double_weight
  jz @done_model

  pushad

  ; Load the model
  lodsb

  mov dl, al
@hash_byte:
  ; EDI = location of byte we are decoding on first pass
  ; [EDI] = partially decoded byte or previously decoded byte
  xor al, [edi]
%if XLINK_STUB_FAST
  rol eax, 9
%else
  imul eax, 0x6f
%endif
  add al, [edi]
  dec eax
@skip_byte:
  dec edi
  add dl, dl
  jc @hash_byte
  jnz @skip_byte

@clear_hash_table:

  ;mov edi, 0
  db 0xbf
_XLINK_heap: dd 0
  mov ecx, hash_table_words
  jnc @index_hash

  rep stosw

  or al, [esi]
  popad

  ;TODO need to hardcode this value 8 bytes + n model bytes
  ;lea esi, [esi + 0x9]
  db 0x8D, 0x76
XLINK_header_size: db 0x9
  ;TODO we can flip this (and all of the weight dwords) if ec_seg - XXX is odd
  jpo @next_bit

@done_decoding:

  ; [ESP] = 0x10010
  ret



@index_hash:

  ; Compute the remainder to index the hash table
  div ecx

  ; EDX = computed hash
  ; EDI = hash table offset
  lea edi, [edi + 2*edx + 1]

  ;TODO don't recompute EDI and instead just store it
  ;store edi somewhere indexed by ESI
  ;mov [ebp + 4*esi], edi
  ;mov edi, [ebp + 4*esi]

  ; Accumulate weighted counts for model

  ; EBP = model weight
  mov ecx, ebp

  ; EDI = hash index
  ; ECX = model weight
  xor eax, eax
@load_counts:
  movzx edx, byte [edi + eax]
  shl edx, cl
  ; Index stack to update EDX (when EAX = 0) and EBX (when EAX = -1)
  add [esp + eax*4 + stack.size + stack.edx], edx
  dec eax
  jp @load_counts

  test ebx, ebx
  jg @skip_update
  shr byte [edi + ebx], 1
  jnz @floor_fine
  rcl byte [edi + ebx], 1
@floor_fine:
  not ebx
  inc byte [edi + ebx]
%if XLINK_STUB_CEIL
  jnz @ceil_fine
  dec byte [edi + ebx]
@ceil_fine:
%endif

@skip_update:
  popad
  inc esi
  jmp @next_model


stub32_end:

%endif
