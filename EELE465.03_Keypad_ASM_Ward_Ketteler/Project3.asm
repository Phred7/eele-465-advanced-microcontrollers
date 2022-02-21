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
; R9: Timer Interrupt Keypad Compare [1h: 'A', 2h: 'B', 3h: 'C', 4h: 'D']
; R10: Second Keypad input byte
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
			bis.b	#0FFFFh, &P3DIR			; Set as output
			bic.b	#0FFFFh, &P3OUT

			;-- Setup Keypad
			bic.b	#0000Fh, &P5DIR			; Set as input
			bis.b	#0000Fh, &P5REN			; EN pull up/down
			bic.b	#0000Fh, &P5OUT			; All pull down
			bic.b	#0000Fh, &P6DIR			; Set as input
			bis.b	#0000Fh, &P6REN			; EN pull up/down
			bic.b	#0000Fh, &P6OUT			; All pull down

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
			mov.w	#01111111b, R7
			mov.w	#00h, R8

			bic.b	#0FFFFh, &P3OUT



main:
			; while

			call 	#check_keypad

			;-- Compare the current keypress with known values
			cmp.b	#081h, R4				; if z=1 A was pressed
			jz		p_a
			cmp.b	#041h, R4				; if z=1 B was pressed
			jz		p_b
			cmp.b	#021h, R4				; if z=1 C was pressed
			jz		p_c
			cmp.b	#011h, R4				; if z=1 D was pressed
			jz		p_d

			mov.w	#00h, R4

			;-- At this point something else or nothing was pressed. So, check the last valid key that was pressed
			cmp.b	#081h, R5				; if z=1 A was pressed
			jz		p_a
			cmp.b	#041h, R5				; if z=1 B was pressed
			jz		p_b
			cmp.b	#021h, R5				; if z=1 C was pressed
			jz		p_c
			cmp.b	#011h, R5				; if z=1 D was pressed
			jz		p_d

			jmp 	p_end

p_a:		call	#pattern_a
			call 	#update_r5
			jmp		p_end
p_b:		call	#pattern_b
			call 	#update_r5
			jmp		p_end
p_c:		call	#pattern_c
			call 	#update_r5
			jmp		p_end
p_d:		call	#pattern_d
			call 	#update_r5
			jmp		p_end
p_end:

end_main:
			mov.w	#00h, R4
			jmp 	main
			nop



check_keypad:
			;mov.b	#041h, R4
			;mov.b	#01h, R5

			;-- Check BIT0 set
			; bit.b	#BIT0, &P5IN			; if Z==0 then the BIT0 is not set in P5IN

			;-- P6.0-P6.3 as input with pull down
			bic.b	#0000Fh, &P6DIR			; Set as input
			bis.b	#0000Fh, &P6REN			; EN pull up/down
			bic.b	#0000Fh, &P6OUT			; All pull down

			;-- P5.0-P5.3 as out and set HIGH
			bis.b	#0000Fh, &P5DIR  		; set rows high then check columns ; Set as output
			bis.b	#0000Fh, &P5OUT

			mov.b	&P6IN, R4

			;-- P5.0-P5.3 as input with pull down
			bic.b	#0000Fh, &P5DIR			; Set as input
			bis.b	#0000Fh, &P5REN			; EN pull up/down
			bic.b	#0000Fh, &P5OUT			; All pull down

			;-- P6.0-P6.3 as out and set HIGH
			bis.b	#0000Fh, &P6DIR  		; set rows high then check columns ; Set as output
			bis.b	#0000Fh, &P6OUT

			;-- Combine P6.0-P6.3 and P5.0-P5.3 with P6.3 being MSB and P5.0 being LSB
			setc
			rlc.b	R4
			setc
			rlc.b	R4
			setc
			rlc.b	R4
			setc
			rlc.b	R4
			mov.b	#0F0h, R12
			or.b	&P5IN, R12
			and.b	R12, R4

ck_end:		ret



pattern_a:
			bic.w	#CCIE, &TB0CCTL0		; disable timer interrupt
			mov.b	#01h, R9
			mov.b	pattern_A_bit_mask, &P3OUT
			ret



pattern_b:
			mov.w	#1562d, &TB0CCR0		; N = 15625: TB0 @ 0.5sec, N = 32992d for 1Hz
			mov.b	#02h, R9
			bis.w	#CCIE, &TB0CCTL0
			cmp.b	R4, R5					; if the last button press was B and this button press was B z=1 and restart the pattern
			jnz		b_cmp_end
			mov.w	#00h, R6
b_cmp_end:
			mov.b	R6, &P3OUT
			ret



pattern_c:
			mov.w	#3299d, &TB0CCR0		; N = 15625: TB0 @ 0.5sec, N = 32992d for 1Hz
			mov.b	#03h, R9
			bis.w	#CCIE, &TB0CCTL0
			mov.b 	R7, &P3OUT
			ret



pattern_d:
			mov.b	#04h, R9
			ret


update_r5:
			cmp.b	#00h, R4
			jz		update_r5_end	; if R4 is not 00h, then update R5 with the value of R4
			mov.w 	R4, R5
update_r5_end:
			ret



unlock_code:
			ret


;-------------------------------------------------------------------------------
; Interrupt Service Routines
;-------------------------------------------------------------------------------

; Service TB0
TimerB0_ISR:
				cmp.b	#02h, R9	; if R9 is 02h then the current pattern is pattern B
				jz		update_pattern_b
				cmp.b	#03h, R9	; if R9 is 03h then the current pattern is pattern C
				jz		update_pattern_c
				cmp.b	#04h, R9	; if R9 is 04h then the current pattern is pattern D
				jz		update_pattern_d
end_tb0_isr:	bic.w	#TBIFG, &TB0CTL
				reti

update_pattern_b:
				inc.b	R6
				jmp 	end_tb0_isr
update_pattern_c:
				rrc.b	R7
				cmp.b	#0FFh, R7		; if R7 is 0FFh, reset to starting value
				jnz		end_tb0_isr
				mov.w	#01111111b, R7
				jmp 	end_tb0_isr
update_pattern_d:
				jmp 	end_tb0_isr
				nop


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
