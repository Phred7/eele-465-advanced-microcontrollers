;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
;
;
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
			bis.b	#BIT0, &P1DIR			; Set as output P1.0
			bic.b	#BIT0, &P1OUT

			bis.b	#BIT1, &P1DIR			; Set as output P1.1
			bic.b	#BIT1, &P1OUT

			bis.b	#BIT4, &P1DIR			; Set as output P1.4
			bic.b	#BIT4, &P1OUT

			bis.b	#BIT5, &P1DIR			; Set as output P1.5
			bic.b	#BIT5, &P1OUT

			bis.b	#BIT6, &P1DIR			; Set as output P1.6
			bic.b	#BIT6, &P1OUT

			bis.b	#BIT7, &P1DIR			; Set as output P1.7
			bic.b	#BIT7, &P1OUT

			bis.b	#BIT6, &P2DIR			; Set as output P2.6
			bic.b	#BIT6, &P2OUT

			bis.b	#BIT7, &P2DIR			; Set as output P2.7
			bic.b	#BIT7, &P2OUT

			bic.b	#LOCKLPM5, &PM5CTL0		; disable DIO low-power default

main:
			xor.b	#011110011b, &P1OUT
			xor.b	#011000000b, &P2OUT
			call	#delay
			jmp 	main
			nop

                                            

;-------------------------------------------------------------------------------
; Subroutines
;-------------------------------------------------------------------------------

delay:			mov.w	global_outer_delay, R4  ; sets outer loop delay
delay_gid:		mov.w	global_inner_delay, R5	; sets inner loop delay
dec_inner:		dec		R5						; decrements inner delay reg
				jnz		dec_inner				; loop if R5 is not 0
				dec		R4						; decrements outer delay reg
				jnz		delay_gid				; loop if R4 is not 0
				ret

;-------------------------------------------------------------------------------
; Memory Allocation
;-------------------------------------------------------------------------------

					.data
					.retain

global_outer_delay:		.short	00BD3h
global_inner_delay:		.short  00072h

;global_outer_delay:		.short	00005h
;global_inner_delay:		.short  00002h

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
            
