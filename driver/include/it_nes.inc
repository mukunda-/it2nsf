.ifndef IT_NES_DEFINED
IT_NES_DEFINED = 1

	;--------------------------------------------
	; variable: current song position
	;--------------------------------------------
	.import ITNES_Music_Position
	
	;--------------------------------------------
	; start music
	;
	; a = song index
	; y = starting position
	;--------------------------------------------
	.import Music_Play
	
	;--------------------------------------------
	; stop music
	;--------------------------------------------
	.import Music_Stop	
	
	;--------------------------------------------
	; update routine (60HZ)
	;--------------------------------------------
	.import Music_Process

.endif
