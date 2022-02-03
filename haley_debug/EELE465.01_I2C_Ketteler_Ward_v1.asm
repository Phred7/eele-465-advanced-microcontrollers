;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
; H.Ketteler, W.Ward, EELE465, Project 02
; Jan. 28 2021
; Bit Banging the I2C Protocol
;
;
;--------------------------------Notes------------------------------------------
; SDA is P3.3
; SCL is P3.2
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
; Initialize
;-------------------------------------------------------------------------------
init:
;-- Setup LED1
		bis.b	#BIT0, &P1DIR			; Set P1.0 as an output. 	P1.0 is LED1
		bic.b	#BIT0, &P1OUT			; Clear LED1 initially

;-- Setup LED2
		bis.b	#BIT6, &P6DIR			; Set P6.6 as an output		P6.6 is LED2
		bic.b	#BIT6, &P6OUT			; Clear LED2 initially
		bic.b	#LOCKLPM5, &PM5CTL0		; Disbale the GPIO power-on default high-Z mode

;-- Setup SDA and SCL Ports
		bis.b	#BIT3, &P3DIR			; Set P3.3 as an output for the SDA line
		bic.b	#BIT3, &P3OUT			; Clear SDA line initially
		bis.b	#BIT2, &P3DIR			; Set P3.2 as an output for the SCL line
		bic.b	#BIT2, &P3OUT			; Clear SCL line initially

;-- Setup Timer B0
;		bis.w	#TBCLR, &TB0CTL			; Clear timer and dividers
;		bis.w	#TBSSEL__SMCLK, &TB0CTL	; Select SMCLK as timer source
;		bis.w	#MC__CONTINUOUS, &TB0CTL; Choose continuous counting
;		bis.w	#ID__8, &TB0CTL			; Divide timer by 8
;		bis.w	#TBIDEX__2, &TB0EX0		; Divide Timer by 2
;		bis.w	#TBIE, &TB0CTL			; Enable overflow interrupt
;		bic.w	#TBIFG, &TB0CTL			; Clear Interrupt Flag
;		bis.w	#GIE, SR				; Enable Maskable Interrupts
;--------------------------------End init---------------------------------------


;-------------------------------------------------------------------------------
; Main loop here
;-------------------------------------------------------------------------------
main:
;		call 	#FlashLED1				; Flash LED1
		call	#I2Cstart				; Call I2Cstart Subroutine
		mov.b	#055h, &TransmitByte	; Set variable to slave address
		call 	#I2Csend				; Call I2Csend Subroutine
		mov.w	#08h, R4
		call	#I2CstableDelay
		jmp 	main

;------------------------------End of main--------------------------------------

;-------------------------------------------------------------------------------
; I2C Start - Configure the SDA and SCL lines for the start of an I2C transmission
;-------------------------------------------------------------------------------
I2Cstart:
		bis.b	#BIT3, &P3OUT			; Set P3.3 as an output for SDA line
		bis.b	#BIT2, &P3OUT			; Set P3.2 as an output for SCL line
		ret
;-------------------------------End of I2Cstart---------------------------------

;-------------------------------------------------------------------------------
; I2C Send - Send data from the master to the slave
;-------------------------------------------------------------------------------
I2Csend:
		call #I2CtxByte			; Call I2CtxByte Subroutine
		ret
;-------------------------------End of I2Csend---------------------------------

;-------------------------------------------------------------------------------
; I2CtxByte - Send byte of data (address) to slave
;-------------------------------------------------------------------------------
I2CtxByte:
		mov.b	TransmitByte, R7		; Move data for transmission into R7
		mov.b	#8, R6					; Set number of bits for address (1 more than the actual address)

CheckData:
		sub.b	#01h, R6				; Count number of bits
		tst.b	R6
		jz		TxDone					;
		rla.b	R7						; Move MSB to the carry Flag (will not keep address in R7)
		jc		SetData					; Jump to SetData
		jnc		ClearData				; Jump to ClearData

		jmp		CheckData				; Jump to check next bit

SetData:
		bis.b	#BIT3, &P3OUT			; Set SDA to high
		mov.w	#01h, R4				; Set Delay Loop counter
		call	#I2CstableDelay			; Stable pause

		bis.b	#BIT2, &P3OUT			; Set SCL to high
		mov.w	#01h, R4				; Set Delay Loop counter
		call	#I2CbitDelay				; Short pause


		bic.b	#BIT2, &P3OUT			; Set SCL to low
		mov.w	#01h, R4				; Set Delay Loop counter
		call	#I2CstableDelay			; Stable pause
		bic.b	#BIT3, &P3OUT			; Set SDA to low
		mov.w	#01h, R4				; Set Delay Loop counter
		call	#I2CstableDelay				; Short pause

		jmp		CheckData

