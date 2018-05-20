; -- Demo 3 --
; executes a subroutine using the call instruction and resumes execution using
; ret.
;
; the subroutine is used to print out a null-terminated ASCII string at the
; location pointed to by the HL register to the console in the same manner as
; test ROMs do (by writing the character to $ff01 and then writing $81 to $ff02)


; main entry point at $0100 (cartridge ROM bank 0)
; we just jump to our program routine at $4000
main:          ; @ $0100
  nop          ; 00
  jp program   ; c3 00 40


; the strings that we'll be printing (stored at $3fe0 and $3fea respectively)
bar_str:         ; @ 3fe0
  .db "Bar\n\0"  ; 42 61 72 0a 00
foo_str:         ; @ 3fea
  .db "Foo\n\0"  ; 46 6f 6f 0a 00


; define the location of the IO registers we'll be using to print stuff
; locations of the IO registers are $ff00 + n
.define IO_SB 0x01
.define IO_SC 0x02


; prints the null-terminated ASCII string at HL using serial IO test ROM debug
; functionality; subroutine at $3ff0 (cartridge ROM bank 0)
print_str_at_hl:      ; @ $3ff0
  ; load the ASCII character at HL into register B
  ld b,(hl)           ; 46

  ; set A=0 (A xor A always = 0). check for null-terminator by comparing B with
  ; A=0. if B=0, then the Z flag in register F will be set - this means we've
  ; reached the end of our string, so we return
  xor a               ; af
  cp b                ; b8
  ret z               ; c8

  ; write to the console via serial IO.
  ; to do this, we write our character to IO_SB ($ff01) and then set
  ; IO_SB ($ff02) to $81 to print our character to the console using the test
  ; ROM debugging functionality of sdgbc
  ld a,b              ; 78
  ldh (IO_SB),a       ; e0 01
  ld a,0x81           ; 3e 81
  ldh (IO_SC),a       ; e0 02

  ; increment HL to point to the next character in the string and then run again
  inc hl              ; 23
  jr print_str_at_hl  ; 18 f2


; program routine at $4000 (cartridge ROM bank 1)
program:                ; @ $4000
  ld sp,0xffff          ; 31 ff ff

  ; print "Foo\n\0"
  ld hl,foo_str         ; 21 ea 3f
  call print_str_at_hl  ; cd f0 3f

  ; print "Bar\n\0"
  ld hl,bar_str         ; 21 e0 3f
  call print_str_at_hl  ; cd f0 3f

  ; we're done, just loop forever
- jr -                  ; 18 fe
