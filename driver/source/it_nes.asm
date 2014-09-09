;************************************************************
;* IT-NES SOUND DRIVER
;*
;* Copyright (c) 2009, Mukunda Johnson (www.mukunda.com)
;* All rights reserved.
;*
;* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;*
;* 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;*
;* 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;*
;* 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;*
;* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
;* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
;* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
;* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;************************************************************

;=============================================================================================
; ASSEMBLY SWITCHES
;=============================================================================================

BANKSWITCHING = 1		; NON-ZERO FOR BANKSWITCHING MODE
EXTCHIP = 1			; EXTENSION CHIPS

;=============================================================================================
; global functions
;=============================================================================================

	.export musicInit
	.export musicPlay
	.export musicProcess

;=============================================================================================
; DEFINITIONS
;=============================================================================================

	;bankswitched layout:
	;--------------------------
	; 8000-8FFF = program bank
	; 9000-9FFF = program bank 2
	; A000-AFFF = not used
	; B000-CFFF = not used
	; C000-DFFF = random data access
	; E000-FFFF = DPCM data access
	; (E000-FFFF mapping must not be changed outside of music routine)
	;--------------------------
	
	; bank controls
	BCTRL8	:=$5FF8
	BCTRL9	:=$5FF9
	BCTRLC	:=$5FFC
	BCTRLD	:=$5FFD
	BCTRLE	:=$5FFE
	BCTRLF	:=$5FFF

NCHANNELS = 16
TEMPO_PRECISION = 4096

;---------------------------------------------------------------------------------------------
; soundbank data offsets
;---------------------------------------------------------------------------------------------
sbt_modcount		= 0
sbt_mtable		= 1

sbm_itempo		= 0
sbm_ispeed		= 1
sbm_ivolume		= 2
sbm_pattcount		= 3
sbm_inscount		= 4
sbm_length		= 5
sbm_chcount		= 6
sbm_expchips		= 7
sbm_n106wt		= 10h
sbm_chmap		= 50h
sbm_tables		= 60h

sbi_type		=0
sbi_dvolume		=1
sbi_pbase		=2
sbi_pbase2		=3
sbi_mdelay		=4
sbi_msweep		=5
sbi_mdepth		=6
sbi_mrate		=7
sbi_envelopemask	=8
sbi_envelopes		=25

sbi_dpcmbank		=9
sbi_dpcmoffset		=10
sbi_dpcmlength		=11
sbi_dpcmloop		=12

sbi_vrc7custom		=9

sbi_noiseloop		=9

; sum macros

.macro resb labelname
labelname: .res 1
.endmacro

.macro resw labelname
labelname: .res 2
.endmacro

.macro res labelname, size
labelname: .res size
.endmacro

.macro CHVAR labelname
labelname: .res NCHANNELS
.endmacro

.macro lsr_x4
	lsr
	lsr
	lsr
	lsr
.endmacro

;=============================================================================================
; MEMORY
;=============================================================================================

;---------------------------------------------------------------------------------------------
	.zeropage
;---------------------------------------------------------------------------------------------

;---------------------------------------------------------------------------------------------
; general purpose memory
;---------------------------------------------------------------------------------------------
	resb	var1
	resb	var2
	resb	var3
	resb	var4
	resb	var5
	resb	var6
	resb	var7
	resb	var8
	
;---------------------------------------------------------------------------------------------
; fast access variables
;---------------------------------------------------------------------------------------------
	CHVAR ch_param			; command parameter
	CHVAR ch_cmem			; command memory
	resb	mod_tick		; song tick counter

;---------------------------------------------------------------------------------------------
; intermediate data
;---------------------------------------------------------------------------------------------
	resw	t_pitch		; temporary pitch (for imminent update)
	resb	t_volume	;
	resb	t_flags		;
	resb	t_duty_env	;

;---------------------------------------------------------------------------------------------
; other pointers
;---------------------------------------------------------------------------------------------
	resw	ptr_mtable	; module table (top level)
	resw	ptr_itableL	; instrument table
	resw	ptr_itableH
	resw	ptr_itableB
	resw	ptr_ptableL	; pattern table
	resw	ptr_ptableH
	resw	ptr_ptableB
	resw	ptr_sequence	; order list
	resw	ptr_pattern	; pattern data pointer

;---------------------------------------------------------------------------------------------
	.bss
;---------------------------------------------------------------------------------------------

;---------------------------------------------------------------------------------------------
; channel data
;---------------------------------------------------------------------------------------------

	CHVAR ch_pitch_l		; 16-bit pitch
	CHVAR ch_pitch_h		;
	CHVAR ch_instrument		; instrument index
	CHVAR ch_vcmd			; volume command
	CHVAR ch_command		; command index
	CHVAR ch_volume			; volume level
	CHVAR ch_note			; pattern note
	CHVAR ch_flags			; flags
	
	CHVAR ch_uflags			; &1 = pitch change, &2 = note off, &4 = modulation, &64 = note on, &128 = sustain
	CHVAR ch_upitch_l		; final pitch = this + envelope + modulation
	CHVAR ch_upitch_h		; 
	CHVAR ch_uvolume		;  final volume = this + envelope
	CHVAR ch_mod			; modulation variable
	
	CHVAR ch_env_vol		; envelope positions
	CHVAR ch_env_pitch		;
	CHVAR ch_env_duty		;
	
	CHVAR ch_msweep			; instrument modulation
	CHVAR ch_mdepth			;
	CHVAR ch_mpos			;
	
	UF_PITCH = 1
	UF_NOTEOFF = 2
	UF_MOD = 4
	UF_DELAY = 8
	UF_INIT = 16 ; (do not update audio unless this bit is set)
	UF_NOTEON = 64
	UF_SUSTAIN = 128
	
;---------------------------------------------------------------------------------------------

	resw	mod_tempo
	resb	mod_bpm
	resb	mod_speed
	resb	mod_volume	; global volume
	resb	modtab_bank	; bank where module table resides
	resb	mod_bank	; bank where the current module's info resides
	resb	mod_chcount
	resb	mod_length
	resb	mod_index
	resb	mod_expchips
	resb	pattern_bank
	resb	pattern_rows
	
	resb	fds_instrument
	
	res	mod_chmap, 16
	
	resw	pattern_update_flags

	.export ITNES_Music_Position
ITNES_Music_Position:		;(external access):
	resb	mod_position	; song position
	
;	resb	mod_pattreado	; pattern read offset (storage during update) TODO ?

;	resb	mod_channelptr	; channel counter

;	resb	mod_endofrow	; end of row flag

;---------------------------------------------------------------------------------------------
; pattern loop variables
;---------------------------------------------------------------------------------------------
	resb	mod_ploop_row	; MSb = enable jump
	resb	mod_ploop_num	
	resb	mod_ploop_adr

;---------------------------------------------------------------------------------------------
; pattern delay variable
;---------------------------------------------------------------------------------------------
	resb	mod_pdelay

;---------------------------------------------------------------------------------------------
; pattern jump variables
;---------------------------------------------------------------------------------------------
	resb	mod_pjump
	resb	mod_pjumpe
;	resb	mod_pjumpr ( not supported )

;---------------------------------------------------------------------------------------------
; channel iteration counter
;---------------------------------------------------------------------------------------------
	resb	mod_ccounter

;---------------------------------------------------------------------------------------------
; is playing flag
;---------------------------------------------------------------------------------------------
	resb	mod_playing

;---------------------------------------------------------------------------------------------
; position variables
;---------------------------------------------------------------------------------------------
	resb	mod_row		; song row position
	resb	mod_timer	; song timer (8.8 fixed)
	resb	mod_timersk	; timer skip enable
	resb	mod_timerv	; timer speed LO

;---------------------------------------------------------------------------------------------
; apu shadow data
;---------------------------------------------------------------------------------------------
	resb	apu_shadow_pulse1
	resb	apu_shadow_mmc51
	resb	__reserved2
	resb	__reserved3
	resb	apu_shadow_pulse2
	resb	apu_shadow_mmc52
	resb	__reserved5
	resb	__reserved6

cfxpG	=1	; shared with e,f
cfxpVS	=11	; vibrato-speed
cfxpVD	=12	; vibrato-depth
	
;---------------------------------------------------------------------------------------------
; channel flags
;---------------------------------------------------------------------------------------------
cf_note		=1
cf_instr	=2
cf_vol		=4
cf_cmd		=8

;---------------------------------------------------------------------------------------------
commandMemory:
;---------------------------------------------------------------------------------------------
	.res	NCHANNELS*8

;=============================================================================================
	.code
;=============================================================================================

;---------------------------------------------------------------------------------------------
; SWITCHES
;---------------------------------------------------------------------------------------------


;---------------------------------------------------------------------------------------------
; TABLES
;---------------------------------------------------------------------------------------------

;---------------------------------------------------------------------------------------------
commandRoutinesL:
;---------------------------------------------------------------------------------------------
	.byte 	<commandUnused,			<commandSetSpeed
	.byte	<commandSetPosition,		<commandPatternBreak
	.byte	<commandVolumeSlide,		<commandPitchSlideDown
	.byte	<commandPitchSlideUp,		<commandGlissando
	.byte	<commandVibrato,		<commandTremor
	.byte	<commandArpeggio,		<commandVolumeSlideVibrato
	.byte	<commandVolumeSlideGlis,	<commandUnused
	.byte	<commandUnused,			<commandUnused
	.byte	<commandUnused,			<commandRetriggerNote
	.byte	<commandTremolo,		<commandExtended
	.byte	<commandChangeTempo,		<commandFineVibrato
	.byte	<commandSetGlobalVolume,	<commandUnused
	.byte	<commandUnused,			<commandUnused
	.byte	<commandUnused
	
;---------------------------------------------------------------------------------------------
commandRoutinesH:
;---------------------------------------------------------------------------------------------
	.byte 	>commandUnused,			>commandSetSpeed
	.byte	>commandSetPosition,		>commandPatternBreak
	.byte	>commandVolumeSlide,		>commandPitchSlideDown
	.byte	>commandPitchSlideUp,		>commandGlissando
	.byte	>commandVibrato,		>commandTremor
	.byte	>commandArpeggio,		>commandVolumeSlideVibrato
	.byte	>commandVolumeSlideGlis,	>commandUnused
	.byte	>commandUnused,			>commandUnused
	.byte	>commandUnused,			>commandRetriggerNote
	.byte	>commandTremolo,		>commandExtended
	.byte	>commandChangeTempo,		>commandFineVibrato
	.byte	>commandSetGlobalVolume,	>commandUnused
	.byte	>commandUnused,			>commandUnused
	.byte	>commandUnused
	
;---------------------------------------------------------------------------------------------
xcommandRoutinesL:
;---------------------------------------------------------------------------------------------
	.byte	<xcommandUnused,	<xcommandUnused
	.byte	<xcommandUnused,	<xcommandUnused
	.byte	<xcommandUnused,	<xcommandUnused
	.byte	<xcommandUnused,	<xcommandUnused
	.byte	<xcommandUnused,	<xcommandUnused
	.byte	<xcommandUnused,	<xcommandUnused
	.byte	<xcommandNoteCut,	<xcommandNoteDelay
	.byte	<xcommandPatternDelay,	<xcommandUnused
	
;---------------------------------------------------------------------------------------------
xcommandRoutinesH:
;---------------------------------------------------------------------------------------------
	.byte	>xcommandUnused,	>xcommandUnused
	.byte	>xcommandUnused,	>xcommandUnused
	.byte	>xcommandUnused,	>xcommandUnused
	.byte	>xcommandUnused,	>xcommandUnused
	.byte	>xcommandUnused,	>xcommandUnused
	.byte	>xcommandUnused,	>xcommandUnused
	.byte	>xcommandNoteCut,	>xcommandNoteDelay
	.byte	>xcommandPatternDelay,	>xcommandUnused
	
;---------------------------------------------------------------------------------------------
audioRoutinesL:
;---------------------------------------------------------------------------------------------
	.byte	<AR_2A031,	<AR_2A032,	<AR_2A03T,	<AR_2A03N
	.byte	<AR_2A03D,	<AR_VRC61,	<AR_VRC62,	<AR_VRC6S
	.byte	<AR_N106x,	<AR_N106x,	<AR_N106x,	<AR_N106x
	.byte	<AR_N106x,	<AR_N106x,	<AR_N106x,	<AR_N106x
	.byte	<AR_VRC7x,	<AR_VRC7x,	<AR_VRC7x,	<AR_VRC7x
	.byte	<AR_VRC7x,	<AR_VRC7x,	<AR_MMC51,	<AR_MMC52
	.byte	<AR_FME71,	<AR_FME72,	<AR_FME73,	<AR_FDSEX
	
;---------------------------------------------------------------------------------------------
audioRoutinesH:
;---------------------------------------------------------------------------------------------
	.byte	>AR_2A031,	>AR_2A032,	>AR_2A03T,	>AR_2A03N
	.byte	>AR_2A03D,	>AR_VRC61,	>AR_VRC62,	>AR_VRC6S
	.byte	>AR_N106x,	>AR_N106x,	>AR_N106x,	>AR_N106x
	.byte	>AR_N106x,	>AR_N106x,	>AR_N106x,	>AR_N106x
	.byte	>AR_VRC7x,	>AR_VRC7x,	>AR_VRC7x,	>AR_VRC7x
	.byte	>AR_VRC7x,	>AR_VRC7x,	>AR_MMC51,	>AR_MMC52
	.byte	>AR_FME71,	>AR_FME72,	>AR_FME73,	>AR_FDSEX


