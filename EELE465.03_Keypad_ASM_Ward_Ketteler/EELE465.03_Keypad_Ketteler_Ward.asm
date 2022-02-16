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
			bis.w	#ID__4, &TB0CTL			; choose divider for D1 (D1 = 4)
			bis.w 	#TBIDEX__8, &TB0EX0		; choose divider for D2 (D2 = 8)

			;-- Setup TB0 Interrupt: Compare
			mov.w	#32992d, &TB0CCR0		; N = 15625: TB0 @ 0.5sec, N = 32992d for 1Hz
			bis.w	#CCIE, &TB0CCTL0
			bic.w	#CCIFG, &TB0CCTL0

			nop
			eint							; assert global interrupt flag
			nop

			bic.b	#LOCKLPM5, &PM5CTL0		; disable DIO low-power default

main:
			bis.b	#0FFFFh, &P3OUT
			jmp main
			nop


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
            
            .sect	".int43"				; TB0CCR0
            .short	TimerB0_ISR

