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
RESET       		mov.w   #__STACK_END,SP         ; Initialize stackpointer
StopWDT     		mov.w   #WDTPW|WDTHOLD,&WDTCTL  ; Stop watchdog timer


;-------------------------------------------------------------------------------
; Main loop here
;-------------------------------------------------------------------------------
init:
					bis.b	#BIT6, &P6DIR 			; set green LED2 as out - reps SCL
					bic.b	#BIT6, &P6OUT 			; set init val to 0

					bis.b	#BIT0, &P1DIR 			; set red LED1 as out - reps SDA
					bic.b	#BIT0, &P1OUT 			; set init val to 0

					bis.b	#BIT2, &P3DIR			; set P3.2 as out for SCL
					bic.b	#BIT2, &P3OUT			; set init val to 0

					bis.b	#BIT3, &P3DIR			; set P3.3 as out for SDA
					bic.b	#BIT3, &P3OUT			; set init val to 0

					; setup timer TB0:
					bis.w	#TBCLR, &TB0CTL 		; clears timers and dividers
					bis.w	#TBSSEL__SMCLK, &TB0CTL	; choose clock (f = 1 MHz)
					bis.w	#MC__UP, &TB0CTL		; choose mode (UP)

					bis.w	#CNTL_0, &TB0CTL		; choose counter length (N = 2^16)
					bis.w	#ID__4, &TB0CTL			; choose divider for D1 (D1 = 4)
					bis.w 	#TBIDEX__8, &TB0EX0		; choose divider for D2 (D2 = 8)

					; TB0 interrupt: Compare
					mov.w	#32992d, &TB0CCR0		; N = 15625: TB0 @ 0.5sec, N = 32992d for ~0.5Hz
					bis.w	#CCIE, &TB0CCTL0
					bic.w	#CCIFG, &TB0CCTL0

					nop
					eint							; assert global interrupt flag
					nop

					bic.b	#LOCKLPM5, &PM5CTL0		; disable DIO low-power default

					bis.b	#BIT2, &P3OUT			; SCL
					bis.b	#BIT3, &P3OUT			; SDA

main:
					; start condition
					call 	#i2c_start

					; send
					mov.b	transmit_byte, R8		; i2c addr	; mov.b	#055h, R8
					setc	; TODO: if read write bit set/clear C then rotate with carry
					rlc.b 	R8
					call	#i2c_send
					call	#ack
					call	#i2c_stop
					jmp 	main



;-------------------------------------------------------------------------------
; Subroutines
;-------------------------------------------------------------------------------

short_delay:		mov.w	global_short_outer_delay, R4	; sets outer loop delay
					mov.w	global_short_inner_delay, R6
					jmp		inner_loop
delay:				mov.w	global_outer_delay, R4	; sets outer loop delay
					mov.w	global_inner_delay, R6
					jmp 	inner_loop
inner_loop:			mov.w	R6, R5					; sets inner loop delay
					jmp		dec_inner
dec_inner:			dec		R5						; decrements inner delay reg
					jnz		dec_inner				; loop if R5 is not 0
					dec		R4						; decrements outer delay reg
					jnz		inner_loop				; loop if R4 is not 0
					ret



i2c_start:
					; start condition implies that SCL must be high and SDA must change from H->L then SCL->L
					call 	#delay
					bic.b	#BIT3, &P3OUT			; SDA
					call 	#delay
					bic.b	#BIT2, &P3OUT			; SCL
					call 	#delay
					ret


i2c_stop:
					bis.b	#BIT2, &P3OUT			; SCL high
					call	#delay
					bis.b	#BIT3, &P3OUT			; SDA high
					call 	#delay
					call 	#delay
					call 	#delay
					call 	#delay
					call 	#delay
					call 	#delay
					ret



i2c_send:
					call	#i2c_tx_byte
					ret



i2c_tx_byte:		; R8 stores byte data to transmit
					mov.b	#08h, R7				; loop counter (07h sends 7 bits)
for:				dec.b	R7
					mov.b	#080h, R9				; bit mask: 1000 0000 0000 0000b = 080000h, 1000 0000 = 080h
					; send bit
					and.b	R8, R9
					jz		sda_low					; if z=1 then MSB is 0 and dont set SDA
					bis.b	#BIT3, &P3OUT			; SDA high
					jmp		stab_delay				; skip to stability delay
sda_low:			bic.b	#BIT3, &P3OUT			; SDA low
stab_delay:			call	#clock_pulse
					bic.b	#BIT3, &P3OUT			; SDA low
					call	#delay
					; END send bit
					rla.b	R8						; rotate i2c addr
for_cmp:			cmp		#00h, R7				; compare R7 to 0
					jnz		for						; if R7 is not 0 then continue iterating
					; TODO send r/w
					ret



ack:				; this simulates an ACK
					bic.b	#BIT3, &P3OUT			; SDA low
					call	#clock_pulse
					ret



ack:				; this simulates an NACK
					bis.b	#BIT3, &P3OUT			; SDA high
					call	#clock_pulse
					ret



clock_pulse:
					call	#short_delay			; stability delay
					bis.b	#BIT2, &P3OUT			; SCL high
					call 	#delay					; bit delay
					bic.b	#BIT2, &P3OUT			; SCL low
					call	#short_delay			; stability delay
					ret


;-------------------------------------------------------------------------------
; Interrupt Service Routines
;-------------------------------------------------------------------------------

; Service TB0
timer_b0_isr:
					bic.w	#TBIFG, &TB0CTL
					reti
;-------------- END service_TB0 --------------
                                            
;-------------------------------------------------------------------------------
; Memory Allocation
;-------------------------------------------------------------------------------

					.data
					.retain

global_short_outer_delay:		.short	00001h
global_short_inner_delay:		.short  00001h

global_outer_delay:		.short	00005h
global_inner_delay:		.short  00002h

;global_short_outer_delay:	.short	00BD3h
;global_short_inner_delay:	.short  00072h

;global_outer_delay:	.short	00BD3h
;global_inner_delay:	.short  00072h

transmit_byte:		.byte  00068h					; DS3231 Addr: 1101000 = 068h

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
	            	.short	timer_b0_isr