;---------------------------------------------------------------------------------------------
vibratoTable:
;---------------------------------------------------------------------------------------------
	;.byte 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h ; position
	.byte 00h, 00h, 01h, 01h, 02h, 02h, 02h, 03h, 03h, 04h, 04h, 04h, 05h, 05h, 05h, 06h ;  |
	.byte 00h, 01h, 02h, 02h, 03h, 04h, 05h, 05h, 06h, 07h, 08h, 09h, 09h, 0Ah, 0Bh, 0Ch ;  |
	.byte 00h, 01h, 02h, 03h, 05h, 06h, 07h, 08h, 09h, 0Ah, 0Ch, 0Dh, 0Eh, 0Fh, 10h, 11h ;  |
	.byte 00h, 02h, 03h, 05h, 06h, 08h, 09h, 0Bh, 0Ch, 0Eh, 0Fh, 11h, 12h, 14h, 15h, 17h ;  |
	.byte 00h, 02h, 04h, 06h, 08h, 09h, 0Bh, 0Dh, 0Fh, 11h, 13h, 15h, 17h, 19h, 1Ah, 1Ch ;  V
	.byte 00h, 02h, 04h, 07h, 09h, 0Bh, 0Dh, 10h, 12h, 14h, 16h, 18h, 1Bh, 1Dh, 1Fh, 21h
	.byte 00h, 03h, 05h, 08h, 0Ah, 0Dh, 0Fh, 12h, 14h, 17h, 19h, 1Ch, 1Eh, 21h, 24h, 26h
	.byte 00h, 03h, 06h, 08h, 0Bh, 0Eh, 11h, 14h, 17h, 19h, 1Ch, 1Fh, 22h, 25h, 28h, 2Ah
	.byte 00h, 03h, 06h, 09h, 0Ch, 0Fh, 13h, 16h, 19h, 1Ch, 1Fh, 22h, 25h, 28h, 2Bh, 2Eh
	.byte 00h, 03h, 07h, 0Ah, 0Dh, 11h, 14h, 17h, 1Bh, 1Eh, 21h, 25h, 28h, 2Bh, 2Fh, 32h
	.byte 00h, 04h, 07h, 0Bh, 0Eh, 12h, 15h, 19h, 1Ch, 20h, 23h, 27h, 2Ah, 2Eh, 31h, 35h
	.byte 00h, 04h, 07h, 0Bh, 0Fh, 12h, 16h, 1Ah, 1Eh, 21h, 25h, 29h, 2Ch, 30h, 34h, 37h
	.byte 00h, 04h, 08h, 0Bh, 0Fh, 13h, 17h, 1Bh, 1Fh, 22h, 26h, 2Ah, 2Eh, 32h, 36h, 39h
	.byte 00h, 04h, 08h, 0Ch, 10h, 14h, 18h, 1Bh, 1Fh, 23h, 27h, 2Bh, 2Fh, 33h, 37h, 3Bh
	.byte 00h, 04h, 08h, 0Ch, 10h, 14h, 18h, 1Ch, 20h, 24h, 28h, 2Ch, 30h, 34h, 38h, 3Ch
	; depth ---------->

;---------------------------------------------------------------------------------------------
Table_1_ASL_n:
;---------------------------------------------------------------------------------------------
	.byte	01h, 02h, 04h, 08h, 10h, 20h, 40h, 80h
	
;---------------------------------------------------------------------------------------------
Table_1_ASL_n_mask:
;---------------------------------------------------------------------------------------------
	.byte	0FEh, 0FCh, 0F8h, 0F0h, 0E0h, 0C0h, 080h ;,000h
	
;---------------------------------------------------------------------------------------------
Table_mul16:
;---------------------------------------------------------------------------------------------
	.byte	00h, 10h, 20h, 30h, 40h, 50h, 60h, 70h, 80h, 90h, 0A0h, 0B0h, 0C0h, 0D0h, 0E0h, 0F0h
	
;---------------------------------------------------------------------------------------------
frequencyTableL: ; ftab[0..192] = (32767/2) * 2^(i/192)
;---------------------------------------------------------------------------------------------
	.byte 0FFh, 03Ah, 076h, 0B1h, 0EDh, 029h, 066h, 0A2h, 0DFh, 01Ch, 059h, 097h, 0D4h, 012h, 050h, 08Fh
	.byte 0CDh, 00Ch, 04Bh, 08Ah, 0CAh, 009h, 049h, 089h, 0CAh, 00Ah, 04Bh, 08Ch, 0CEh, 00Fh, 051h, 093h
	.byte 0D5h, 018h, 05Bh, 09Eh, 0E1h, 024h, 068h, 0ACh, 0F0h, 035h, 079h, 0BEh, 003h, 049h, 08Fh, 0D5h
	.byte 01Bh, 061h, 0A8h, 0EFh, 036h, 07Eh, 0C5h, 00Dh, 056h, 09Eh, 0E7h, 030h, 079h, 0C3h, 00Dh, 057h
	.byte 0A1h, 0ECh, 037h, 082h, 0CEh, 019h, 065h, 0B2h, 0FEh, 04Bh, 098h, 0E5h, 033h, 081h, 0CFh, 01Eh
	.byte 06Dh, 0BCh, 00Bh, 05Bh, 0ABh, 0FBh, 04Ch, 09Ch, 0EDh, 03Fh, 091h, 0E3h, 035h, 087h, 0DAh, 02Eh
	.byte 081h, 0D5h, 029h, 07Dh, 0D2h, 027h, 07Ch, 0D2h, 028h, 07Eh, 0D5h, 02Ch, 083h, 0DAh, 032h, 08Ah
	.byte 0E3h, 03Ch, 095h, 0EEh, 048h, 0A2h, 0FCh, 057h, 0B2h, 00Dh, 069h, 0C5h, 022h, 07Eh, 0DBh, 039h
	.byte 096h, 0F4h, 053h, 0B2h, 011h, 070h, 0D0h, 030h, 090h, 0F1h, 052h, 0B4h, 016h, 078h, 0DBh, 03Eh
	.byte 0A1h, 004h, 068h, 0CDh, 032h, 097h, 0FCh, 062h, 0C8h, 02Fh, 096h, 0FDh, 065h, 0CDh, 035h, 09Eh
	.byte 007h, 071h, 0DBh, 045h, 0B0h, 01Bh, 086h, 0F2h, 05Fh, 0CBh, 038h, 0A6h, 014h, 082h, 0F0h, 060h
	.byte 0CFh, 03Fh, 0AFh, 020h, 091h, 002h, 074h, 0E7h, 059h, 0CCh, 040h, 0B4h, 028h, 09Dh, 012h, 088h
	.byte 0FEh
;---------------------------------------------------------------------------------------------
frequencyTableH:
;---------------------------------------------------------------------------------------------
	.byte 3Fh, 40h, 40h, 40h, 40h, 41h, 41h, 41h, 41h, 42h, 42h, 42h, 42h, 43h, 43h, 43h
	.byte 43h, 44h, 44h, 44h, 44h, 45h, 45h, 45h, 45h, 46h, 46h, 46h, 46h, 47h, 47h, 47h
	.byte 47h, 48h, 48h, 48h, 48h, 49h, 49h, 49h, 49h, 4Ah, 4Ah, 4Ah, 4Bh, 4Bh, 4Bh, 4Bh
	.byte 4Ch, 4Ch, 4Ch, 4Ch, 4Dh, 4Dh, 4Dh, 4Eh, 4Eh, 4Eh, 4Eh, 4Fh, 4Fh, 4Fh, 50h, 50h
	.byte 50h, 50h, 51h, 51h, 51h, 52h, 52h, 52h, 52h, 53h, 53h, 53h, 54h, 54h, 54h, 55h
	.byte 55h, 55h, 56h, 56h, 56h, 56h, 57h, 57h, 57h, 58h, 58h, 58h, 59h, 59h, 59h, 5Ah
	.byte 5Ah, 5Ah, 5Bh, 5Bh, 5Bh, 5Ch, 5Ch, 5Ch, 5Dh, 5Dh, 5Dh, 5Eh, 5Eh, 5Eh, 5Fh, 5Fh
	.byte 5Fh, 60h, 60h, 60h, 61h, 61h, 61h, 62h, 62h, 63h, 63h, 63h, 64h, 64h, 64h, 65h
	.byte 65h, 65h, 66h, 66h, 67h, 67h, 67h, 68h, 68h, 68h, 69h, 69h, 6Ah, 6Ah, 6Ah, 6Bh
	.byte 6Bh, 6Ch, 6Ch, 6Ch, 6Dh, 6Dh, 6Dh, 6Eh, 6Eh, 6Fh, 6Fh, 6Fh, 70h, 70h, 71h, 71h
	.byte 72h, 72h, 72h, 73h, 73h, 74h, 74h, 74h, 75h, 75h, 76h, 76h, 77h, 77h, 77h, 78h
	.byte 78h, 79h, 79h, 7Ah, 7Ah, 7Bh, 7Bh, 7Bh, 7Ch, 7Ch, 7Dh, 7Dh, 7Eh, 7Eh, 7Fh, 7Fh
	.byte 7Fh

;--------------------------------------------------------------------------------------------
tempoTableL:
;--------------------------------------------------------------------------------------------
	.byte 000h, 0BAh, 097h, 092h, 0ABh, 0DDh, 028h, 08Ah, 000h, 089h, 025h, 0D0h, 08Ch, 055h, 02Dh, 010h
	.byte 000h, 0FBh, 000h, 00Fh, 027h, 048h, 072h, 0A3h, 0DBh, 01Bh, 061h, 0AEh, 000h, 058h, 0B6h, 018h
	.byte 080h, 0ECh, 05Dh, 0D2h, 04Bh, 0C8h, 049h, 0CEh, 055h, 0E0h, 06Fh, 000h, 094h, 02Bh, 0C5h, 061h
	.byte 000h, 0A1h, 045h, 0EAh, 092h, 03Ch, 0E8h, 096h, 046h, 0F7h, 0ABh, 060h, 016h, 0CEh, 088h, 043h
	.byte 000h, 0BEh, 07Dh, 03Eh, 000h, 0C3h, 088h, 04Dh, 014h, 0DBh, 0A4h, 06Eh, 039h, 005h, 0D1h, 09Fh
	.byte 06Eh, 03Dh, 00Dh, 0DFh, 0B1h, 083h, 057h, 02Bh, 000h, 0D6h, 0ACh, 083h, 05Bh, 033h, 00Ch, 0E6h
	.byte 0C0h, 09Bh, 076h, 052h, 02Fh, 00Ch, 0E9h, 0C7h, 0A6h, 085h, 064h, 044h, 025h, 005h, 0E7h, 0C9h
	.byte 0ABh, 08Dh, 070h, 054h, 037h, 01Bh, 000h, 0E5h, 0CAh, 0B0h, 096h, 07Ch, 062h, 049h, 031h, 018h
	.byte 000h, 0E8h, 0D1h, 0B9h, 0A2h, 08Ch, 075h, 05Fh, 049h, 034h, 01Eh, 009h, 0F4h, 0DFh, 0CBh, 0B7h
	.byte 0A3h, 08Fh, 07Ch, 068h, 055h, 042h, 030h, 01Dh, 00Bh, 0F9h, 0E7h, 0D6h, 0C4h, 0B3h, 0A2h, 091h
	.byte 080h, 06Fh, 05Fh, 04Fh, 03Fh, 02Fh, 01Fh, 00Fh, 000h, 0F1h, 0E2h, 0D3h, 0C4h, 0B5h, 0A7h, 098h
	.byte 08Ah, 07Ch, 06Eh, 060h, 052h, 045h, 037h, 02Ah, 01Ch, 00Fh, 002h, 0F5h, 0E9h, 0DCh, 0D0h, 0C3h
	.byte 0B7h, 0ABh, 09Fh, 093h, 087h, 07Bh, 06Fh, 064h, 058h, 04Dh, 042h, 036h, 02Bh, 020h, 016h, 00Bh
	.byte 000h, 0F5h, 0EBh, 0E0h, 0D6h, 0CCh, 0C2h, 0B7h, 0ADh, 0A3h, 09Ah, 090h, 086h, 07Ch, 073h, 069h
;--------------------------------------------------------------------------------------------
tempoTableH:
;--------------------------------------------------------------------------------------------
	.byte 04Bh, 048h, 046h, 044h, 042h, 040h, 03Fh, 03Dh, 03Ch, 03Ah, 039h, 037h, 036h, 035h, 034h, 033h
	.byte 032h, 030h, 030h, 02Fh, 02Eh, 02Dh, 02Ch, 02Bh, 02Ah, 02Ah, 029h, 028h, 028h, 027h, 026h, 026h
	.byte 025h, 024h, 024h, 023h, 023h, 022h, 022h, 021h, 021h, 020h, 020h, 020h, 01Fh, 01Fh, 01Eh, 01Eh
	.byte 01Eh, 01Dh, 01Dh, 01Ch, 01Ch, 01Ch, 01Bh, 01Bh, 01Bh, 01Ah, 01Ah, 01Ah, 01Ah, 019h, 019h, 019h
	.byte 019h, 018h, 018h, 018h, 018h, 017h, 017h, 017h, 017h, 016h, 016h, 016h, 016h, 016h, 015h, 015h
	.byte 015h, 015h, 015h, 014h, 014h, 014h, 014h, 014h, 014h, 013h, 013h, 013h, 013h, 013h, 013h, 012h
	.byte 012h, 012h, 012h, 012h, 012h, 012h, 011h, 011h, 011h, 011h, 011h, 011h, 011h, 011h, 010h, 010h
	.byte 010h, 010h, 010h, 010h, 010h, 010h, 010h, 00Fh, 00Fh, 00Fh, 00Fh, 00Fh, 00Fh, 00Fh, 00Fh, 00Fh
	.byte 00Fh, 00Eh, 00Eh, 00Eh, 00Eh, 00Eh, 00Eh, 00Eh, 00Eh, 00Eh, 00Eh, 00Eh, 00Dh, 00Dh, 00Dh, 00Dh
	.byte 00Dh, 00Dh, 00Dh, 00Dh, 00Dh, 00Dh, 00Dh, 00Dh, 00Dh, 00Ch, 00Ch, 00Ch, 00Ch, 00Ch, 00Ch, 00Ch
	.byte 00Ch, 00Ch, 00Ch, 00Ch, 00Ch, 00Ch, 00Ch, 00Ch, 00Ch, 00Bh, 00Bh, 00Bh, 00Bh, 00Bh, 00Bh, 00Bh
	.byte 00Bh, 00Bh, 00Bh, 00Bh, 00Bh, 00Bh, 00Bh, 00Bh, 00Bh, 00Bh, 00Bh, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah
	.byte 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah, 00Ah
	.byte 00Ah, 009h, 009h, 009h, 009h, 009h, 009h, 009h, 009h, 009h, 009h, 009h, 009h, 009h, 009h, 009h

	
