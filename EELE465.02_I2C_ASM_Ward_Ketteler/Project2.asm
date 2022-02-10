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
;	R7 - loop counter
;	R8 - transmit byte
;	R9 - transmit byte
;	R10 - recieve byte
;	R11
;	R12 - Represents the state of the last acknowledgment recieved via I2C. 00h == ACK. 01h == NACK.
;	R13 - Contains seconds val. from mem for ease of viewing
;	R14 - Contains minutes val. from mem for ease of viewing
;	R15 - Contains hours val. from mem for ease of viewing
;-------------------------------------------------------------------------------
init:
					bis.b	#BIT2, &P3DIR			; set P3.2 as out for SCL
					bic.b	#BIT2, &P3OUT			; set init val to 0

					bis.b	#BIT3, &P3DIR			; set P3.3 as out for SDA
					bic.b	#BIT3, &P3OUT			; set init val to 0

					bic.b	#LOCKLPM5, &PM5CTL0		; disable DIO low-power default

main:
					; 1st packet: RTC Addr+W and Register Addr.
					call 	#i2c_start				; start condition
					mov.b	transmit_byte, R8		; i2c addr into R8
					clrc							; read/write bit set/clear C then rotate with carry
					rlc.b 	R8						; rotate R8 to append W/R bit to end
					call	#i2c_send				; send addr
					tst.b	R12						; tests to see if the MSP recieved a NACK
					jnz		nak_ed					; if the MSP recieved a NACK jump
					mov.b	seconds_register, R8	; mov the value of the seconds_register into R8
					call 	#i2c_send				; send seconds register addr
					tst.b	R12						; tests to see if the MSP recieved a NACK
					jnz		nak_ed					; if the MSP recieved a NACK jump
					call	#i2c_stop				; sends i2c stop condition (same as re-start condition but should only happen after an ACK. If a NACK resend)

					; 2nd packet: RTC Addr+R and data.
;main:
					call	#i2c_start
					mov.b	transmit_byte, R8		; i2c addr into R8
					setc							; read/write bit set/clear C then rotate with carry
					rlc.b 	R8						; rotate R8 to append W/R bit to end
					call	#i2c_send				; send addr
					tst.b	R12						; tests to see if the MSP recieved a NACK
					jnz		nak_ed					; if the MSP recieved a NACK jump
					call	#i2c_recieve
					call	#i2c_stop				; sends i2c stop condition (same as re-start condition but should only happen after an ACK. If a NACK resend)

					mov.b	seconds, R13
					mov.b	minutes, R14
					mov.b	hours, R15

					jmp 	main
					nop
nak_ed:
					call	#i2c_stop
					jmp 	main
					nop

;-------------------------------------------------------------------------------
; Subroutines
;-------------------------------------------------------------------------------

; Delay
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


; I2C Start Condition
i2c_start:
					; start condition implies that SCL must be high and SDA must change from H->L then SCL->L
					call 	#delay
					bic.b	#BIT3, &P3OUT			; SDA
					call 	#delay
					bic.b	#BIT2, &P3OUT			; SCL
					call 	#delay
					ret


; I2C Stop Condition
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


; I2C Send
i2c_send:
					call	#i2c_tx_byte
					call 	#recieve_ack
					ret


; I2C Tx Byte
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


; I2C Recieve Acknowledgement
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
recieved_finally:	call 	#clock_pulse_half
					bis.b	#BIT3, &P3DIR			; SDA as output
					bic.b	#BIT3, &P3OUT			; SDA low
					call	#short_delay
					ret


; I2C Recieve
i2c_recieve:
					; recieve 3 bytes, write each to mem, send acks and nacks when applicable
					call	#i2c_rx_byte
					mov.b	R10, seconds
					call 	#ack
					call	#i2c_rx_byte
					mov.b	R10, minutes
					call 	#ack
					call	#i2c_rx_byte
					mov.b	R10, hours
					call 	#nack
					ret


; I2C Rx Byte
i2c_rx_byte:
					mov.b	#08h, R7				; loop counter (08h sends 8 bits)
					mov.w	#00h, R10
					bic.b	#BIT3, &P3DIR
rx_byte_for:		dec.b	R7
					call	#short_delay			; stability delay
					bis.b	#BIT2, &P3OUT			; SCL high
					bit.b	#BIT3, &P3IN			; tests the value of P3.3. If z=1 we got a 0, if z=0 we got an 1
					jz		rx_zero
rx_one:				setc
					jmp		rx_delay
rx_zero:			clrc
rx_delay:			rlc.b	R10
					call 	#clock_pulse_half
					call	#delay
					cmp		#00h, R7				; compare R7 to 0
					jnz		rx_byte_for				; if R7 is not 0 then continue iterating
					bis.b	#BIT3, &P3DIR
					ret

; ACK
ack:				; this simulates an ACK
					bic.b	#BIT3, &P3OUT			; SDA low
					call	#clock_pulse
					ret


; NACK
nack:				; this simulates an NACK
					bis.b	#BIT3, &P3OUT			; SDA high
					call	#clock_pulse
					bic.b	#BIT3, &P3OUT			; SDA low
					ret


; SCL Pulse
clock_pulse:
					call	#short_delay			; stability delay
					bis.b	#BIT2, &P3OUT			; SCL high
					call 	#delay					; bit delay
					bic.b	#BIT2, &P3OUT			; SCL low
					call	#short_delay			; stability delay
					ret


; SCL half pulse
clock_pulse_half:
					call 	#delay					; bit delay
					bic.b	#BIT2, &P3OUT			; SCL low
					call	#short_delay			; stability delay
					ret
                                            
;-------------------------------------------------------------------------------
; Memory Allocation
;-------------------------------------------------------------------------------

					.data
					.retain

global_short_outer_delay:		.short	00001h
global_short_inner_delay:		.short  00001h

global_outer_delay:		.short	000005h
global_inner_delay:		.short  00002h

;global_short_outer_delay:	.short	00BD3h
;global_short_inner_delay:	.short  00072h

;global_outer_delay:	.short	00BD3h
;global_inner_delay:	.short  00072h

transmit_byte:		.byte  	00068h					; DS1337 Addr: 1101000 = 068h
seconds_register: 	.byte	00000h					; Seconds register address on the DS1337
seconds:			.byte	00000h
minutes:			.byte	00000h
hours:				.byte	00000h

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
