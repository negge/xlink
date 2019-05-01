GLOBAL hash_table_segs
GLOBAL hash_table_words

hash_table_memory EQU 12*1024*1024

hash_table_segs  EQU hash_table_memory/65536
hash_table_words EQU hash_table_memory/2

SEGMENT _PROG USE16 CLASS=BSS

GLOBAL ec_segs
GLOBAL ec_bits

ec_segs:
ec_bits: resb 8192

GLOBAL _XLINK_base

SEGMENT _BASE USE16 CLASS=BSS

_XLINK_base: resd 1