ClearData:
		bic.b	#BIT3, &P3OUT			; Set SDA to low
		mov.w	#01h, R4				; Set Delay Loop counter
		call	#I2CstableDelay			; Stable pause

		bis.b	#BIT2, &P3OUT			; Set SCL to high
		mov.w	#01h, R4				; Set Delay Loop counter
		call	#I2CbitDelay				; Short pause

;
		bic.b	#BIT2, &P3OUT			; Set SCL to low
		mov.w	#01h, R4				; Set Delay Loop counter
		call	#I2CstableDelay			; Stable pause
		bic.b	#BIT3, &P3OUT			; Set SDA to low
		mov.w	#01h, R4				; Set Delay Loop counter
		call	#I2CstableDelay				; Short pause

		jmp 	CheckData

TxDone:
		bis.b	#BIT2, &P3OUT			; Set SCL to high
		mov.w	#01h, R4
		call	#I2CstableDelay
		bis.b	#BIT3, &P3OUT
		ret								; Return to I2CSend
;-------------------------------End of I2CtxByte---------------------------------

;-------------------------------------------------------------------------------
; I2CstableDelay - delay between data and clock switches
;-------------------------------------------------------------------------------
I2CstableDelay:
		tst.w	R4				; Check if R4 is zero
		mov.w	#01111h, R5
		jz		StableDelayDone	; Exit subroutine if loop counter is zero
		jnz		StableCountDown		; Jump to CountDown if not zero

StableCountDown:
		dec.w	R5				; Decrement delay counter
		jnz		StableCountDown		; Repeat CountDown until R5 is zero
		dec.w	R4				; Decrement loop counter
;		mov.w	#04444h, R5		; Reset delay counter
		jmp		I2CstableDelay

StableDelayDone:
		ret						; Return to FlashLED1 subroutine

;------------------------End of I2CstableDelay----------------------------------

;-------------------------------------------------------------------------------
; I2CbitDelay - delay between data and clock switches
;-------------------------------------------------------------------------------
I2CbitDelay:
		tst.w	R4				; Check if R4 is zero
		mov.w	#01111h, R5
		jz		BitDelayDone	; Exit subroutine if loop counter is zero
		jnz		BitCountDown		; Jump to CountDown if not zero

BitCountDown:
		dec.w	R5				; Decrement delay counter
		jnz		BitCountDown		; Repeat CountDown until R5 is zero
		dec.w	R4				; Decrement loop counter
;		mov.w	#04444h, R5		; Reset delay counter
		jmp		I2CbitDelay

BitDelayDone:
		ret						; Return to FlashLED1 subroutine

;------------------------End of I2CbitDelay----------------------------------

;-------------------------------------------------------------------------------
; Flash LED1
;-------------------------------------------------------------------------------
FlashLED1:
		xor.b	#01h, P1OUT		; Toggle P1.0 (LED1)
		mov.w 	#20, R4			; Put number of repetitions into R4
		mov.w	#04444h, R5		; Put a large number into R5 for delay timing
		setz					; set Z flag
		call 	#DelayLoop		; Jump to the delay loop
		ret						; Jump to main loop
;------------------------End of FlashLED1---------------------------------------

;-------------------------------------------------------------------------------
; Delay Loop Red LED Heartbeat
;-------------------------------------------------------------------------------
DelayLoop:
		tst.w	R4				; Check if R4 is zero
		jz		done			; Exit subroutine if loop counter is zero
		jnz		CountDown		; Jump to CountDown if not zero

CountDown:
		dec.w	R5				; Decrement delay counter
		jnz		CountDown		; Repeat CountDown until R5 is zero
		dec.w	R4				; Decrement loop counter
		mov.w	#04444h, R5		; Reset delay counter
		jmp		DelayLoop

done:
		ret						; Return to FlashLED1 subroutine

;------------------------End of DelayLoop---------------------------------------


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
            
;-------------------------------------------------------------------------------
; Memory Allocation
;-------------------------------------------------------------------------------
			.data
			.retain

TransmitByte:	.space	1

;--------------------------End of Memory Allocation-----------------------------
