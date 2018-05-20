; -- Demo 5 --
; demonstration of the functionality of switching VRAM banks
;
; NOTE: there's also support for switching WRAM banks 1-7 at $d000-dfff, but
;       its the exact same mechanism as switching VRAM banks, which is simpler
;       to demo imo


; enable CGB features - VRAM banks can't be switched in normal DMG mode
cgb_mode:    ; @ $0143
  .db 0xc0   ; c0


; main entry point at $0100 (cartridge ROM bank 0)
; we just jump to our program routine at $4000
main:          ; @ $0100
  nop          ; 00
  jp program   ; c3 00 40


; the different IO registers for bank switching VRAM
; VBK  - switches between VRAM banks 0 and 1 (addressed at $8000-$9fff)
.define IO_VBK 0x4f


; program routine at $4000 (cartridge ROM bank 1)
program:                ; @ $4000
  ld sp,0xffff          ; 31 ff ff

  ; first, we'll ensure that we're currently using VRAM bank #0
  xor a                 ; af
  ldh (IO_VBK),a        ; e0 4f

  ; now, to demonstrate, let's write $aa to $8000 (VRAM bank 0)
  ld hl,0x8000          ; 21 00 80
  ld (hl),0xaa          ; 36 aa
  ; and then switch to VRAM bank 1
  inc a                 ; 3c
  ldh (IO_VBK),a        ; e0 4f

  ; now, if you inspect $8000 the $aa we wrote is now gone!
  ;
  ; actually, it's still in memory within VRAM bank #0 but we've switched banks
  ; so that $8000-$9fff now maps to VRAM bank #1!
  ;
  ; we'll write $bb to $8000 (VRAM bank 1 this time) (HL is already $8000)
  ld (hl),0xbb          ; 36 bb

  ; and we'll switch back to VRAM bank 0
  dec a                 ; 3d
  ldh (IO_VBK),a        ; e0 4f

  ; now, if you inspect $8000, $aa is now shown again
  ;
  ; let's again switch banks to VRAM bank 1
  inc a                 ; 3c
  ldh (IO_VBK),a        ; e0 4f
  ; and again, we now see $bb at $8000

  ; we're done, just loop forever
- jr -                  ; 18 fe