;=============================================================================================
musicInit:
;=============================================================================================
; bankswitching: x = bank
;
; ya = soundbank address
;---------------------------------------------------------------------------------------------	
.ifdef BANKSWITCHING
	stx	modtab_bank			; save sref_bank
	clc					; setup module table
	adc	#1				;
	sta	ptr_mtable			;
	tya					;
	adc	#0C0h				;
	sta	ptr_mtable+1			;
;---------------------------------------------------------------------------------------------
.else						;
	clc					; setup module table (non bankswitched)
	adc	#1				;
	sta	ptr_mtable			;
	tya					;
	adc	#0				;
	sta	ptr_mtable+1			;
.endif						;
;---------------------------------------------------------------------------------------------
	rts


;=============================================================================================
musicPlay:
;=============================================================================================
; play song
; A = song#
; Y = starting position
;---------------------------------------------------------------------------------------------
	sty	mod_position		; save position
	sta	mod_index		; save index
;---------------------------------------------------------------------------------------------
	asl				; var1/2 = ptr_mtable + index*3
	adc	mod_index		;
	adc	ptr_mtable		;
	sta	var1			;
	lda	ptr_mtable+1		;
	adc	#0			;
	sta	var2			;
;--------------------------------------------------------------------------------------------
	lda	modtab_bank		;
	sta	BCTRLC			;
;--------------------------------------------------------------------------------------------
	ldy	#0			; var3/4 = access for module header
	lda	(var1), y		;
	sta	var3			;
	iny				;
	lda	(var1), y		;
	sta	var4			;
	iny				;
	lda	(var1), y		;
	sta	BCTRLC			;;
	sta	mod_bank		; save for later
;--------------------------------------------------------------------------------------------	
	ldy	#0			; copy tempo
	lda	(var3), y		;
	sta	mod_bpm			;
;--------------------------------------------------------------------------------------------
	iny				; copy volume
	lda	(var3), y		;
	sta	mod_volume		;
;--------------------------------------------------------------------------------------------
	iny				; copy speed
	lda	(var3), y		;
	sta	mod_speed		;
;--------------------------------------------------------------------------------------------
	iny				; var1 = npatt
	lda	(var3), y		;
	sta	var1			;
;--------------------------------------------------------------------------------------------
	iny				; var2 = ninst
	lda	(var3), y		;
	sta	var2			;
;--------------------------------------------------------------------------------------------
	iny				; copy length
	lda	(var3), y		;
	sta	mod_length		;
;--------------------------------------------------------------------------------------------
	iny				; copy channel count
	lda	(var3), y		;
	sta	mod_chcount		;
;--------------------------------------------------------------------------------------------
	iny				; get exp chips
	lda	(var3), y		;
	sta	mod_expchips		;
	and	#16
	beq	@non106
;--------------------------------------------------------------------------------------------
	ldy	#80h+64			; reset n106
	sty	N106A			;
	lda	#0			;
:	sta	N106D			;
	iny
	bne	:-

	lda	#7Fh			; init n106 wavetable
	sta	N106A			; and setup 8 channels
	lda	#%1110000		;;
	sta	N106D			;
	lda	#80h			;
	sta	N106A			;
	ldy	#sbm_n106wt		;
:	lda	(var3), y		;
	sta	N106D			;
	iny				;
	cpy	#sbm_n106wt+64		;
	bne	:-			;
@non106:				;

;--------------------------------------------------------------------------------------------
	ldy	#sbm_chmap		; copy channel mapping
:	lda	(var3), y		;
	sta	mod_chmap-sbm_chmap, y	;
	iny				;
	cpy	#sbm_chmap+16		;
	bne	:-			;
;--------------------------------------------------------------------------------------------
	
.macro add8_16m dest,  value
	lda	value
	ldy	#dest-ptr_mtable
	jsr	add8_16
.endmacro
	
	clc
	add8_16m ptr_sequence, #sbm_tables	 ; get sequence pointer
;--------------------------------------------------------------------------------------------
	add8_16m ptr_ptableL, mod_length 	; setup other table pointers
	add8_16m ptr_ptableH, var1
	add8_16m ptr_ptableB, var1
	add8_16m ptr_itableL, var1
	add8_16m ptr_itableH, var2
	add8_16m ptr_itableB, var2
;--------------------------------------------------------------------------------------------
	lda	mod_bpm				; set initial tempo
	jsr	setTempo			;
;--------------------------------------------------------------------------------------------
	ldy	#0				; reset position
	sty	mod_timer+1			; reset timer
	sty	mod_timer			;
	jsr	setPosition			;
;--------------------------------------------------------------------------------------------
	lda	#128				; set playing flag
	sta	mod_playing 			;
;--------------------------------------------------------------------------------------------
	lda	#%1111				; turn on SOUND
	sta	$4015				;
	lda	#%1000				;
	sta	$4001				;
	sta	$4005				;
;--------------------------------------------------------------------------------------------
	lda	mod_expchips			; init mmc5
	and	#8				;
	beq	@no_mmc5			;
	lda	#%11				;
	sta	$5015				;
@no_mmc5:					;
;--------------------------------------------------------------------------------------------
	lda	#255				; init fds
	sta	fds_instrument			;
;--------------------------------------------------------------------------------------------
	lda	mod_expchips			; init fme7
	and	#32				;
	beq	@no_fme7			;------
	ldy	#0Dh				; reset regs
	lda	#0				;
:	sty	FME7_ADDR			;
	sta	FME7_DATA			;
	dey					;
	bpl	:-				;------
	lda	#7				; enable tones, disable noise
	sta	FME7_ADDR			;
	lda	#%111000			;
	sta	FME7_DATA			;
@no_fme7:					;
;--------------------------------------------------------------------------------------------
	lda	mod_expchips			; reset vrc7
	and	#2				;
	beq	@no_vrc7			;
	ldy	#20h				;
	lda	#0				;
:	sty	VRC7A				;
	sta	VRC7D				;
	iny					;
	cpy	#26h				;
	bne	:-				;
@no_vrc7:					;
;--------------------------------------------------------------------------------------------
	rts
	
add8_16:					; var3/4 += a
	clc					; pointers+y = value
	adc	var3
	sta	var3
	sta	ptr_mtable, y
	lda	var4
	adc	#0
	sta	var4
	sta	ptr_mtable+1, y
	rts
	
;********************************************************************************************
;* set current position
;*
;* y: sequence position
;********************************************************************************************
setPosition:				
;--------------------------------------------------------------------------------------------
	lda	mod_bank			; switch to info bank
	sta	BCTRLC				;
;--------------------------------------------------------------------------------------------
:	lda	(ptr_sequence), y		; read sequence until valid pattern found
	cmp	#254				;
	beq	@skippatt			;
	bcc	@found				;
	ldy	#0				;(end of song, restart)
	jmp	:-				;
;--------------------------------------------------------------------------------------------
@skippatt:					;
	bne	@found				;
	iny					;
	jmp	:-				;
;--------------------------------------------------------------------------------------------
@found:	sty	mod_position			; save new position
;--------------------------------------------------------------------------------------------
	tay					; setup pattern data address
	lda	(ptr_ptableL), y		;
	sta	ptr_pattern			;
	lda	(ptr_ptableH), y		;
	sta	ptr_pattern+1			;
	lda	(ptr_ptableB), y		;
	sta	pattern_bank			;
	sta	BCTRLC				;
;--------------------------------------------------------------------------------------------
	ldy	#0				; get row count and increment pointer
	lda	(ptr_pattern), y		;
	sta	pattern_rows			;
	inc	ptr_pattern			;	
	lda	ptr_pattern			;
	bne	:+				;
	inc	ptr_pattern+1			;
;--------------------------------------------------------------------------------------------
:	lda	#0				; reset tick/position
	sta	mod_row				;		
	sta	mod_tick			;
;--------------------------------------------------------------------------------------------
	rts					; return

	
;********************************************************************************************
;* change module tempo
;*
;* a: new bpm
;********************************************************************************************
setTempo:
;--------------------------------------------------------------------------------------------
	sta	mod_bpm			; save BPM
;--------------------------------------------------------------------------------------------
	; read tempo setting
;--------------------------------------------------------------------------------------------
	tay
	lda	tempoTableL-32, y
	sta	mod_tempo
	lda	tempoTableH-32, y
	sta	mod_tempo+1
	rts

	
;********************************************************************************************
;* update module playback
;*
;* Call at 60hz
;********************************************************************************************
musicProcess:
;--------------------------------------------------------------------------------------------
	bit	mod_timer+1				; process tick while timer >= 0
	bmi	@no_updates				;
	jsr	processTick				;
;--------------------------------------------------------------------------------------------

	sec						;   timer -= TEMPO (P*150/BPM)
	lda	mod_timer				;
	sbc	mod_tempo				;
	sta	mod_timer				;
	lda	mod_timer+1				;
	sbc	mod_tempo+1				;
	sta	mod_timer+1				;
	jmp	musicProcess				; and loop
;--------------------------------------------------------------------------------------------
@no_updates:						; add "P" to timer

	clc						;
	lda	mod_timer+1				;
	adc	#TEMPO_PRECISION/256			;
	sta	mod_timer+1				;
;--------------------------------------------------------------------------------------------
	jsr	updateAudio				;
;--------------------------------------------------------------------------------------------
	rts						; return
	
	
;********************************************************************************************
;* process a module tick
;*
;********************************************************************************************
processTick:
;--------------------------------------------------------------------------------------------
	lda	mod_tick				; read pattern on tick 0
	bne	@skip_read_pattern			;
	jsr	readPattern				;
@skip_read_pattern:					;
;--------------------------------------------------------------------------------------------
	lda	mod_bank
	sta	BCTRLC
	jsr	updateChannels				; update all channels
;--------------------------------------------------------------------------------------------
	inc	mod_tick				; increment tick
	lda	mod_tick				; test for tick >= speed
	cmp	mod_speed				;
	bcc	@exit					;
;--------------------------------------------------------------------------------------------
	lda	#0					; reset tick, do pattern jump
	sta	mod_tick				;
	bit	mod_pjumpe				;
	bpl	@no_pjump				;
	sta	mod_pjumpe				;
	ldy	mod_pjump				;
	jmp	setPosition				;
;--------------------------------------------------------------------------------------------
@no_pjump:						; increment row
	inc	mod_row					; test for pattern end
	lda	mod_row					;
	cmp	pattern_rows				;
	bne	@exit					;
;--------------------------------------------------------------------------------------------
	ldy	mod_position				; increment position
	iny						;
	jmp	setPosition				;
;--------------------------------------------------------------------------------------------
@exit:	rts
	
	
;********************************************************************************************
;* read pattern data
;*
;********************************************************************************************
readPattern:
;--------------------------------------------------------------------------------------------

	ldy	pattern_bank				; map banks to pattern data
	sty	BCTRLC					;
	iny						;
	sty	BCTRLD					;
;--------------------------------------------------------------------------------------------
	ldy	#0					; process first chunk
	lda	(ptr_pattern), y			; (channels 0..7)
	iny						;
	sta	pattern_update_flags			;
	ldx	#0					;
	jsr	readPatternF				;
;--------------------------------------------------------------------------------------------
	lda	mod_chcount			; process second chunk if chcount is >= 8
	cmp	#8				;
	bcc	@lessthan8			;
	lda	(ptr_pattern), y		;
	iny					;
	sta	pattern_update_flags+1		;
	ldx	#8				;
	jsr	readPatternF			;
@lessthan8:					;
;--------------------------------------------------------------------------------------------
	tya						; add offset to ptr_pattern
	clc						; on overflow into next bank:
	adc	ptr_pattern				;   increment pattern bank
	sta	ptr_pattern				;   mask out overflow bit
	lda	ptr_pattern+1				;
	adc	#0					;
	cmp	#%11010000				;
	bcc	:+					;
	inc	pattern_bank				;
	and	#%11101111				;
:	sta	ptr_pattern+1				;
;--------------------------------------------------------------------------------------------
	rts

;--------------------------------------------------------------------------------------------
; a = bits
; x = channel
; y = offset
;--------------------------------------------------------------------------------------------
readPatternF:
;--------------------------------------------------------------------------------------------
	lsr						; test/shift first bit
	sta	var2					;
	bcc	@no_channel_data			;
;--------------------------------------------------------------------------------------------
@read_pattern_data:
;--------------------------------------------------------------------------------------------
	lda	(ptr_pattern), y			; read maskvar
	iny						;
;--------------------------------------------------------------------------------------------
	lsr						; test/read note
	sta	var1					;
	bcc	@skip_read_note				;
	lda	(ptr_pattern), y			;
	iny						;
	sta	ch_note, x				;
@skip_read_note:					;
;--------------------------------------------------------------------------------------------
	lsr	var1					; test/read instrument
	bcc	@skip_read_instr			;
	lda	(ptr_pattern), y			;
	iny						;	
	sta	ch_instrument, x				;
@skip_read_instr:					;
;--------------------------------------------------------------------------------------------
	lsr	var1					; test/read vcmd
	bcc	@skip_read_vcmd				;
	lda	(ptr_pattern), y			;
	iny						;
	sta	ch_vcmd, x				;
@skip_read_vcmd:					;
;--------------------------------------------------------------------------------------------
	lsr	var1					; test/read cmd/param
	bcc	@skip_read_cmd				;
	lda	(ptr_pattern), y			;
	iny						;
	sta	ch_command, x				;
	lda	(ptr_pattern), y			;
	iny						;
	sta	ch_param, x				;
@skip_read_cmd:						;
;--------------------------------------------------------------------------------------------
	lda	var1					; set data flags
	sta	ch_flags, x				;
;--------------------------------------------------------------------------------------------
@no_channel_data:					;
	inx						; increment index
	lsr	var2					; shift out next bit
	bcs	@read_pattern_data			; process if set
	bne	@no_channel_data			; loop if bits remain
;--------------------------------------------------------------------------------------------
	rts
	
;********************************************************************************************
;* update module channels that are flagged
;*
;********************************************************************************************
updateChannels:
;--------------------------------------------------------------------------------------------
	ldx	#0					; reset channel counter
	lda	pattern_update_flags			; load update flags for loop
