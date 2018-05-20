; -- Demo 1 --
; this prototype demo ROM will write "Hello World!" onto the stack in ASCII
; using the push and ld instructions.
;
; ASCII values for "Hello World!"
; 48 65 6c 6c 6f 20 77 6f 72 6c 64 21


; main entry point at $0100 (cartridge ROM bank 0)
; we just jump to our program routine at $4000
main:          ; @ $0100
  nop          ; 00
  jp program   ; c3 00 40


; program routine at $4000 (cartridge ROM bank 1)
; write "Hello World!" onto the stack backwards so it reads the right way round
program:       ; @ $4000
  ; start our SP at $ffff just for completeness
  ; (BIOS/boot ROM may have changed our initial stack position)
  ld sp,0xffff ; 31 ff ff

  ; start writing "World!"
  ; we'll use the 8-bit register loads for demo purposes

  ; "d!"
  ld b,0x21    ; 06 21
  ld c,0x64    ; 0e 64
  push bc      ; c5

  ; "rl"
  ld d,0x6c    ; 16 6c
  ld e,0x72    ; 1e 72
  push de      ; d5

  ; "Wo"
  ld h,0x6f    ; 26 6f
  ld l,0x77    ; 2e 77
  push hl      ; e5

  ; start writing "Hello "
  ; this time, we'll use some of the 16-bit load instructions

  ; "o "
  ld bc,0x206f ; 01 6f 20
  push bc      ; c5

  ; "ll"
  ld de,0x6c6c ; 11 6c 6c
  push de      ; d5

  ; "He"
  ld hl,0x6548 ; 21 48 65
  push hl      ; e5

  ; we're done, just loop forever
- jr -         ; 18 fe
