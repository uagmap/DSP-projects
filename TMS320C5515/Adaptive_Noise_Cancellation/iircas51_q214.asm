;*******************************************************************************
;* FUNCTION NAME: iircas51_q214                                                *
;*                                                                             *
;*   Function Uses Regs : T0,T1,AR0,XAR0,AR1,XAR1,AR2,XAR2,AR3,XAR3,SP,M40,    *
;*                        SATA,SATD,RDM,FRCT,SMUL                              *
;*   Based on DSPLIB iircas51() function									   *
;*   Modified by D. Leon-Salas to accept Q2.14 coefficients                   *
;*******************************************************************************

	.sect	".text"
	.global	_iircas51_q214
_iircas51_q214:

;**********************************************************************
; Program section
;**********************************************************************

	PSH	mmap(ST0_55)
	PSH	mmap(ST1_55)
	PSH	mmap(ST2_55)
	PSH	mmap(ST3_55)

	BCLR	CPL			;DP relative addressing

;	PSH	mmap(DP)
;	MOV	#0, DP
;   ADDED REGARDING THE BUG REPORT, ZHENGTING
	PSH mmap(DP)
	PSH mmap(DPH)
	AMOV #0, XDP

	.dp	0

	PSH	@T3_L
	PSHBOTH	XAR4
	PSHBOTH	XAR7

;initialization
	BSET	SXMD			;sign extension enable
	BCLR	SATD			;D-unit saturate disable
	BSET	FRCT			;fractional mode enable
	BCLR	SATA			;A-unit saturate disable


;setup circular addressing
	MOV	T0, T3				;compute 2*nbiq
	SFTS	T3, #1
	MOV	@T3_L, BK03			;init AR0-3 circular buf size (2*nbiq)
	MOV	@T3_L, BK47			;init AR4-7 circular buf size (2*nbiq)

	BSET	AR3LC			;init AR3 = circular (dbuffer)
	MOV	@AR3_L, BSA23		;init AR2-3 circular start addr: dbuffer(1)

	BSET	AR4LC			;init AR4 = circular (dbuffer)
	MOV	XAR3, XAR4			;adjust AR4 to buffer start
	ADD	T3, AR4
	MOV	@AR4_L, BSA45		;init AR45 circ start addr: dbuffer(1+nbiq)

	MOV	#0, AR3				;init AR3 offset to x(n) buffer start
	MOV	#0, AR4				;init AR4 offset to y(n) buffer start

	SUB	#1, T1, T3			;compute nx-1
	MOV	T3, BRC0			;init outer loop counter (nx-1)

	MOV	XAR1, XAR7			;save original value to reinitialize coeff buffer pntr

	SUB	#1, T0, T3			;init inner loop counter (#bi-quads-1)
	MOV	T3, BRC1

	ADD	#1, T0, T1			;index for buffer reset

; Kernel
;  XAR0: x[] input
;  XAR1: h[] coefficients
;  XAR2: r[] result
;  XAR3: dbuffer[x]
;  XAR4: dbuffer[y]
;  XAR7: reinit XAR1

	RPTBLOCAL	loop1-1				;outer loop: process a new input

	MOV	*AR0+ << #16, AC1			; HI(AC1) = x(n)
	||RPTBLOCAL	loop2-1				;inner loop: process a bi-quad

	MPYM	*AR1+, AC1, AC0			; AC0 = b0*x(n)

	MACM	*AR1+, *(AR3+T0), AC0	; AC0 += b1*x(n-1)

	MACM	*AR1+, *AR3, AC0		; AC0 += b2*x(n-2)

	MOV	HI(AC1), *AR3				; x(n) replaces x(n-2) 
	||AADD	T1, AR3					; point to next x(n-1)

	MASM	*AR1+, *(AR4+T0), AC0	; AC0 -= a0*y(n-1) 

	MASM	*AR1+, *AR4, AC0		; AC0 -= a1*y(n-2) 
	
    SFTS AC0, #1, AC0				; added by D. Leon-Salas (01/20/16) to accept Q2.14 coefficients

	MOV	rnd(HI(AC0)), *AR4			; y(n) replaces y(n-2)
	||AADD	T1, AR4					; point to next y(n-1)

	MOV	AC0, AC1					; input to next biquad

loop2:

	MOV	XAR7, XAR1					; reinitialize coeff pointer

	MOV	rnd(HI(AC0)), *AR2+			; store result to output buffer

loop1:

; Signal overflow
	MOV	#0, T0
	XCC	check1, overflow(AC0)
	MOV	#1, T0
check1:



;Context restore
	POPBOTH	XAR7
	POPBOTH	XAR4
	POP	@T3_L
;	POP	mmap(DP_L)
;   ZHENGTING ADDED
	POP mmap(DPH)
	POP mmap(DP)

	POP	mmap(ST3_55)
	POP	mmap(ST2_55)
	POP	mmap(ST1_55)
	POP	mmap(ST0_55)

	RET	
                                        
                                        