;--------------------------------------------------------------------------------------------
@loop1:	lsr						; loop through flags
	pha						; update channel
	jsr	channelProcessData			;
	pla						;
	inx						;
	cmp	#0					;
	bne	@loop1					;
;--------------------------------------------------------------------------------------------
	lda	pattern_update_flags+1			;
	beq	@skip_loop2				;
	ldx	#8					; repeat for upper channels
@loop2:	lsr						;
	pha						;
	jsr	channelProcessData			;
	pla						;
	inx						;
	cmp	#0					;
	bne	@loop2					;
;--------------------------------------------------------------------------------------------
@skip_loop2:
	rts

	
;********************************************************************************************
;* process channel
;*
;* x = channel index
;********************************************************************************************
channelProcessData:

	; c = "channel has data"
	
	bcs	:+
	jmp	@do_copy_values_only
:	lda	mod_tick				; skip tick0 processing on !tick0
	bne	@nonzero_tick				;
;--------------------------------------------------------------------------------------------
	lda	ch_flags, x				; test for note
	lsr						;
	bcc	@no_note				;
;--------------------------------------------------------------------------------------------
	asl						; clear note flag
	sta	ch_flags, x				;
;--------------------------------------------------------------------------------------------
	lda	ch_note, x				; test for notecut/off
	cmp	#254					;
	beq	@notecut				;
	bcs	@noteoff				;
;--------------------------------------------------------------------------------------------
	lda	ch_flags, x				; test for glissando
	and	#%1000					;
	beq	@note_glistest				;
	lda	ch_command, x				;
	cmp	#7					;
	beq	@no_note				;
;--------------------------------------------------------------------------------------------
@note_glistest:						;
	jsr	channelStartNewNote			; start new note...	
	jmp	@note_next				;
;--------------------------------------------------------------------------------------------
@notecut:						; zero volume
	lda	#0					;
	sta	ch_volume, x				;
	beq	@no_note				;(bra)
;--------------------------------------------------------------------------------------------
@noteoff:						; disable sustain, set noteoff
	lda	ch_uflags, x				;
	and	#~UF_SUSTAIN				;
	ora	#UF_NOTEOFF				;
	sta	ch_uflags, x				;
	bne	@no_note				;
;--------------------------------------------------------------------------------------------
@note_next:						; copy default volume

	ldy	ch_instrument, x			;
	lda	(ptr_itableL), y			;
	sta	var1					;
	lda	(ptr_itableH), y			;
	sta	var2					;
	lda	(ptr_itableB), y			;
	sta	BCTRLD					;
	ldy	#sbi_dvolume				;
	lda	(var1), y				;
	sta	ch_volume, x				;
	jmp	@resetvolume				;
@no_note:						;
;--------------------------------------------------------------------------------------------
	lda	ch_flags, x				; reset volume on note or instrument
	and	#%10					;
	beq	:+					;
@resetvolume:						;
	jsr	channelResetVolume			;
:							;

;--------------------------------------------------------------------------------------------
@nonzero_tick:
;--------------------------------------------------------------------------------------------	
	lda	ch_flags, x				; process vcmd
	and	#%100					;
	beq	@no_vcmd				;
	jsr	channelProcessVolumeCommand		;
@no_vcmd:						;
;--------------------------------------------------------------------------------------------
	
	lda	ch_flags, x				; process command
	and	#%1000					;
	beq	@no_command				;
	
	jsr	copy2temps
	
	jsr	channelProcessCommand			;
	

	lda	t_volume
@copy_values:
	; convert 0..64 to 0..15
	beq	@copyvol
	lsr
	lsr
	cmp	#0
	bne	:+
	lda	#1
:	cmp	#16
	bcc	@copyvol
	lda	#15
@copyvol:
	sta	ch_uvolume, x
	
	lda	t_pitch					; copy t_pitch to final pitch
	cmp	ch_upitch_l, x				; if values are the same dont set the pitch flag.
	
	beq	@test_pitch_h
	sta	ch_upitch_l, x
	lda	t_pitch+1
	
@setpitchflag:
	sta	ch_upitch_h, x
	
	lda	ch_uflags, x
	ora	#UF_PITCH
	sta	ch_uflags, x
	jmp	@has_command
	
@test_pitch_h:
	lda	t_pitch+1
	cmp	ch_upitch_h, x
	bne	@setpitchflag
	jmp	@has_command
@no_command:						;

@do_copy_values_only:
	jsr	copy2temps
	jmp	@copy_values
;--------------------------------------------------------------------------------------------
@has_command:
	rts
	
copy2temps:

	lda	ch_pitch_l, x
	sta	t_pitch
	lda	ch_pitch_h, x
	sta	t_pitch+1
	lda	ch_volume, x
	sta	t_volume
	rts
	
;============================================================================================
channelStartNewNote:
;============================================================================================

	lda	#0				; pitch = note * 64
	sta	var1				;
	lda	ch_note, x			;
	lsr					;
	ror	var1				;
	lsr					;
	sta	ch_pitch_h, x			;
	lda	var1				;
	ror					;
	sta	ch_pitch_l, x			;
;--------------------------------------------------------------------------------------------
	lda	#UF_NOTEON|UF_PITCH|UF_INIT	; set start flag & new pitch
	ora	ch_uflags, x			;
	and	#~UF_NOTEOFF			;-remove noteoff flag
	sta	ch_uflags, x			;
;--------------------------------------------------------------------------------------------
	rts
	
;============================================================================================
channelResetVolume:
;============================================================================================
	lda	#0				; reset envelopes
	sta	ch_env_vol, x			;
	sta	ch_env_pitch, x			;
	sta	ch_env_duty, x			;
;--------------------------------------------------------------------------------------------
;	lda	#0				; reset cmem
	sta	ch_cmem, x			;
;--------------------------------------------------------------------------------------------
	lda	ch_uflags, x			; set sustain
	ora	#UF_SUSTAIN			;
	sta	ch_uflags, x			;
;--------------------------------------------------------------------------------------------
	rts
	
;********************************************************************************************
;* process volume command for channel
;*
;********************************************************************************************
channelProcessVolumeCommand:
;--------------------------------------------------------------------------------------------
	lda	ch_volume, x				; volume = do_vcmd( a=volume, var1/y=vcmd )
	sta	var1					;
	ldy	ch_vcmd, x				;
	jsr	do_vcmd					;
	sta	ch_volume, x				;
	rts						;
	
;--------------------------------------------------------------------------------------------
do_vcmd:						; test/execute set volume
	cpy	#65					; (set volume is most common)
	bcs	:+					;
	lda	mod_tick				;
	bne	skip_vcmd				;
	tya						;
	rts						;
;--------------------------------------------------------------------------------------------
:	cpy	#75					; test/execute other commands
	bcc	vcmd_finevolup				;
	cpy	#85					;
	bcc	vcmd_finevoldown			;
	cpy	#95					;
	bcc	vcmd_volup				;
	cpy	#105					;
	bcc	vcmd_voldown				;
;--------------------------------------------------------------------------------------------
skip_vcmd:						;
	lda	var1					;
exit_vcmd:
	rts						;

	
;--------------------------------------------------------------------------------------------
; 65-74 fine volume up
;--------------------------------------------------------------------------------------------
vcmd_finevolup:						; tick0
	lda	mod_tick				;
	bne	skip_vcmd				;
;--------------------------------------------------------------------------------------------
	tya						; get rate-1 (p-65) (carry is clear)
	sbc	#65					;
_vcmd_add_sat64:
	adc	var1					; get vol+rate+1 (carry is set)
;--------------------------------------------------------------------------------------------
	cmp	#65					; saturate to 64
	bcc	exit_vcmd				;
	lda	#64					;
	rts						;
	
;--------------------------------------------------------------------------------------------
; 75-84 fine volume down
;--------------------------------------------------------------------------------------------
vcmd_finevoldown:					; tick0
	lda	mod_tick				;
	bne	skip_vcmd				;
;--------------------------------------------------------------------------------------------
	tya						; var2 = rate (carry is clear)
	sbc	#75-1					;
_vcmd_sub_sat0:
	sta	var2					;
	lda	var1					; a = volume
;--------------------------------------------------------------------------------------------
	sbc	var2					; vol -= rate
	bcs	exit_vcmd				; saturate to zero
	lda	#0					;
	rts						;
	
;--------------------------------------------------------------------------------------------
; 85-94 fine volume up
;--------------------------------------------------------------------------------------------
vcmd_volup:						; !tick0
	lda	mod_tick				;
	beq	skip_vcmd				;
;--------------------------------------------------------------------------------------------
	tya						; get rate-1
	sbc	#85					;
	jmp	_vcmd_add_sat64				;
	
;--------------------------------------------------------------------------------------------
; 95-104 fine volume down
;--------------------------------------------------------------------------------------------
vcmd_voldown:						; !tick0
	lda	mod_tick				;
	beq	skip_vcmd				;
;--------------------------------------------------------------------------------------------
	tya						; get rate
	sbc	#95-1
;--------------------------------------------------------------------------------------------
	jmp	_vcmd_sub_sat0
	

command_memory_map:
	.byte	00h, 00h, 00h, 10h, 20h, 20h, 30h, 70h, 00h
	;         A    B    C    D    E    F    G    H    I
	.byte	40h, 10h, 10h, 00h, 10h, 50h, 00h, 80h, 70h
	;         J    K    L    M    N    O    P    Q    R
	.byte	60h, 00h, 70h, 00h, 10h, 00h, 00h, 00h
	;         S    T    U    V    W    X    Y    Z
	
;--------------------------------------------------------------------------------------------
channelProcessCommandMemory:
;--------------------------------------------------------------------------------------------
	ldy	ch_command, x				; read memory map entry
	lda	command_memory_map-1, y			;
	beq	@quit					; exit for 00h
;--------------------------------------------------------------------------------------------
	stx	var1					; get memory entry
	clc						;
	adc	var1					;
	tay						;
;--------------------------------------------------------------------------------------------
	cpy	#70h					; single for <7 param
	bcc	@single					;
;--------------------------------------------------------------------------------------------
@double:						; var1 = cmd mem
	lda	commandMemory-10h, y			;
	sta	var1					;
;--------------------------------------------------------------------------------------------
	lda	ch_param, x				; a = param
	cmp	#10h					; check if upper bits are set
	bcc	@upper_cleared				;
;-------------------------------------------------------------------------------------------
	and	#0F0h					; var1 = (var1&0Fh) | (param&0F0h)
	sta	var2					; (set upper bits to param upper)
	lda	var1					;
	and	#0Fh					;
	ora	var2					;
	sta	var1					;
	lda	ch_param, x				;
@upper_cleared:						;
;--------------------------------------------------------------------------------------------
	and	#0Fh					; test if lower bits are set	
	beq	@lower_cleared				;
;--------------------------------------------------------------------------------------------
	sta	var2					; set lower bits to param lower
	lda	var1					;
	and	#0F0h					;
	ora	var2					;
	sta	var1					;
@lower_cleared:						;
;--------------------------------------------------------------------------------------------
	lda	var1					; save values	
	sta	ch_param, x				;
	sta	commandMemory-10h, y			;
	rts						;
;--------------------------------------------------------------------------------------------
@single:						; if param is zero
	lda	ch_param, x				;    param = memory
	beq	@cleared				; else
	sta	commandMemory-10h, y			;    memory = param
	rts						;
@cleared:						;
	lda	commandMemory-10h, y			;
	sta	ch_param, x				;
@quit:							;
__quit1:
	rts						;
	
	
;--------------------------------------------------------------------------------------------
channelProcessCommand:
;--------------------------------------------------------------------------------------------
	lda	ch_command, x				; exit if cmd = 0
	beq	__quit1					;
;--------------------------------------------------------------------------------------------
	lda	mod_tick				; process memory on t0
	bne	:+					;
	jsr	channelProcessCommandMemory		;
:							;
;--------------------------------------------------------------------------------------------
	ldy	ch_command, x				; set jump address
	lda	commandRoutinesL, y			;
	sta	var1					;
	lda	commandRoutinesH, y			;
	sta	var2					;
;--------------------------------------------------------------------------------------------
	lda	ch_param, x				; preload data
	ldy	mod_tick				;
;--------------------------------------------------------------------------------------------
	; a = param
	; y = tick
	; z = tick0
;--------------------------------------------------------------------------------------------
	jmp	(var1)					; jump to command

;============================================================================================
commandSetSpeed:
;============================================================================================
	bne	:+					; tick0:
	sta	mod_speed				; speed = param
:	rts						;
;============================================================================================
commandSetPosition:
;============================================================================================
	bne	:+					; tick0:
	sta	mod_pjump				; pjump = param
	lda	#128					; enable pjump
	sta	mod_pjumpe				;
:	rts						;
;============================================================================================
commandPatternBreak:
;============================================================================================
	bne	:+					; tick0:
	ldy	mod_position				; pjump = position+1
	iny						; enable pjump
	sty	mod_pjump				;
	lda	#128					;
	sta	mod_pjumpe				;
:	rts						;
;============================================================================================
commandVolumeSlide:
;============================================================================================
	sta	var1				; var1 = param
;--------------------------------------------------------------------------------------------
	and	#0Fh				; test volume command type
	beq	@up				;
	lda	var1				;
	and	#0F0h				;
	beq	@down				;
	lda	var1				;
	and	#0Fh				;
	cmp	#0Fh				;
	beq	@fineup				;
	lda	var1				;
	cmp	#0F0h				;
	bcs	@finedown			;
@quit:	rts					;
;--------------------------------------------------------------------------------------------
@exit0:	lda	#0
@exit:	sta	t_volume			; save result to volume & temp
	sta	ch_volume, x			;
	rts					;

;--------------------------------------------------------------------------------------------
@finedown:
;--------------------------------------------------------------------------------------------
	cpy	#0				; on tick0:
	bne	@quit				;
;--------------------------------------------------------------------------------------------
	lda	var1				; a = volume - y
	and	#0Fh				;
	sta	var1				;
	lda	t_volume			;
	sbc	var1				;
	bcs	@exit				; saturate lower to 0
	jmp	@exit0				;
	
;--------------------------------------------------------------------------------------------
@fineup:
;--------------------------------------------------------------------------------------------
	cpy	#0				; on tick0:
	bne	@quit				;
