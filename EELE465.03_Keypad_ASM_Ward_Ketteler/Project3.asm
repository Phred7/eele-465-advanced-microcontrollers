;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
; H.Ketteler, W.Ward, EELE465, Project 03
; Frb. 16 2022
; Keypad and LED bar
;
;--------------------------------Notes------------------------------------------
; R4: Keypad Status
; R5: Last Key press
; R6: Pattern B mask
; R7: Pattern C mask
; R8: Pattern D mask
;-------------------------------------------------------------------------------
            .cdecls C,LIST,"msp430.h"       ; Include device header file
            
;-------------------------------------------------------------------------------
            .def    RESET                   ; Export program entry-point to
                                            ; make it known to linker.
;-------------------------------------------------------------------------------
            .text                           ; Assemble into program memory.
            .retain                         ; Override ELF conditional linking
                                            ; and retain current section.
            .retainrefs                     ; And retain any sections that have
                                            ; references to current section.

;-------------------------------------------------------------------------------
RESET       mov.w   #__STACK_END,SP         ; Initialize stackpointer
StopWDT     mov.w   #WDTPW|WDTHOLD,&WDTCTL  ; Stop watchdog timer


;-------------------------------------------------------------------------------
; Main loop here
;-------------------------------------------------------------------------------
init:
			;-- Setup LED bar as output on P3
			bis.b	#0FFFFh, &P3DIR
			bic.b	#0FFFFh, &P3OUT

			;-- Setup Keypad
			bic.b	#000FFh, &P5DIR
			bic.b	#000FFh, &P5OUT
			bic.b	#000FFh, &P6DIR
			bic.b	#000FFh, &P6OUT

			;-- Setup timber TB0: delta-5 = 1sec
			bis.w	#TBCLR, &TB0CTL 		; clears timers and dividers
			bis.w	#TBSSEL__SMCLK, &TB0CTL	; choose clock (f = 1 MHz)
			bis.w	#MC__UP, &TB0CTL		; choose mode (UP)
			bis.w	#CNTL_0, &TB0CTL		; choose counter length (N = 2^16)
			bis.w	#ID__8, &TB0CTL			; choose divider for D1 (D1 = 4)
			bis.w 	#TBIDEX__8, &TB0EX0		; choose divider for D2 (D2 = 8)

			;-- Setup TB0 Interrupt: Compare
			mov.w	#32992d, &TB0CCR0		; N = 15625: TB0 @ 0.5sec, N = 32992d for 1Hz
			bis.w	#CCIE, &TB0CCTL0		; enable overflow interrupt
			bic.w	#CCIFG, &TB0CCTL0		; disable interrupt flag

			nop
			eint							; assert global interrupt flag
			nop

			bic.b	#LOCKLPM5, &PM5CTL0		; disable DIO low-power default

			mov.w	#00h, R4
			mov.w	#00h, R5
			mov.w	#00h, R6
			mov.w	#00h, R7
			mov.w	#00h, R8

main:
			; while

			call 	#check_keypad

			cmp.b	#081h, R4				; if z=1 A was pressed
			jz		p_a
			cmp.b	#041h, R4				; if z=1 B was pressed
			jz		p_b
			cmp.b	#021h, R4				; if z=1 C was pressed
			jz		p_c
			cmp.b	#011h, R4				; if z=1  was pressed
			jz		p_d
			jmp 	p_end

p_a:		call	#pattern_a
			jmp		p_end
p_b:		call	#pattern_b
			jmp		p_end
p_c:		call	#pattern_c
			jmp		p_end
p_d:		call	#pattern_d
			jmp		p_end
p_end:
			cmp.b	#00h, R4
			jz		end_main				; if R4 is not 00h then move R4 into R6
			mov.w 	R4, R6
end_main:
			mov.w	#00h, R4
			jmp 	main
			nop

check_keypad:
			mov.b	#021h, R4
			ret

pattern_a:
			bic.w	#CCIE, &TB0CCTL0		; disable timer interrupt
			bis.b	pattern_A_bit_mask, &P3OUT
			ret

pattern_b:
			mov.w	#15625d, &TB0CCR0		; N = 15625: TB0 @ 0.5sec, N = 32992d for 1Hz
			bis.w	#CCIE, &TB0CCTL0
			cmp.b	R4, R5					; if the last button press was B and this button press was B z=1 and restart the pattern
			jnz		b_cmp_end
			mov.w	#00h, R6
b_cmp_end:	mov.b	#008h, R6				; test code
			bis.b	R6, &P3OUT
			ret

pattern_c:
			mov.w	#32992d, &TB0CCR0		; N = 15625: TB0 @ 0.5sec, N = 32992d for 1Hz
			bis.w	#CCIE, &TB0CCTL0
			mov.b	#0FFh, R7				; test code
			bis.b 	R7, &P3OUT
			ret

pattern_d:
			ret

unlock_code:
			ret


;-------------------------------------------------------------------------------
; Interrupt Service Routines
;-------------------------------------------------------------------------------

; Service TB0
TimerB0_ISR:
;			xor.b	#BIT6, &P6OUT
			bic.w	#TBIFG, &TB0CTL
			reti
;-------------- END service_TB0 --------------

;-------------------------------------------------------------------------------
; Memory Allocation
;-------------------------------------------------------------------------------
						.data
						.retain

pattern_A_bit_mask: 	.byte	0AAh	; 010101010b

;-------------------------------------------------------------------------------
; Stack Pointer definition
;-------------------------------------------------------------------------------
            .global __STACK_END
            .sect   .stack
            
;-------------------------------------------------------------------------------
; Interrupt Vectors
;-------------------------------------------------------------------------------
            .sect   ".reset"                ; MSP430 RESET Vector
            .short  RESET
            
            .sect	".int43"				; TB0CCR0 compare interrupt vector
            .short	TimerB0_ISR

