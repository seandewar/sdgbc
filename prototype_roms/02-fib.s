; -- Demo 2 --
; this prototype demo ROM will compute 10 numbers of the fibonnaci sequence
; using arithmetic instructions and push them on the stack
;
; (wont be reversed like the ASCII in demo 1, though - sorry!)


; main entry point at $0100 (cartridge ROM bank 0)
; we just jump to our program routine at $4000
main:          ; @ $0100
  nop          ; 00
  jp program   ; c3 00 40


; program routine at $4000 (cartridge ROM bank 1)
program:        ; @ $4000
   ld sp,0xffff ; 31 ff ff

   ; load initial register values
   ;
   ; A : used to store our next fib number - this reg (the accumulator)
   ;     is written to directly by the arithemtic instructions
   ; B : used to store the current fib number
   ; C : used to store how many iterations are left
   ld a,1       ; 3e 01
   ld b,0       ; 06 00
   ld c,10      ; 0e 0a

   ; begin our fib calc loop & push the current fib number in B as a byte
   ; (push instruction only lets us push 16-bits at a time)
-- ldhl sp,-1   ; f8 ff
   ld (hl),b    ; 70
   dec sp       ; 3b

   ; A will become our current fib number afterwards, so store it in D
   ; so we don't lose it
   ld d,a       ; 57

   ; calculate next fib number and store it in A
   add a,b      ; 80

   ; restore the old value of A from D and put it in B
   ld b,d       ; 42

   ; decrement C. if C>0, loop again
   dec c        ; 0d
   jr nz,--     ; 20 f6

   ; we're done, just loop forever
-  jr -         ; 18 fe
