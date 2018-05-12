# IT2NSF MANUAL

IT2NSF is a conversion program that accepts special IT (Impulse Tracker
module) files which contain instrument data specified with a form of MML and
then produces NSF (NES sound format) music files with it.

## Composing a song

To write a song there are a few things to watch out for. First you must keep
the instrument types limited to the channels they are compatible with. The
beginning example of the MML chapters explains the 'map' and compatible
instrument types. If you just want to compose a song with a pack that you have
acquired you only need to know about this 'channel mapping'.

The samples and instruments (which are generated) in the module are NOT used
during conversion, so they should not be changed (only the MML data can be
changed). The generated samples and instruments are used for composition
purposes only.

### Other things in IT that are not supported

* Global Volume
* Channel Volume (different from channel's 'volume level')
* Panning (NES sound is MONO)
* Pattern Effects:
  * Cxx (C00 is supported though)
  * Lxx
  * Mxx (no channel volume)
  * Nxx (no channel volume)
  * Oxx (maybe supported later somehow (for dpcm))
  * Pxx (mono sound)
  * S3x (vibrato control)
  * S4x (tremolo control)
  * S5x (panbrello control)
  * S6x (fine patt delay)
  * S7x (instr control)
  * S8x (no panning)
  * S9x (sound control)
  * SAx (high offset
  * SBx (pattern loop)
  * SEy (maybe supported soon)
  * Uxx (just use vibrato)
  * Vxx (no global volume)
  * Wxx (no global volume)
  * Xxx (no panning)
  * Yxx (no panning)
  * Zxx (wut)

Only volume commands that affect the volume are supported.

### Things that ARE supported and/or MUST be used

* Variable pattern length
* Linear Pitch (must be used, amiga mode is not supported)
* +++ orders (+++ is like ropes in the pool, that tell you to stay out of the deep end because you're a little kid and can't swim for shit)

### Things that you should NOT do

* Use instruments that don't exist.
* Bxx to an invalid position.

### About tempo

Tempo 150 is the best, because then 1 tick equals 1 NES frame. Tempos less
than 150 are also okay. Tempos above 150 should be avoided and will cause
double-updates (two updates in one frame to keep up with the song, eating
up cpu and skipping ticks).

### Another slight warning

The final pitch is not clipped, sliding too far may result in an undefined
pitch setting.

## NOISE and DPCM notes

NOISE and DPCM can have period index envelopes. If they do then the note they
are played at is ignored (must be C-5 to play the right sound).

Otherwise, without an envelope to control the period index, here are the notes
with their corresponding frequencies that may be used. (index is the
corresponding envelope value for when you are making an envelope.)

### NOISE

| NOTE | INDEX | FREQUENCY | NOTE | INDEX | FREQUENCY        |
|------|-------|-----------|------|-------|------------------|
| C-0  | 15    | 439.96 Hz | G-4  | 7     | 11.186 kHz       |
| B-0  | 14    | 879.93 Hz | B-4  | 6     | 13.983 kHz       |
| B-1  | 13    | 1761.6 Hz | E-5  | 5     | 18.643 kHz       |
| E-2  | 12    | 2348.8 Hz | B-5  | 4     | 27.965 kHz       |
| B-2  | 11    | 3523.2 Hz | B-6  | 3     | 55.930 kHz       |
| E-3  | 10    | 4709.9 Hz | B-7  | 2     | 111.86 kHz       |
| B-3  | 9     | 7046.3 Hz | B-8  | 1     | 223.72 kHz       |
| D#4  | 8     | 8860.3 Hz | B-9  | 0     | 447.44 kHz (heh) |

### DPCM

| NOTE | INDEX | FREQUENCY | NOTE | INDEX | FREQUENCY  |
|------|-------|-----------|------|-------|------------|
| C-4  | 0     | 4181.7 Hz | D-5  | 8     | 9419.9 Hz  |
| D-4  | 1     | 4709.9 Hz | F-5  | 9     | 11.186 kHz |
| E-4  | 2     | 5264.0 Hz | G-5  | 10    | 12.604 kHz |
| F-4  | 3     | 5593.0 Hz | A-5  | 11    | 13.983 kHz |
| G-4  | 4     | 6257.9 Hz | C-6  | 12    | 16.885 kHz |
| A-4  | 5     | 7046.3 Hz | E-6  | 13    | 21.307 kHz |
| B-4  | 6     | 7919.3 Hz | G-6  | 14    | 24.858 kHz |
| C-5  | 7     | 8363.4 Hz | C-7  | 15    | 33.144 kHz |

You can slide the pitch with Exx/Fxx too but it will still be limited to the
16 frequencies.

## Writing the MML

The MML data is written in the IT song message. Here is an example song
message which we will examine:

```mml
here is my great song
(c) 2009 foobar

[[IT2NSF]]

map squ sq2 tri nse dmc

instr my square wave
type PULSE
duty 0 0 1 2
vol 16 13 12

instr triwave
type TRI

instr bass drum
type DPCM
sample 1

instr gyruss noise snare
type NOISE
duty 14 14 6 4 3 4 3 2 0
vol 14 14 8 3 4 3 1 1 1 0
```

The MML parser ignores all text until it finds the "[[IT2NSF]]" marker. After
this marker the MML commands follow. All of the MML commands are in this
format:

```mml
[command] <arg1> <arg2> <arg3> ...
```

The first command we see is the 'map' command. This command maps certain sound
hardware to the module channels. In this case, the first channel will be
mapped to 2A03 square wave #1 ("SQU"), the second to square wave #2 ("SQ2"),
third to the 2A03 triangle wave ("TRI"), fourth to the 2A03 noise channel
("NSE"), and fifth to the 2A03 DPCM channel ("DMC"). Each hardware sound
channel can only be mapped to one module channel. Here is a list of all of the
hardware channels:

| Short | Channel Name                         |
|-------|--------------------------------------|
| SQU   | 2A03 Pulse channel 1                 |
| SQ2   | 2A03 Pulse channel 2                 |
| TRI   | 2A03 Triangle wave channel           |
| NSE   | 2A03 Noise channel                   |
| DMC   | 2A03 DPCM channel                    |
| V61   | VRC6 pulse 1                         |
| V62   | VRC6 pulse 2                         |
| V6S   | VRC6 sawtooth                        |
| N6x   | N106 channel #x (replace x with 1-8) |
| V7x   | VRC7 channel #x (replace x with 1-6) |
| MMC   | MMC5 pulse 1                         |
| MM2   | MMC5 pulse 2                         |
| F7x   | FME7 pulse #x (replace x with 1-3)   |
| FDS   | FDS Sound                            |

Up to 16 channels may be defined; any more will be ignored/crash something.
The selected expansion chips will automatically be added to the NSF.

The next command we see defines a new instrument. The "instr" command takes
a single argument which is the name of the instrument (may contain spaces).
The name will be given to the generated instrument. The first instrument
command+subcommands will define the behavior of the first instrument in the
module. The following instr commands will define the behavior for the
following instruments in the module. IT2NSF has a 'pack creation' mode that
generates instruments in the IT module with the MML data to be used for
composition.

Each instrument must have a TYPE assigned to it. Instruments of certain TYPES
must be only used in compatible channels. (ie PULSE in SQU or SQ2, TRI in TRI,
NOISE in NSE). Here's a list of the instrument types (and compatible channels)

| Short     | Instrument Type              |
|-----------|------------------------------|
| PULSE     | 2A03 square wave (SQU,SQ2)   |
| TRI       | 2A03 triangle wave (TRI)     |
| NOISE     | 2A03 noise (NSE)             |
| DPCM      | 2A03 DPCM instrument (DMC)   |
| VRC6PULSE | VRC6 square wave (V61,V62)   |
| VRC6SAW   | VRC6 sawtooth wave (V6S)     |
| N106      | N106 sound (N6x)             |
| VRC7      | VRC7 preset instrument (V7x) |
| VRC7C     | VRC7 custom instrument (V7x) |
| MMC5      | MMC5 square wave (MMC,MM2)   |
| FME7      | FME-07 square wave (F7x)     |
| FDS       | FDS sound instrument (FDS)   |

The "duty" and "vol" commands define ENVELOPES for an instrument. Envelopes
are processed once every frame.

The duty envelope for the "my square wave" instrument will cause the duty of
the square wave to be 12.5% (1) for the first two frames, 25% for the third
frame, and then 50% for frames past the third.

The volume envelope changes the volume scale of the sound per-frame. Each
entry ranges from 0..16, where 0 is silent and 16 is the original volume.

The second instrument defined is for the 2A03 triangle channel. Just a basic
triangle sound.

The third instrument defined is for the 2A03 dpcm channel. The 'sample'
command selects a sample in the module that will be converted to 1-bit DPCM
for the instrument, so in this case the first sample in the module will be
converted to DPCM and then played when this instrument appears in the DMC
mapped channel.

The fourth/last instrument defined is for the 2A03 noise channel. For the
noise channel, the duty envelope controls the period index, where 15 selects
the lowest period and 0 selects the highest period. Combined with the volume
envelope this instrument should try to mimic the sound of a snare drum.

After you have written the MML data for a module you can use the --pack option
with IT2NSF to fill the module with 'preview' instruments:

```cmd
C:\nes>it2nsf --pack mysong.it
```

## Instrument Envelopes

There are three types of envelopes that can be used with the instruments:
volume, duty, and pitch.

The volume and duty envelopes were described briefly in the above section.

The pitch envelope is another type of envelope that controls the pitch of the
channel. The pitch envelope contains entries that range from -32 to +31.5
semitones (you can put numbers with decimal places.) The resolution of the
decimal places is limited to .25, .50 and .75.

The envelopes have a loop point which is the position from where the envelope
resumes after having reached the end. By default the last entry is looped
(appears as just holding the last entry). The loop point can be set with a '|'
in the envelope data:

```mml
vol 2 3 4 5 5 | 6 6 5 5
```

This envelope will excecute as "2 3 4 5 5" and then "6 6 5 5 6 6 5 5..."
repeated.

And an example for a 'major' arpeggio with the note switching every 2 frames:

```mml
pitch | 0 0 4 4 7 7
```

There is also another special loop which is only active while the note is
'held' issueing a 'note-off' in the pattern data cancels this loop. The
beginning of the 'sustain loop' is set with the '{' character, and the end is
set with the '}' character. WARNING: The start and end of the sustain loop
must remain on the same tick for all envelopes (for proper sample generation!)

(example which follows the sustain start/end warning)

```mml
vol 16 15 14 { 13 13 13 } 12 11 10 9 8 7 6 5 4 3 2 1 0
duty 0 0 1 { 2 2 1 } 2
```

The duty envelope is used for more than just controlling the wave duty cycle
of square waves. Here is it's behavior for each instrument type:

|Instrument Type|Behavior|
|---------|---------|
|PULSE,MMC|Selects the duty cycle (0..3) (12.5%, 25%, 50%, 75%)|
|NOISE,DPCM|elects period index (0..15)noise 0=highest, dpcm 0=lowest (see tables below for notes) Also: add 16 to noise duty for SHORT noise (16..31)|
|VRC6PULSE|Selects the duty cycle (0..7) ((1+x)*6.25%)|
|N106|Selects the wavetable index (0..63). The wavetable can be overwritten but here are the indexes for the default table: 0..7: square wave ((1+d) * 6.25% duty) 24: triangle wave 32..39: sawtooth wave (with varying softness)|
|VRC7|Selects the instrument index. Typically you just use one value, but the instrument index can be enveloped too...|
|TRI,VRC6SAW,VRC7C,FME7,FDS|The duty envelope is not and must not be used.|

## Instrument Modulation

All instruments (except for [pitchless] NOISE and DPCM) can have modulation
(auto-vibrato) applied to them. Four commands setup the modulation parameters.

```mml
mdel [1..256]
```

Sets the modulation delay. This is the number of ticks that are passed before
the modulation starts its process.

```mml
msw [1..256]
```

Sets the modulation 'sweep'. This controls the speed of the modulation
reaching its specified depth level. The time for the modulation depth to slide
from 0 to the desired level will be "s/60*d seconds", where 'd' is the desired
depth level and 's' is the sweep value.

```mml
mdep [0..15]
```

Sets the desired modulation depth level. The depth slides from 0 to this level
during the 'sweep' phase, and then it continues at this constant. The depth
value ranges from 0..15 representing "+- 0 to 0.94 semitones".

```mml
mrate [0..64]
```

Sets the speed/frequency of the modulation cycle. (how fast it 'vibrates')

## Complete Command List

```mml
map <channel map>
```

Sets the hardware channel mapping. (see beginning MML example for explanation)

```mml
n106 <n106 index> <sample index>
```

Overwrites n106 wavetable starting at `<index>` samples with the contents of the
sample pointed to by `<sample index>`. The n106 wavetable is 128 samples wide and
the waveform length of the n106 channels is 16 samples. ('duty' 0-127 selects
the start sample, and then it can be changed during the envelope)

```mml
instr <name>
```

Starts creation of a new instrument with the specified name.

```mml
type <instrument type>
```

Sets the type of the current instrument being created. This command must be
issued before other instrument commands.

```mml
vol <envelope>
pitch <envelope>
duty <envelope>
```

Adds and envelope to an instrument. See the "Instrument Envelopes" chapter.
Some instruments require a duty envelope.

```mml
mdel <modulation delay>
msw <modulation sweep>
mdep <modulation depth>
mrate <modulation rate>
```

Sets the instrument modulation parameters. See the "Instrument Modulation"
chapter.

```mml
sample <sample index>
```

Sets the sample index (1=first sample in module) for DPCM and FDS instruments.
FDS samples should be 64 samples long (and they are converted to 6-bit). DPCM
samples can be much longer and they are converted to 1-bit DPCM.

```mml
fm <1> <2> <3> <4> <5> <6> <7> <8>
```

Sets the parameters for a VRC7 custom instrument. Each parameter is one byte
and may be specified in either decimal or hex. (append 'h' to number for hex)

```mml
defvol <default volume>
```

Sets the default volume level for an instrument. This must be used to change
the default volume level for the generated samples. The default volume
specified in the generated samples is not used during conversion.

```mml
detune <semitones>
```

Detunes the current instrument by an amount of semitones (can also use decimal
places (like 0.05)).

```mml
shortnoise
```

For NOISE instruments that don't use duty envelopes, this sets loop flag which
makes it sound like ass.

## Credits

IT2NSF is brought to you by:

* mukunda (6502 driver, conversion program)
* coda (VRC7 help, sharing his genius)
* madbrain (additional (VRC7) help)
* reduz (s3m2nsf dude)