;--------------------------------------------------------------------------------------------
	lda	var1				; a = x
	lsr_x4					;
;--------------------------------------------------------------------------------------------
	clc					; volume += x
	adc	t_volume			; saturate to 64
	cmp	#64				;
	bcc	@exit				;
	lda	#64				;
	jmp	@exit				;
	
;--------------------------------------------------------------------------------------------
@down:
;--------------------------------------------------------------------------------------------
	lda	var1				; on !tick0 OR y == 15
	cmp	#15				;
	beq	:+				;
	cpy	#0				;
	beq	@quit				;
;--------------------------------------------------------------------------------------------
:	lda	t_volume			; volume -= param
	sec					; saturate lower to 0
	sbc	var1				;
	bcs	@exit				;
	jmp	@exit0				;

;--------------------------------------------------------------------------------------------
@up:
;--------------------------------------------------------------------------------------------
	lda	var1				; on !tick0 OR x == 15
	cmp	#0F0h				;
	beq	:+				;
	cpy	#0				;
	beq	@quit				;
;--------------------------------------------------------------------------------------------
:	lsr_x4					; volume += x
	clc					;
	adc	t_volume			;
	cmp	#64				; saturate to 64
	bcc	@exit				;
	lda	#64				;
	bne	@exit				;(bra)

;============================================================================================
commandPitchSlideDown:
;============================================================================================
	jsr	pitchSlideLoad			; get pitch slide amount
	lda	t_pitch				; pitch -= amount
	sec					;
	sbc	var1				;
	sta	t_pitch				;
	sta	ch_pitch_l, x			;
	lda	t_pitch+1			;
	sbc	var2				;
	bmi	@overflow			;
	sta	t_pitch+1			;
	sta	ch_pitch_h, x			;
	rts					;
;--------------------------------------------------------------------------------------------
@overflow:					; on overflow: pitch = 0
	lda	#0				;
	sta	t_pitch				;
	sta	t_pitch+1			;
	sta	ch_pitch_l, x			;
	sta	ch_pitch_h, x			;
	rts					;
	
;============================================================================================
commandPitchSlideUp:
;============================================================================================
	jsr	pitchSlideLoad			; get pitch slide amount
;--------------------------------------------------------------------------------------------
	clc					;
	lda	t_pitch				; pitch += amount
	adc	var1				;
	sta	t_pitch				;
	sta	ch_pitch_l, x			;
	lda	t_pitch+1			;
	adc	var2				;
	cmp	#01Ah				; saturate to 01A00h
	bcs	@overflow			;
	sta	t_pitch+1			;
	sta	ch_pitch_h, x			;
	rts					;
;--------------------------------------------------------------------------------------------
@overflow:					; on overflow: pitch  =max
	lda	#1Ah				;
	sta	t_pitch+1			;
	sta	ch_pitch_h, x			;
	lda	#0				;
	sta	t_pitch				;
	sta	ch_pitch_l, x			;
	rts					;
	
;============================================================================================
pitchSlideLoad:
;============================================================================================
	cmp	#0F0h
	bcs	@fineslide
	cmp	#0E0h
	bcs	@extrafineslide
;--------------------------------------------------------------------------------------------
@normalslide:
;--------------------------------------------------------------------------------------------
	cpy	#0				; no slide on tick0
	beq	@zeroslide			;
;--------------------------------------------------------------------------------------------
@load_x4:
	ldy	#0				; var1/2 = p*4
	sty	var2				;
	asl					;
	rol	var2				;
	asl					;
	rol	var2				;
	sta	var1				;
	rts					;

;--------------------------------------------------------------------------------------------
@fineslide:
;--------------------------------------------------------------------------------------------
	cpy	#0				; no slide on !tick0
	bne	@zeroslide			;
;--------------------------------------------------------------------------------------------
	and	#0Fh				; var 1/2 = y*4
	jmp	@load_x4			;
	
;--------------------------------------------------------------------------------------------
@extrafineslide:
;--------------------------------------------------------------------------------------------
	cpy	#0				; no slide on !tick0
	bne	@zeroslide			;
;--------------------------------------------------------------------------------------------
	ldy	#0				; var1/2 = y
	sty	var2				;
	and	#0Fh				;
	sta	var1				;
	rts					;

;--------------------------------------------------------------------------------------------
@zeroslide:
;--------------------------------------------------------------------------------------------
	lda	#0				; var1/2 = 0
	sta	var1				;
	sta	var2				;
	rts					;
	
;============================================================================================
commandGlissando:
;============================================================================================
	beq	@exit				; on !tick0:
;--------------------------------------------------------------------------------------------
	ldy	#0				; var1/2 = xx*4 (slide amount)
	sty	var2				;
	sty	var3				;
	asl					;
	rol	var2				;
	asl					;
	rol	var2				;
	sta	var1				;
;--------------------------------------------------------------------------------------------
	lda	ch_note, x			; var3/4 = note * 64 (target pitch)
	lsr					;
	ror	var3				;
	lsr					;
	ror	var3				;
	sta	var4				;
;--------------------------------------------------------------------------------------------
	lda	t_pitch+1			; test slide direction (upper byte)
	cmp	var4				;
	bcc	@slideup			;
	bne	@slidedown			;
;--------------------------------------------------------------------------------------------
	lda	t_pitch				; lower byte
	cmp	var3				;
	bcc	@slideup			;
	bne	@slidedown			;	
@exit:	rts					;
	
;--------------------------------------------------------------------------------------------
@slidedown:					; note: carry is set
	lda	t_pitch				; var1/2 = pitch - var1/2
	sbc	var1				; set direct if overflow past 0
	sta	var1				;
	lda	t_pitch+1			;
	sbc	var2				;
	bcc	@set_direct			;
	sta	var2				;
;--------------------------------------------------------------------------------------------
	cmp	var4				; test for upper < target_upper
	bcc	@set_direct			;
	bne	@set_normal			;
	lda	var1				; test for lower < target_lower
	cmp	var3				;
	bcc	@set_direct			;
;--------------------------------------------------------------------------------------------
@set_normal:					; pitch = var1/2
	lda	var1				;
	sta	t_pitch				;
	sta	ch_pitch_l, x			;
	lda	var2				;
	sta	t_pitch+1			;
	sta	ch_pitch_h, x			;
	rts
;--------------------------------------------------------------------------------------------
@set_direct:					; pitch = var3/4
	lda	var3				;
	sta	t_pitch				;
	sta	ch_pitch_l, x			;
	lda	var4				;
	sta	t_pitch+1			;
	sta	ch_pitch_h, x			;
	rts					;

;--------------------------------------------------------------------------------------------
@slideup:					; carry is CLEAR
	lda	t_pitch				; var1/2 = var1/2 + pitch
	adc	var1				;
	sta	var1				;
	lda	t_pitch+1			;
	adc	var2				;
	sta	var2				;
;--------------------------------------------------------------------------------------------
	cmp	var4				; if upper < target, set normal
	bcc	@set_normal			; if upper = target, check lower
	bne	@set_direct			; if upper > target, set direct
;--------------------------------------------------------------------------------------------
	lda	var1				; if lower < target, set normal
	cmp	var3				; otherwise set direct
	bcc	@set_normal			;
	bcs	@set_direct			;(bra)

;============================================================================================
commandVibrato:
;============================================================================================
	lda	#70h				; read vibrato param
	stx	var1				;
	ora	var1				;
	tay					;
	lda	commandMemory-10h, y		;
	sta	var1				;
;--------------------------------------------------------------------------------------------
	lsr_x4					; cmem = (cmem + x) & 63
	clc					;
	adc	ch_cmem, x			;
	and	#63				;
	sta	ch_cmem, x			;
;--------------------------------------------------------------------------------------------
	ldy	#16				; var2 = const 16
	sty	var2				;
;--------------------------------------------------------------------------------------------
	cmp	#32				; c = position >= 32
;--------------------------------------------------------------------------------------------
	bit	var2				; a = (pos&15)
	beq	:+				; if pos & 16 then a = a ^ 15
	eor	#15				; y = a
:	and	#15				;
	beq	@quit				;
	tay					;
;--------------------------------------------------------------------------------------------
	lda	var1				; a = y
	and	#0Fh				;
	beq	@quit				; catch zero depth
;--------------------------------------------------------------------------------------------
	ora	Table_mul16, y			; y = a | (p << 4)
	tay					; a = vibtable[y]
	lda	vibratoTable-10h, y		;
;--------------------------------------------------------------------------------------------
	bcs	@subtract			; pitch += a
	adc	t_pitch				;
	sta	t_pitch				;
	lda	t_pitch+1			;
	adc	#0				;
	sta	t_pitch+1			;
	rts					;
;--------------------------------------------------------------------------------------------
@subtract:					; pitch -= a
	sta	var1				;
	lda	t_pitch				;
	sbc	var1				;
	sta	t_pitch				;
	lda	t_pitch+1			;
	sbc	#0				;
	sta	t_pitch+1			;
@quit:	rts					;

;============================================================================================
commandTremor:
;============================================================================================
	and	#0F0h				; strange code
	sta	var1
	lda	ch_cmem, x
	cmp	var1
	bcc	@ontime
@offtime:
	cmp	ch_cmem, x
	bcc	:+
	lda	#0
	beq	@ontime ;(bra)
:	adc	#1
	sta	ch_cmem, x
	lda	#0
	sta	t_volume
@ontime:
	adc	#10h
	sta	ch_cmem, x
	rts
	
	
;============================================================================================
commandArpeggio:
;=============================================================================================
	bne	@othertick		; tick0 or multiple of 3: reset counter
@resetarp:				;
	lda	#0			;
	sta	ch_cmem, x		;
	rts				;
;--------------------------------------------------------------------------------------------
@othertick:				; increment counter
	inc	ch_cmem, x		;
	ldy	ch_cmem, x		;
	cpy	#2			; 1 : add x
	beq	@add_y			; 2 : add y
	bcs	@resetarp		; 3 : reset
;--------------------------------------------------------------------------------------------
@add_x:					; pitch += x semitones
	ldy	#0			;
	sty	var2			;
	asl				;
	rol	var2			;
	asl				;
	rol	var2			;
@do_add:				;
	adc	t_pitch			;
	sta	t_pitch			;
	lda	t_pitch+1		;
	adc	var2			;
	sta	t_pitch+1		;
	rts				;
;--------------------------------------------------------------------------------------------
@add_y:					; pitch += y semitiones
	and	#0Fh			;
	sta	var2			;
	lda	#0			;
	lsr	var2			;
	ror				;
	lsr	var2			;
	ror				;
	jmp	@do_add			;
	
;============================================================================================
commandVolumeSlideVibrato:
;============================================================================================
	jsr	commandVibrato
	lda	ch_param, x
	ldy	mod_tick
	jmp	commandVolumeSlide
	
;============================================================================================
commandVolumeSlideGlis:
;============================================================================================
	rts				; todo
	
;============================================================================================
commandRetriggerNote:
;============================================================================================
	and	#0Fh			; var1 = y == 0 ? 1 : y
	bne	:+			;
	lda	#1			;
:	sta	var1			;
;--------------------------------------------------------------------------------------------
	lda	ch_cmem, x		; if cmem == 0: cmem = var1
	bne	:+			;
	lda	var1			;
@count_ret:				;
	sta	ch_cmem, x		;
	rts				;
;--------------------------------------------------------------------------------------------
:	sec				; else: decrement cmem until 0
	sbc	#1			;
	bne	@count_ret		;
;--------------------------------------------------------------------------------------------
	lda	var1			; cmem = m0
	sta	ch_cmem, x		;
;--------------------------------------------------------------------------------------------
	lda	ch_param, x
	lsr_x4
	tay
	lda	@retriggerVolumeRoutinesL, y
	sta	var1
	lda	@retriggerVolumeRoutinesH, y
	sta	var2
	lda	t_volume
	clc
	jmp	(var1)
	
;--------------------------------------------------------------------------------------------
@retriggerVolumeRoutinesL:
	.byte	<@av0, <@av1, <@av2, <@av3
	.byte	<@av4, <@av5, <@av6, <@av7
	.byte	<@av8, <@av9, <@avA, <@avB
	.byte	<@avC, <@avD, <@avE, <@avF
;--------------------------------------------------------------------------------------------
@retriggerVolumeRoutinesH:
	.byte	>@av0, >@av1, >@av2, >@av3
	.byte	>@av4, >@av5, >@av6, >@av7
	.byte	>@av8, >@av9, >@avA, >@avB
	.byte	>@avC, >@avD, >@avE, >@avF
;--------------------------------------------------------------------------------------------
@av1:	sbc	#1-1				; -1
	jmp	@sat0				;
;--------------------------------------------------------------------------------------------
@av2:	sbc	#2-1				; -2
	jmp	@sat0				;
;--------------------------------------------------------------------------------------------
@av3:	sbc	#4-1				; -4
	jmp	@sat0				;
;--------------------------------------------------------------------------------------------
@av4:	sbc	#8-1				; -8
	jmp	@sat0				;
;--------------------------------------------------------------------------------------------
@av5:	sbc	#16-1				; -16
	jmp	@sat0				;
;--------------------------------------------------------------------------------------------
@av6:	tay					; *2/3
	lsr					; V = V - round(V/4) - round(V/16) - round(v/64)
	lsr					; 
	pha					;
	adc	#0				;
	sta	var2				; var2 = round(V/4)
	pla					;
	lsr					;
	lsr					;
	adc	#0				;  a = round(V/8)
	adc	var2				;  +var2
	cpy	#32				;  +1 if v >= 32 [round(v/64)]
	adc	#0				; (clears carry, -1 for following subtraction)
	sbc	var1				; a = V - subt
	eor	#255				;
	jmp	@set				;
;--------------------------------------------------------------------------------------------
@av7:	lsr					; /2
	jmp	@set				;
;--------------------------------------------------------------------------------------------
@av0:						; no change
@av8:	jmp	@set				;
;--------------------------------------------------------------------------------------------
@av9:	adc	#1				; +1
	jmp	@sat64				;
;--------------------------------------------------------------------------------------------
@avA:	adc	#2				; +2
	jmp	@sat64				;
