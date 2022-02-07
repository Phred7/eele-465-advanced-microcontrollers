;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
; Walker Ward
; Project 2: Custom I2C protocol
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
;	R4 - delay subroutine reserved
;	R5 - delay subroutine reserved
;	R6 - delay subroutine reserved
;	R7 - transmit byte
;	R8 - transmit byte
;	R9 - transmit byte
;	R10
;	R11
;	R12 - test what kind of ack was returned by the RTC
;	R13
;	R14
;	R15
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

					; send addr and data
					mov.b	transmit_byte, R8		; i2c addr	; mov.b	#055h, R8
					clrc							; TODO: if read/write bit set/clear C then rotate with carry
					rlc.b 	R8
					call	#i2c_send
					tst.b	R12						; tests to see if the MSP recieved a NACK
					jnz		nak_ed					; if the MSP recieved a NACK

					; send data (0-9)
					;mov.b 	#00h, R11
					;mov.b	#0Ah, R10				; loop counter (08h sends 8 bits)
;for:				dec.b	R10
					;mov.b	R11, R8
					;call	#i2c_send
					;tst.b	R12						; tests to see if the MSP recieved a NACK
					;jnz		nak_ed				; if the MSP recieved a NACK
					;add.b	#01h, R11				; add 1 to send data
					;cmp		#00h, R10				; compare R10 to 0
					;jnz		for						; if R10 is not 0 then continue iterating

					; stop condition
					call	#i2c_stop				; sends i2c stop condition (same as re-start condition but should only happen after an ACK. If a NACK resend)

					jmp 	main
					nop

nak_ed:
					call	#i2c_stop
					jmp 	main
					nop

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
					call 	#delay					; extra delay to seperate packets
					call 	#delay
					call 	#delay
					call 	#delay
					call 	#delay
					call 	#delay
					ret



i2c_send:
					call	#i2c_tx_byte
					;call	#ack					; simulates recieving an ACK
					call 	#recieve_ack
					ret



i2c_tx_byte:		; R8 stores byte data to transmit
					mov.b	#08h, R7				; loop counter (08h sends 8 bits)
tx_byte_for:		dec.b	R7
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
					rla.b	R8						; rotate byte to next bit
tx_byte_for_cmp:	cmp		#00h, R7				; compare R7 to 0
					jnz		tx_byte_for				; if R7 is not 0 then continue iterating
					ret



recieve_ack:
					bis.b	#BIT3, &P3OUT			; SDA high
					bic.b	#BIT3, &P3DIR			; SDA as input
					call	#short_delay			; stability delay
					bis.b	#BIT2, &P3OUT			; SCL high
					; check ack/nack
					bit.b	#BIT3, &P3IN			; tests the value of P3.3. If z=1 we got a nack, if z=0 we got an ack
					jz		recieved_ack
recieved_nack:		mov.b	#01h, R12
					jmp 	recieved_finally
recieved_ack:		mov.b	#00h, R12
recieved_finally:	call 	#delay					; bit delay
					bic.b	#BIT2, &P3OUT			; SCL low
					call	#short_delay			; stability delay
					bis.b	#BIT3, &P3DIR			; SDA as output
					bic.b	#BIT3, &P3OUT			; SDA low
					ret



ack:				; this simulates an ACK
					bic.b	#BIT3, &P3OUT			; SDA low
					call	#clock_pulse
					ret



nack:				; this simulates an NACK
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
