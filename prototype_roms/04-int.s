; -- Demo 4 --
; demonstrates interrupt functionality of the CPU by triggering some ISRs


; main entry point at $0100 (cartridge ROM bank 0)
; we just jump to our program routine at $4000
main:          ; @ $0100
  nop          ; 00
  jp program   ; c3 00 40


; contents of the interrupt vectors at $0040, $0048, $0050, $0058 and $0060
; for the sake of demonstration, we'll JP to a simple ISR for vector $0060
ivec_40h:       ; @ $0040
  reti          ; d9
ivec_48h:       ; @ $0048
  reti          ; d9
ivec_50h:       ; @ $0050
  reti          ; d9
ivec_58h:       ; @ $0058
  reti          ; d9
ivec_60h:       ; @ $0060
  jp fancy_isr  ; c3 f0 3f


; our super fancy custom ISR at $3ff0 (cartridge ROM bank 0)
; just pointlessly sets A to 1 and multiplies it by 4 via 2 bitwise left shifts
;
; in a real application, we'd likely use an ISR to start writing to VRAM after
; the V-Blank interrupt has been triggered (vector $40 in that case), for
; example
fancy_isr:  ; @ $3ff0
  ld a,1    ; 3e 01

  ; bitwise shift A to the left twice to mul by 4 - because why not?
  ; NOTE: SLA is a 2 byte instr because it is part of the extended instruction
  ;       set (prefixed by opcode $cb)
  sla a     ; cb 27
  sla a     ; cb 27

  ; returns from our ISR and re-enables interrupts (interrupts are automatically
  ; disabled at the CPU-level when an interrupt is triggered - this stops other
  ; ints from triggering while execution is inside of an ISR)
  reti      ; d9


; the different IO registers for controlling interrupts at $ff0f and $ffff
;
; IF - allows us to request an interrupt to be triggered (only happens if
;      the corrisponding bit in IE is set)
; IE - allows us to enable/disable specific interrupts from triggering
;      (even if the corrisponding bit in IF is set to request an interrupt, it
;       will not be triggered until the bit in IE is enabled)
;
; NOTE: interrupts themselves need to be enabled on the CPU via the EI instr
;       for anything to happen, regardless of the values of IF and IE
.define IO_IF 0x0f
.define IO_IE 0xff


; program routine at $4000 (cartridge ROM bank 1)
program:                ; @ $4000
  ld sp,0xffff          ; 31 ff ff

  ; ensure that interrupts are enabled at the CPU-level
  ei                    ; fb

  ; firstly, as a demonstration, we'll enable and then request all 5 different
  ; interrupt types
  ;
  ; to do this, we enable all the 5 int types via setting the lower 5 bits of
  ; IE to 1. then, we request the ints by setting the lower 5 bits of IF to 1
  ; ($1f = 0b00011111)
  ld a,0x1f             ; 3e 1f
  ldh (IO_IE),a         ; e0 ff
  ldh (IO_IF),a         ; e0 0f

  ; secondly, as a demonstration, we'll request all 5 different int types, while
  ; only enabling the interrupt that vectors at $0060
  ; in this case, only that interrupt will fire, even though all int types
  ; were requested
  ;
  ; to do this, we enable the int that vectors to $60 by setting bit 4 of IE to
  ; 1. then, as before, we set the lower 5 bits of IF to 1.
  ; ($10 = 0b00010000)
  ld a,0x10             ; 3e 10
  ldh (IO_IE),a         ; e0 ff
  ld a,0x1f             ; 3e 1f
  ldh (IO_IF),a         ; e0 0f

  ; thirdly, we'll enable and request all 5 int types, this time with interrupts
  ; disabled on the CPU via the DI instruction (no interrupts should be
  ; triggered)
  di                    ; f3
  ld a,0x1f             ; 3e 1f
  ldh (IO_IE),a         ; e0 ff
  ldh (IO_IF),a         ; e0 0f

  ; we're done, just loop forever
- jr -                  ; 18 fe