;--------------------------------------------------------------------------------------------
@avB:	adc	#4				; +4
	jmp	@sat64				;
;--------------------------------------------------------------------------------------------
@avC:	adc	#8				; +8
	jmp	@sat64				;
;--------------------------------------------------------------------------------------------
@avD:	adc	#16				; +16
	jmp	@sat64				;
;--------------------------------------------------------------------------------------------
@avE:	asl					; *3/2
	adc	t_volume			;
	lsr					;
	jmp	@sat64
;--------------------------------------------------------------------------------------------
@avF:	asl					; *2
	jmp	@sat64				;
;--------------------------------------------------------------------------------------------
@sat0:						; saturate lower bound to 0
	bpl	@set				;
	lda	#0				;
	jmp	@set				;
;--------------------------------------------------------------------------------------------
@sat64:						; saturate upper bound to 64
	cmp	#65				;
	bcc	@set				;
	lda	#64				;
;--------------------------------------------------------------------------------------------
@set:						; set volume
	sta	t_volume			;
	sta	ch_volume, x			;
;--------------------------------------------------------------------------------------------
	lda	ch_uflags, x
	ora	#UF_NOTEON
	sta	ch_uflags, x
	rts

;============================================================================================
commandTremolo:
;============================================================================================
	lsr_x4					; cmem = (cmem + x) & 63
	clc					;
	adc	ch_cmem, x			;
	and	#63				;
	sta	ch_cmem, x			;
;--------------------------------------------------------------------------------------------
	ldy	#16				; var2 = const 16
	sty	var2				;
;--------------------------------------------------------------------------------------------
	cmp	#32				; c = position >= 32
;--------------------------------------------------------------------------------------------
	bit	var2				; a = (pos&15)
	beq	:+				; if pos & 16 then a = a ^ 15
	eor	#15				; y = a
:	and	#15				;
	beq	@position_0			;
	tay					;
;--------------------------------------------------------------------------------------------
	lda	ch_param, x			; a = y
	and	#0Fh				;
	beq	@quit				; catch zero depth
;--------------------------------------------------------------------------------------------
	ora	Table_mul16, y			; y = a | (p << 4)
	tay					; a = vibtable[y]
	lda	vibratoTable-10h, y		;
;--------------------------------------------------------------------------------------------
	bcs	@subtract			; volume += a
	adc	t_volume			; saturate to 64
	cmp	#64				;
	bcc	:+				;
	lda	#64				;
:	sta	t_volume			;
	rts					;
;--------------------------------------------------------------------------------------------
@subtract:					; volume -= a
	sta	var1				; saturate lower 0
	lda	t_volume			;
	sbc	var1				;
	bpl	:+				;
	lda	#0				;
:	sta	t_volume			;
@position_0:					;
@quit:	rts					;

	
;============================================================================================
commandChangeTempo:
;============================================================================================
	cmp	#20h				; if param < 20h then its a slide command
	bcc	@slide				;
	jmp	setTempo			;
;--------------------------------------------------------------------------------------------
@slide:						; test slide direction
	cmp	#10h				;
	bcc	@slide_down			;
;--------------------------------------------------------------------------------------------
	and	#0Fh				; bpm += y
	clc					; saturate to 255
	adc	mod_bpm				;
	bcc	:+				;
	lda	#255				;
:	jmp	setTempo			;
;--------------------------------------------------------------------------------------------
@slide_down:					; bpm -= y
	sta	var1				; saturate lower to 255
	lda	mod_bpm				;
	sec					;
	sbc	var1				;
	cmp	#32				;
	bcs	:+				;
	lda	#32				;
:	jmp	setTempo			;

;============================================================================================
commandFineVibrato:				;
;============================================================================================
	rts					; todo ?
	
;============================================================================================
commandSetGlobalVolume:
;============================================================================================
	bne	@quit				; on tick0:
	cmp	#80h				; gv = param > 128 ? 128 : param
	bcc	:+				;
	lda	#80h				;
:	sta	mod_volume			;
@quit:	rts					;
	
;============================================================================================
commandExtended:
;============================================================================================
	lsr_x4					; var1/2 = jump target
	tay					;
	lda	xcommandRoutinesL, y		;
	sta	var1				;
	lda	xcommandRoutinesH, y		;
	sta	var2				;
;--------------------------------------------------------------------------------------------
	lda	ch_param, x			; a = y
	and	#0Fh				; y = tick
	ldy	mod_tick			; z = tick0
;--------------------------------------------------------------------------------------------
	jmp	(var1)

;--------------------------------------------------------------------------------------------

;============================================================================================
commandUnused:
xcommandUnused:
;============================================================================================
	rts
	
;============================================================================================
xcommandNoteCut:
;============================================================================================
	cmp	mod_tick				; on tick y:
	bne	@quit					;
	lda	#0					; volume = 0
	sta	t_volume				;
	sta	ch_volume, x				;
@quit:	rts						;
;============================================================================================
xcommandNoteDelay:
;============================================================================================
	cmp	mod_tick
	beq	@equ
	bcs	@lower
	rts
@lower:
	lda	#UF_DELAY
@addflag:
	ora	ch_uflags, x
	sta	ch_uflags, x
	rts
@equ:
	lda	#(UF_PITCH | UF_NOTEON)
	bne	@addflag ;(bra)
;============================================================================================
xcommandPatternDelay:
;============================================================================================
	rts					; todo
	
	
;********************************************************************************************
;* update envelopes and all kinds of shit
;*
;********************************************************************************************
updateAudio:
;--------------------------------------------------------------------------------------------
	ldx	#0					; bank C = mod header
	lda	mod_bank				;
	sta	BCTRLC					;
;--------------------------------------------------------------------------------------------
@loop:
;--------------------------------------------------------------------------------------------
	ldy	ch_instrument, x			; map var1/2 to instrument (bankD)
	lda	(ptr_itableL), y			;
	sta	var1					;
	lda	(ptr_itableH), y			;
	sta	var2					;
	lda	(ptr_itableB), y			;
	sta	BCTRLD					;
;--------------------------------------------------------------------------------------------
	lda	ch_upitch_l, x
	sta	t_pitch
	lda	ch_upitch_h, x
	sta	t_pitch+1
	lda	ch_uvolume, x
	sta	t_volume
	lda	ch_uflags, x
	sta	t_flags
	and	#~(UF_PITCH | UF_NOTEON)
	sta	ch_uflags, x
	and	#UF_INIT
	beq	@channel_not_initialized
	lda	ch_uflags, x
	and	#UF_DELAY
	beq	@notdelayed
	lda	ch_uflags, x
	and	#~UF_DELAY
	sta	ch_uflags, x
	jmp	@channel_delayed
@notdelayed:
	
;--------------------------------------------------------------------------------------------
	jsr	channelUpdateModulation			; 
	jsr	channelUpdateEnvelopes			; update 60hz stuff 
;--------------------------------------------------------------------------------------------
	ldy	#sbi_pbase				; pitch += pitch base
	clc						; saturate value
	lda	t_pitch					;
	adc	(var1), y				;
	sta	t_pitch					;
	lda	t_pitch+1				;
	iny						;
	adc	(var1), y				;	
	bpl	@good_pitch
;	bmi	@clamp_zero				;
;	cmp	#1Ch					;
;	bcc	@good_pitch				;
;	lda	#1Ch					;
;	sta	t_pitch					;
;	lda	#0					;
;	beq	@good_pitch				;(bra)
@clamp_zero:						;
	lda	#0					;
	sta	t_pitch					;
@good_pitch:						;
	sta	t_pitch+1				;
;--------------------------------------------------------------------------------------------
	jsr	channelUpdateAudio			; update audio
;--------------------------------------------------------------------------------------------
@channel_not_initialized:
@channel_delayed:
	inx						; iterate through all channels
	cpx	mod_chcount				;
	bne	@loop					;
;--------------------------------------------------------------------------------------------
	rts
	
	
;============================================================================================
channelUpdateEnvelopes:
;============================================================================================
	lda	#sbi_envelopes				; var3/4 = instrument+envelopes
	clc						;
	adc	var1					;
	sta	var3					;
	lda	var2					;
	adc	#0					;
	sta	var4					;
;--------------------------------------------------------------------------------------------
	ldy	#sbi_envelopemask			; var7 = envelope bits
	lda	(var1), y				; 
	lsr						;
	sta	var7					;
;--------------------------------------------------------------------------------------------
	bcc	@skip_volenv
	lda	ch_env_vol, x
	jsr	readEnvelope
	sta	ch_env_vol, x
	tya
	bmi	@skip_volenv

	ldy	t_volume			;; var6 = original volume (4bit)  (4x4 multiply, 60+8 cycles)
	cpy	#15		;; use direct value on full volume
	bcc	:+		;
	tay
	lda	@inverse_venv, y
	sta	t_volume
	jmp	@skip_volenv
:	sty	var6				;
	sta	var5				; var5 = factor (inverted) 
	lda	#0				; a = accumulator
	lsr	var5				; test/add value*8
	bcs	:+				;
	adc	var6				;
:	asl					; test/add value*4
	lsr	var5				;
	bcs	:+				;
	adc	var6				;
:	asl					; test/add value*2
	lsr	var5				;
	bcs	:+				;
	adc	var6				;
:	asl					; test/add value*1
	lsr	var5				;
	bcs	:+				;
	adc	var6				;
:	lsr_x4
	sta	t_volume			; save result	
@skip_volenv:					;
;--------------------------------------------------------------------------------------------
	lsr	var7				; test PITCH bit
	bcc	@skip_pitchenv			;
;--------------------------------------------------------------------------------------------
	lda	ch_env_pitch, x			; process pitch envelope
	jsr	readEnvelope			;
	sta	ch_env_pitch, x			;
;--------------------------------------------------------------------------------------------
	tya					; a = envelope value
	eor	#128				; sign value
;--------------------------------------------------------------------------------------------
	bpl	:+				; var5/a = value*4
	ldy	#255				;
	bne	:++				;(bra)
:	ldy	#0				;
:	sty	var5				;
	asl					;
	rol	var5				;
	asl					;
	rol	var5				;
	asl					;
	rol	var5				;
	asl					;
	rol	var5				;
;--------------------------------------------------------------------------------------------
	clc					; pitch += value
	adc	t_pitch				;
	sta	t_pitch				;
	lda	t_pitch+1			;
	adc	var5				;
	sta	t_pitch+1			;
;--------------------------------------------------------------------------------------------
	lda	t_flags				;
	ora	#UF_PITCH			;
	sta	t_flags				;
;--------------------------------------------------------------------------------------------
@skip_pitchenv:					;
;--------------------------------------------------------------------------------------------
	lsr	var7				; test DUTY bit
	bcc	@skip_dutyenv			;
;--------------------------------------------------------------------------------------------
	lda	ch_env_duty, x			; process duty envelope
	jsr	readEnvelope			;
	sta	ch_env_duty, x			;
;--------------------------------------------------------------------------------------------
	sty	t_duty_env			; store value
	jmp	@has_duty_env
;--------------------------------------------------------------------------------------------
@skip_dutyenv:					; disable duty env
	lda	#128				;
	sta	t_duty_env			;
@has_duty_env:					;
;--------------------------------------------------------------------------------------------
	rts
	
	; handcrafted, doublecheck for errors
@inverse_venv:
	.byte	%1111, %0111, %1011, %0011
	.byte	%1101, %0101, %1001, %0001
	.byte	%1110, %0110, %1010, %0010
	.byte	%1100, %0100, %1000, %0000
	
;============================================================================================
; a = envelope position
; var3/4 = envelope address
;
; returns:
;   var3/4 += size
;   y = envelope value
;   a = envelope position
;--------------------------------------------------------------------------------------------
readEnvelope:
;============================================================================================
	sta	var5					; var5 = position
;--------------------------------------------------------------------------------------------
	ldy	#0					; var6 = size
	lda	(var3), y				;
	sta	var6					;
;--------------------------------------------------------------------------------------------
	ldy	var5					; y = ++position
@repeat_read_inc:
	iny						; 
;--------------------------------------------------------------------------------------------
@repeat_read:						; read node and test for control byte
	lda	(var3), y				;
	cmp	#254					;
	bcc	@normal_node				;
;--------------------------------------------------------------------------------------------
@special_command:					; 255 = loop
	beq	@sustain				; 254 = sustain
;--------------------------------------------------------------------------------------------
	iny						; pos = loop point
	lda	(var3), y				;
	tay						;
	jmp	@repeat_read				;
;--------------------------------------------------------------------------------------------
@sustain:						; pos = sustain point
	iny						; if sustain is set
	lda	ch_uflags, x				;
	bpl	@repeat_read_inc			;
	lda	(var3), y				;
	tay						;
	jmp	@repeat_read				;
;--------------------------------------------------------------------------------------------
@normal_node:						; var3/4 += size
	sta	var5					;
	clc						;
	lda	var6					;
	adc	var3					;
	sta	var3					;
	lda	var4					;
	adc	#0					;
	sta	var4					;
	tya
	ldy	var5					;
;--------------------------------------------------------------------------------------------
	rts
	
;============================================================================================
channelUpdateModulation:
;============================================================================================
	lda	t_flags				; read flags
;--------------------------------------------------------------------------------------------
	and	#UF_NOTEON			; if noteon:
	beq	:+				;   mask out MOD flag
	ldy	#sbi_mdepth			;   sweep = delay
	lda	(var1), y			;   pos,depth = 0
	beq	@nomod				;
	lda	ch_uflags, x			;
	and	#~UF_MOD			;
	sta	ch_uflags, x			;
	ldy	#sbi_mdelay			;
	lda	(var1), y			;
	sta	ch_msweep, x			;
	lda	#0				;
	sta	ch_mpos, x			;
	sta	ch_mdepth, x			;
@nomod:						;
	rts					;
;--------------------------------------------------------------------------------------------
:	lda	ch_uflags, x			; read flags
	and	#UF_MOD				; jump to delay part if mod is not set
	beq	@delayed			;
;--------------------------------------------------------------------------------------------
	lda	ch_mdepth, x			; if depth != instr.depth:
	ldy	#sbi_mdepth			;
	cmp	(var1), y			;
	beq	:++				;
