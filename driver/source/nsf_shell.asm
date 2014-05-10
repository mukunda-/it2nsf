;=============================================================================================
	.segment "HEADER"
;=============================================================================================

	.import __CODE_SIZE__
	.import __VECTORS_SIZE__
	.import __VERSION_SIZE__
	
	__ROM_SIZE__ := __CODE_SIZE__ + __VECTORS_SIZE__ + __VERSION_SIZE__

	.import musicInit, musicPlay, musicProcess
	
;=============================================================================================
	.segment "HEADER"
;=============================================================================================
	.byte	"NESM", 1Ah			; marker
	.byte	01h				; version
	.byte	00h				; number of songs
	.byte	00h				; starting song
	.word	__ROM_START__			; load address
	.word	INIT				; init address
	.word	PLAY				; play address
	.byte	"IT-NES DRIVER"			; song name
	.res	32-13, 0			;
	.res	32, 0				; artist
	.res	32, 0				; copyright
	.word	0411Ah				; NTSC speed (60hz)
	.byte	0, 1, 2, 3, 4, 5, 6, 7		; bankswitch init values
	.word	0411Ah				; PAL speed (60hz !)
	.byte	0				; PAL/NTSC bits
	.byte	0				; EXT chip support
	.byte	0, 0, 0, 0			; expansion bytes

;=============================================================================================
	.segment "VECTORS"
;=============================================================================================

__ROM_START__:
	jmp	INIT
	jmp	PLAY
	

	.word	__ROM_SIZE__

;=============================================================================================
; animal
;=============================================================================================
	.byte	"dactylion "
	
	.byte	0, 0, 0, 0, 0, 0
	
;=============================================================================================
	.segment "VERSION"
;=============================================================================================
	.byte	"IT-NES v1.0 "
	
;=============================================================================================
	.code
;=============================================================================================

;=============================================================================================
INIT:
;=============================================================================================
	
	pha
	ldx	#1
	lda	#<(__ROM_START__+__ROM_SIZE__-9000h)
	ldy	#>(__ROM_START__+__ROM_SIZE__-9000h)
	jsr	musicInit
	pla
	jmp	musicPlay

;=============================================================================================
PLAY:
;=============================================================================================
	jmp	musicProcess