;--------------------------------------------------------------------------------------------
	ldy	ch_msweep, x			; sweep--
	dey					;
	bne	:+				; if sweep = 0:
;--------------------------------------------------------------------------------------------
	adc	#1				; depth++
	sta	ch_mdepth, x			;
	ldy	#sbi_msweep			; reset sweep
	lda	(var1), y			;
	sta	ch_msweep, x			;
	jmp	:++				;
;--------------------------------------------------------------------------------------------
:	tya					; (save sweep)
	sta	ch_msweep, x			;
:						;
;--------------------------------------------------------------------------------------------
	lda	t_flags				; set update pitch flag
	ora	#UF_PITCH			;
	sta	t_flags				;
;--------------------------------------------------------------------------------------------
	lda	ch_mpos, x			; pos += rate
	ldy	#sbi_mrate			;
	clc					;
	adc	(var1), y			;
	sta	ch_mpos, x			;
;--------------------------------------------------------------------------------------------
	asl					; shift pos into upper bits
	asl					;
;--------------------------------------------------------------------------------------------
	asl					; shift out &32 (sign)
	bcs	@subtraction			;
;--------------------------------------------------------------------------------------------
@addition:					; 
	asl					; shift out 'phase'
	bcc	:+				;
	eor	#0F0h				;
	clc					;
:	beq	@position_0
	ora	ch_mdepth, x			; apply depth
	tay					;
	lda	vibratoTable-10h, y		; read vibrato table and add to pitch
	adc	t_pitch				;
	sta	t_pitch				;
	lda	t_pitch+1			;
	adc	#0				;
	sta	t_pitch+1			;
@position_0:
	rts					;
;--------------------------------------------------------------------------------------------
@subtraction:					; same as above except subtract.
	asl					;
	bcc	:+				;
	eor	#0F0h				;
:	beq	@position_0
	ora	ch_mdepth, x			;
	tay					;
	lda	vibratoTable-10h, y		;
	sec					;
	sta	var3				;
	lda	t_pitch				;
	sbc	var3				;
	sta	t_pitch				;
	lda	t_pitch+1			;
	sbc	#0				;
	sta	t_pitch+1			;
	rts					;
;--------------------------------------------------------------------------------------------
@delayed:					; decrement sweep until zero

	ldy	ch_msweep, x			;
	dey					;
	beq	@start				;
;--------------------------------------------------------------------------------------------
	tya					; (save sweep)
	sta	ch_msweep, x			;
	rts					;
;--------------------------------------------------------------------------------------------
@start:						; set MOD flag

	lda	ch_uflags, x			;
	ora	#UF_MOD				;
	sta	ch_uflags, x			;
	ldy	#sbi_msweep			;
	lda	(var1), y			;
	sta	ch_msweep, x			; reset sweep
	rts					;

;============================================================================================
channelUpdateAudio:
;============================================================================================
	ldy	mod_chmap, x				; jump to audio service routine
	lda	audioRoutinesL, y			;
	sta	var5					;
	lda	audioRoutinesH, y			;
	sta	var6					;
	lda	t_flags					;
	lsr						; c = &pitch
	lda	#0					; v = &noteon
	bit	t_flags					; m = &sustain
	jmp	(var5)					; a = 0
;--------------------------------------------------------------------------------------------
	rts
	
;********************************************************************************************
;*
;* 2A03
;*
;********************************************************************************************

;--------------------------------------------------------------------------------------------
	SOUND00	:= 4000H	; DUTY, VOLUME		PULSE1
	SOUND01	:= 4001H	; SWEEP
	SOUND02	:= 4002H	; FINE PERIOD
	SOUND03	:= 4003H	; COURSE PERIOD
;--------------------------------------------------------------------------------------------
	SOUND10	:= 4004H	; DUTY, VOLUME		PULSE2
	SOUND11	:= 4005H	; SWEEP
	SOUND12	:= 4006H	; FINE PERIOD
	SOUND13	:= 4007H	; COURSE PERIOD
;--------------------------------------------------------------------------------------------
	SOUND20	:= 4008H	; COUNTER SETUP		TRIANGLE
	SOUND22	:= 400AH	; FINE PERIOD
	SOUND23	:= 400BH	; COURSE PERIOD
;--------------------------------------------------------------------------------------------
	SOUND30	:= 400CH	; VOLUME		NOISE
	SOUND32	:= 400EH	; LOOP / PERIOD INDEX
	SOUND33	:= 400FH	; LENGTH COUNTER / ENVELOPE RESTART
;--------------------------------------------------------------------------------------------
	SOUND40	:= 4010H	; FLAGS, PERIOD INDEX	DPCM
	SOUND41	:= 4011H	; DIRECT LOAD
	SOUND42	:= 4012H	; SAMPLE ADDRESS
	SOUND43	:= 4013H	; SAMPLE LENGTH
;--------------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------------
AR_2A031:
	ldy	#0	
	jmp	update2A03pulse
;--------------------------------------------------------------------------------------------
AR_2A032:
	ldy	#4
	
;============================================================================================
update2A03pulse:
;============================================================================================	
;	bvc	@no_noteon			; turn off sound on new note
;	sta	SOUND00, y			;
;@no_noteon:					;
;--------------------------------------------------------------------------------------------
	bcc	@no_pitch_update		; update pitch

	sTy	var2				;
	jsr	pitchToPeriod			;
	lDy	var2				;
	sta	SOUND02, y			;
	lda	var4				;
	
;	bit	t_flags				;
;	bvs	@force_set_pitch		;	
	cmp	apu_shadow_pulse1, y		;
	beq	@no_pitch_update		;
@force_set_pitch:				;
	sta	apu_shadow_pulse1, y		;
	ora	#%11111000			;
	sta	SOUND03, y			;
@no_pitch_update:				;
;--------------------------------------------------------------------------------------------
	lda	t_duty_env			; set duty+volume (+L,E)
	ora	t_volume			;
	sta	SOUND00, y			;
;--------------------------------------------------------------------------------------------
	rts
	
;--------------------------------------------------------------------------------------------
AR_2A03T:
	bvc	@no_noteon			; newnote: disable triangle
	jsr	@disable_tri			;
@no_noteon:					;
;--------------------------------------------------------------------------------------------
	bcc	@no_pitch_update		; update pitch if required
	jsr	pitchToPeriod			;
	sta	SOUND22				;
	lda	var4				;
	ora	#%11111000			;
	sta	SOUND23				;
@no_pitch_update:				;
;--------------------------------------------------------------------------------------------
	lda	t_volume			; activate if volume != 0
	beq	@disable_tri			;
	lda	#255				;
	sta	SOUND20				;
	rts					;
;--------------------------------------------------------------------------------------------
@disable_tri:					; deactivate if volume is 0
	lda	#128				;
	sta	SOUND20				;
	lda	#0				; do not touch carry
	sta	SOUND22				;
	sta	SOUND23				;
	rts					;

;--------------------------------------------------------------------------------------------
AR_2A03N:					; disable channel for newnote setup
	bvc	@no_note			;
	lda	#%1000				;
	sta	SOUND30				;
@no_note:					;
;--------------------------------------------------------------------------------------------
	lda	t_duty_env			; test for envelope
	bmi	@direct_pitch			;
;--------------------------------------------------------------------------------------------
@envelope_pitch:				; copy duty envelope into period index
	cmp	#16				;
	bcc	:+				;
	ora	#128				;
	and	#~16				;
:	sta	SOUND32				;
	lda	#%1111000			;
	sta	SOUND33				;
;--------------------------------------------------------------------------------------------
@setvolume:					; set volume
	lda	t_volume			;
	ora	#%110000			;
	sta	SOUND30				;
	rts					;
;--------------------------------------------------------------------------------------------	
@direct_pitch:					; set period from pitch
	asl	t_pitch				;
	lda	t_pitch+1			;
	rol					;
	asl	t_pitch				;
	rol					;
;--------------------------------------------------------------------------------------------	
	ldy	#0		;
	cmp	#119		; B9
	bcs	@0		;
	cmp	#107		; B8
	bcs	@1		;
	cmp	#95		; B7
	bcs	@2		;
	cmp	#83		; B6
	bcs	@3		;
	cmp	#71		; B5
	bcs	@4		;
	cmp	#64		; E5
	bcs	@5		;
	cmp	#59		; B4
	bcs	@6		;
	cmp	#55		; G4
	bcs	@7		;
	cmp	#51		; D#4
	bcs	@8		;
	cmp	#47		; B3
	bcs	@9		;
	cmp	#40		; E3
	bcs	@10		;
	cmp	#35		; B2
	bcs	@11		;
	cmp	#28		; E2
	bcs	@12		;
	cmp	#23		; B1
	bcs	@13		;
	cmp	#11		; B0
	bcs	@14		;
;--------------------------------------------------------------------------------------------	
	iny			; set y to certain value
@14:	iny			;
@13:	iny			;
@12:	iny			;
@11:	iny			;
@10:	iny			;
@9:	iny			;
@8:	iny			;
@7:	iny			;
@6:	iny			;
@5:	iny			;
@4:	iny			;
@3:	iny			;
@2:	iny			;
@1:	iny			;
@0:				;
;--------------------------------------------------------------------------------------------	
	tya				; add loop flag
	ldy	#sbi_noiseloop		;	
	ora	(var1), y		;
	sta	SOUND32			; set period index + loop flag
	lda	#%1111000		;
	sta	SOUND33			;
	jmp	@setvolume		; goto set volume routine

;--------------------------------------------------------------------------------------------
AR_2A03D:					; disable DPCM if volume is 0
	lda	t_volume			;
	bne	:+				;
@disable_dpcm:					;
	lda	#%1111				;
	sta	$4015				;
	rts					;
:						;
;--------------------------------------------------------------------------------------------
	bvc	@no_note			; test for note
;--------------------------------------------------------------------------------------------
	jsr	@disable_dpcm			; setup new note
	ldy	#sbi_dpcmbank			;
	lda	(var1), y			;
	sta	BCTRLE				;
	clc					;
	adc	#1				;
	sta	BCTRLF				;
	lda	#64				; todo: is DAC signed?
	sta	SOUND41				;
	iny					;
	lda	(var1), y			;
	sta	SOUND42				;
	iny					;
	lda	(var1), y			;
	sta	SOUND43				;
;--------------------------------------------------------------------------------------------
	jsr	@setperiod			; set pitch
;--------------------------------------------------------------------------------------------
	lda	#%11111				; enable dpcm
	sta	$4015				;
;--------------------------------------------------------------------------------------------
	rts					; return
;--------------------------------------------------------------------------------------------
@no_note:					; set pitch only
;--------------------------------------------------------------------------------------------
@setperiod:					; test duty env
	lda	t_duty_env			;	
	bmi	@direct_pitch			;
;--------------------------------------------------------------------------------------------
	cmp	#16				; test duty env bit4
	bcc	:+				; set: apply loop bit
	and	#~16				;
	ora	#%1000000			;
;--------------------------------------------------------------------------------------------
:	sta	SOUND40				; set period index
	rts					;
;--------------------------------------------------------------------------------------------
@direct_pitch:					; create period index from pitch
	lda	t_flags				;
	lsr					;
	bcc	@nopitch			;
	
	ldy	#0
;--------------------------------------------------------------------------------------------
	lda	t_pitch+1			; a = note
	asl	t_pitch				;	
	rol					;
	asl	t_pitch				;
	rol					;
;--------------------------------------------------------------------------------------------
	cmp	#4*12+2
	bcc	@setlowest
	cmp	#7*12
	bcs	@sethighest

	tay
	lda	@dpcm_map-(4*12+2), y
	
@setpitch:
	ldy	#sbi_dpcmloop
	ora	(var1), y
	sta	SOUND40
	
@nopitch:
	rts
@setlowest:
	lda	#0
	beq	@setpitch
@sethighest:
	lda	#15
	bne	@setpitch
@dpcm_map:
	.byte	       1, 1, 2, 3, 3, 4, 4, 5, 5, 6
	.byte	 7, 7, 8, 8, 8, 9, 9,10,10,11,11,11
	.byte	12,12,12,12,13,13,13,14,14,14,14,14
	
	
tableDiv3:
	.byte	0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,10,11,11,11,12,12,12,13,13,13,14,14,14,15,15,15
	
;********************************************************************************************
;*
;* VRC6
;*
;********************************************************************************************

	VRC690	:=	09000H		; pulse 1
	VRC691	:=	09001H		;
	VRC692	:=	09002H		;
	VRC6A0	:=	0A000H		; pulse 2
	VRC6A1	:=	0A001H		;
	VRC6A2	:=	0A002H		;
	VRC6B0	:=	0B000H		; sawtooth
	VRC6B1	:=	0B001H		;
	VRC6B2	:=	0B002H		;
	
;--------------------------------------------------------------------------------------------
AR_VRC61:
	ldy	#>VRC690
;	sty	var6
;	clc
	bne	updateVRC6pulse ;(bra)
;--------------------------------------------------------------------------------------------
AR_VRC62:
	ldy	#>VRC6A0
updateVRC6pulse:
	sty	var6	
	clc
	
;--------------------------------------------------------------------------------------------
updateVRC6sound:
;--------------------------------------------------------------------------------------------
	sta	var5
	bvc	@no_newnote			; newnote: disable channel
	ldy	#2				;
	sta	(var5), y			;
@no_newnote:					;
;--------------------------------------------------------------------------------------------

	bcc	@pulse_sound			; 
	ldy	t_volume
	lda	@sawVolumeMap, y
	jmp	@saw_sound
@pulse_sound:
	lda	t_volume			; set duty+volume
	ora	t_duty_env			;
@saw_sound:
	ldy	#0				;
	sta	(var5), y			;

	lda	t_flags
	lsr
	bcc	@dont_update_pitch
;--------------------------------------------------------------------------------------------
	jsr	pitchToPeriod			; set pitch
	ldy	#1				;
	sta	(var5), y			;
	iny					;
	lda	var4				;
	ora	#%10000000			;
	sta	(var5), y			;
;--------------------------------------------------------------------------------------------
@dont_update_pitch:
	rts
	
;--------------------------------------------------------------------------------------------
@sawVolumeMap:
;--------------------------------------------------------------------------------------------
	.byte 00h, 03h, 06h, 08h, 0Bh, 0Eh, 11h, 14h, 16h, 19h, 1Ch, 1Fh, 22h, 24h, 27h, 2Ah
	
;--------------------------------------------------------------------------------------------
AR_VRC6S:
;--------------------------------------------------------------------------------------------
	ldy	#>VRC6B0			; map to sawtooth channel
	sty	var6				;
	sec					;
	bcs	updateVRC6sound			;(bra)

;********************************************************************************************
;*
;* N106
;*
;********************************************************************************************

	N106A :=	0F800H
	N106D :=	04800H

;--------------------------------------------------------------------------------------------
AR_N106x:
;--------------------------------------------------------------------------------------------
	lda	mod_chmap, x				; a = channel address (40h + ch*8)
	sec						;
	sbc	#8					;
	asl						;
	asl						;
	asl						;
	adc	#80h+40h				;
;--------------------------------------------------------------------------------------------
	bit	t_flags					; noteon: volume=0	
	bvc	@no_noteon				;
	ora	#7					;
	sta	N106A					;
	ldy	#%1110000				;
	sty	N106D					;
	and	#~7
@no_noteon:						;
;--------------------------------------------------------------------------------------------
							; set frequency (x<<3) (super long)
	sta	N106A					; and length (4*4)

	jsr	pitchToFrequency			; (and strange top 3 bits)

	ldy	#%111100				;
	sty	var5					;
	asl						;
	rol	var4					;
	asl						;
	rol	var4					;
	rol	var5					;
	asl						;
	rol	var4					;
	rol	var5					;
	
	sta	N106D					; FREQ L
	sta	N106D					;(increment 2x)
	
	ldy	var4					;
	lda	var5					;
	sty	N106D					; FREQ M
	sty	N106D					;(increment 2x)
	sta	N106D					; FREQ H, LEN, "???"
	sta	N106D					;(increment 2x)
;--------------------------------------------------------------------------------------------
	ldy	t_duty_env				; set wavetable index
	sty	N106D					;
;--------------------------------------------------------------------------------------------
	lda	t_volume				; set volume
	ora	#%01110000				;(8 channels
	sta	N106D					;
;--------------------------------------------------------------------------------------------

	rts

;********************************************************************************************
;*
;* VRC7
;*
;********************************************************************************************

	VRC7A :=	9010H
	VRC7D :=	9030H
	
;============================================================================================
AR_VRC7x:
;============================================================================================

	lda	mod_chmap, x				; var5 = channel#
	sec						;
	sbc	#16					;
	sta	var5					;
;--------------------------------------------------------------------------------------------	
	bit	t_flags					; reset channel on keyon
	bvc	@no_keyon				;
	ora	#20h					;
	sta	VRC7A					;
	lda	#0					;
	sta	VRC7D					;
;--------------------------------------------------------------------------------------------
	ldy	#sbi_type				; test for custom instrument
	lda	(var1), y				;
	cmp	#8					;
	bne	@no_custom_instrument			;
;--------------------------------------------------------------------------------------------
	;bit	t_flags					; if noteon:
	;bvc	@no_custom_instrument			;   copy custom instrument 
	lda	var1					;
	adc	#sbi_vrc7custom-1			;
	sta	var3					;
	lda	var2					;
	adc	#0					;
	sta	var4					;
	ldy	#7					;
:	sty	VRC7A					;
	lda	(var3), y				;
	sta	VRC7D					;
	dey						;
	bpl	:-					;
@no_custom_instrument:					;
@no_keyon:						;
;--------------------------------------------------------------------------------------------
	lda	var5					; set volume & instrument
	ora	#30h					;
	sta	VRC7A					;
	lda	t_volume				;
;--------------------------------------------------------------------------------------------
	bne	@has_volume				; zero volume: reset freq, leave ctrig
	lda	var5					;
	ora	#10h					;
	sta	VRC7A					;
	ldy	#0					;
	sty	VRC7D					;
	clc						;
	adc	#10h					;
	sta	VRC7A					;
	ldy	#%10000					;
	sty	VRC7D					;
	rts						;
@has_volume:						;
;--------------------------------------------------------------------------------------------
	eor	#15					; set volume
	tay						;
	lda	@vrc7_vmap, y				;
	ora	t_duty_env				;-add instrument index
	sta	VRC7D					;
;--------------------------------------------------------------------------------------------
	lda	var5
;--------------------------------------------------------------------------------------------
	ora	#20h					; addr = pitch H/trigger
	sta	VRC7A					;
;--------------------------------------------------------------------------------------------
	jsr	pitchToFrequency			; calc pitch
	ldy	var4					;
	cpy	#2					;
	bcc	@smallfreq				;
	ldy	#0					;
@shiftagain:						;
	lsr	var4					;
	beq	@shiftcomplete				;
	ror						;
	iny						;
	jmp	@shiftagain				;
@shiftcomplete:						;
	rol	var4					;
	jmp	@largefreq				;
@smallfreq:						;
	ldy	#0					;
@largefreq:						;
;--------------------------------------------------------------------------------------------
	pha						; set frequency
	tya						;
	asl						;
	ora	var4					;
;--------------------------------------------------------------------------------------------
	pha						; ctrig = noteoff ? 0:1
	lda	#UF_NOTEOFF				;
	bit	t_flags					;
	beq	@no_keyoff				;
	pla						;
	jmp	@keyedoff				;
@no_keyoff:						;
	pla						;
	ora	#%10000					;
@keyedoff:						;
;--------------------------------------------------------------------------------------------
	sta	VRC7D					; 
	lda	var5					;
	ora	#10h					;
	sta	VRC7A					;
	pla						;
	sta	VRC7D					;
;--------------------------------------------------------------------------------------------
	rts
	
@vrc7_vmap:
	.byte 0+2, 0+2, 0+2, 0+2, 1+2, 1+2, 1+2, 1+2, 2+2, 2+2, 3+2, 3+2, 4+2, 5+2, 6+2, 8+2 

;********************************************************************************************
;*
;* MMC5
;*
;********************************************************************************************
	
	MMC500	:= 5000H
	MMC502	:= 5002H
	MMC503	:= 5003H
	MMC510	:= 5004H
	MMC511	:= 5006H
	MMC512	:= 5007H
	
;--------------------------------------------------------------------------------------------
AR_MMC51:
	ldy	#0
	beq	updateMMC5pulse ;(bra)
;--------------------------------------------------------------------------------------------
AR_MMC52:
	ldy	#4
	
;============================================================================================
updateMMC5pulse:
;============================================================================================	

	bvc	@no_noteon			; turn off sound on new note
	sta	MMC500, y			;
@no_noteon:					;
;--------------------------------------------------------------------------------------------
	bcc	@no_pitch_update		; update pitch
	sty	var2				;
	jsr	pitchToPeriod			;
	ldy	var2				;
	sta	MMC502, y			;
	lda	var4				;
	bit	t_flags				;
	bvs	@force_set_pitch		;	
	cmp	apu_shadow_mmc51, y		;
	beq	@no_pitch_update		;
@force_set_pitch:				;
	sta	apu_shadow_mmc51, y
	ora	#%11111000			;
	sta	MMC503, y			;
@no_pitch_update:				;
;--------------------------------------------------------------------------------------------
	lda	t_duty_env			; set duty+volume (+L,E)
	ora	t_volume			;
	sta	MMC500, y			;
;--------------------------------------------------------------------------------------------

	rts
	
;********************************************************************************************
;*
;* FME-07
;*
;********************************************************************************************

	FME7_ADDR :=	0C000H
	FME7_DATA :=	0E000H
	
.macro updateFME7 index
	lda	#8+index
	jsr	fme_offIfNoteon
	lda	#index*2
	jsr	fme_setPitch
	lda	#8+index
	sta	FME7_ADDR
	lda	t_volume
	sta	FME7_DATA
	rts
.endmacro
	
;--------------------------------------------------------------------------------------------
AR_FME71:
	updateFME7 0
;--------------------------------------------------------------------------------------------
AR_FME72:
	updateFME7 1
;--------------------------------------------------------------------------------------------
AR_FME73:
	updateFME7 2
	
;--------------------------------------------------------------------------------------------
fme_offIfNoteon:
;--------------------------------------------------------------------------------------------

	bvc	:+					; zero volume if V
	sta	FME7_ADDR				;
	lda	#0					;
	sta	FME7_DATA				;
:	rts						;

;--------------------------------------------------------------------------------------------
fme_setPitch:
;--------------------------------------------------------------------------------------------
	sta	var2					; calc/set pitch if flag is set
	lda	t_flags					;
	lsr						;
	bcc	@skip					;
	jsr	pitchToPeriod				;
	ldy	var2					;
	sty	FME7_ADDR				;
	sta	FME7_DATA				;
	iny						;
	sty	FME7_ADDR				;
	lda	var4					;
	sta	FME7_DATA				;
@skip:	rts						;
	
;********************************************************************************************
;*
;* FDS EXPANSION
;*
;********************************************************************************************

	FDSVOLUME	:= $4080
	FDSFREQL	:= $4082
	FDSFREQH	:= $4083
	FDSWAVETABLE	:= $4040
	FDSWAVECTRL	:= $4089

;--------------------------------------------------------------------------------------------
AR_FDSEX:
;--------------------------------------------------------------------------------------------

	bvc	@no_keyon			; disable on new note
	lda	#128				;
	sta	FDSVOLUME			;
@no_keyon:					;
;--------------------------------------------------------------------------------------------
	lda	ch_instrument, x		; test if instrument matches with loaded
	cmp	fds_instrument			;
	beq	@instrument_matches		;
;--------------------------------------------------------------------------------------------
	sta	fds_instrument			;
;--------------------------------------------------------------------------------------------
	lda	#%10000000			; enable waveform write
	sta	FDSWAVECTRL			;
;--------------------------------------------------------------------------------------------

	ldy	#63				; copy waveform
:	lda	(var3), y				;
	sta	FDSWAVETABLE, y			;
	dey					;
	lda	(var3), y				;
	sta	FDSWAVETABLE, y			;
	dey					;
	lda	(var3), y				;
	sta	FDSWAVETABLE, y			;
	dey					;
	lda	(var3), y				;
	sta	FDSWAVETABLE, y			;
	dey					;
	bpl	:-
	
	lda	#0
	sta	FDSWAVECTRL
;--------------------------------------------------------------------------------------------
@instrument_matches:				; set frequency
	lda	t_pitch				;
	lsr					;
	bcs	@freq_cached			;
	jsr	pitchToFrequency		;
	sta	FDSFREQL			;
	lda	var4				;
	ora	#%01000000			; disable sweep
	sta	FDSFREQH			;
@freq_cached:					;
;--------------------------------------------------------------------------------------------
	lda	t_volume			; set volume
	asl					;
	asl					;
	ora	#128				;
	sta	FDSVOLUME			;
;--------------------------------------------------------------------------------------------
	rts

	
;============================================================================================
pitchToPeriod:
;============================================================================================
; returns var4 = periodH
; a = periodL
	ldy	t_pitch+1			; var3/a = octave
	lda	tableDiv3, y			;
	sta	var3				; 
;--------------------------------------------------------------------------------------------
	lda	t_pitch+1			; y = note (0..191)
	sec					;
	sbc	var3				;
	sbc	var3				;
	sbc	var3				;
	lsr					;
	ror	t_pitch				;

	lsr
	lda	t_pitch
	ror
	bcs	@interpolate
;---------------------------------------------------------------------------------------------
	clc					; read ftab[192-p] / 8
	sbc	#192				;
	eor	#255				;
	tay					;
	lda	frequencyTableH, y		;
	lsr					;
	sta	var4				;
	lda	frequencyTableL, y		;
	ror					;
	lsr	var4				;
	ror					;
	lsr	var4				;	
	ror					;
	jmp	@gotperiod			;
;---------------------------------------------------------------------------------------------
@interpolate:					; interpolated value
	clc					;
	sbc	#192				;
	eor	#255				;
	tay					;
	lda	frequencyTableL, y		;
	adc	frequencyTableL-1, y		;
	pha					;
	lda	frequencyTableH, y		;
	adc	frequencyTableH-1, y		;
	lsr					;
	sta	var4				;
	pla					;
	ror					;
;---------------------------------------------------------------------------------------------
	lsr	var4				; /8
	ror					;
	lsr	var4				;
	ror					;
	lsr	var4				;
	ror					;
;---------------------------------------------------------------------------------------------
@gotperiod:
	
	ldy	var3
	
_var4_a_shift_y:
	cpy	#0
	beq	@skip_shift
	cpy	#8
	bcs	@greater8
	and	Table_1_ASL_n_mask-1, y
	ora	Table_1_ASL_n-1, y
	
@shift_more:
	lsr	var4
	ror	
	bcc	@shift_more
@skip_shift:
	rts
	
@greater8:
	lda	var4
:	cpy	#8
	beq	@shift_exit2
	lsr
	dey
	jmp	:-
@shift_exit2:
	ldy	#0
	sty	var4
	rts

;============================================================================================
pitchToFrequency:
;============================================================================================

	ldy	t_pitch+1			; var3/a = octave
	lda	tableDiv3, y			;
	sta	var3				;
;--------------------------------------------------------------------------------------------
	lda	t_pitch+1			; y = note (0..191)
	sec					;
	sbc	var3				;
	sbc	var3				;
	sbc	var3				;
	lsr					;
	ror	t_pitch				;
;--------------------------------------------------------------------------------------------
	lsr
	lda	t_pitch
	ror
	tay
	bcs	@interpolate
;--------------------------------------------------------------------------------------------
	lda	frequencyTableH, y		; direct value
	sta	var4				;
	lda	frequencyTableL, y		;
	jmp	@gotperiod
;--------------------------------------------------------------------------------------------
@interpolate:					; interpolated value
	lda	frequencyTableL, y		;
	adc	frequencyTableL+1, y		;
	pha					;
	lda	frequencyTableH, y		;
	adc	frequencyTableH+1, y		;
	lsr
	sta	var4				;
	pla					;
	ror					;
@gotperiod:					;
;--------------------------------------------------------------------------------------------
	pha					; y = 15 - octave
	lda	var3				;
	eor	#15				;
	tay					;
	pla					;
	jmp	_var4_a_shift_y			;
