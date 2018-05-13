# Guide

Hello, welcome to the IT2NSF MML-ish guide!

To specify data, you type a command name, followed by 0 or more arguments.

Example: `map SQU SQ2 TRI NSE DMC`

Here the command is "map" which sets up the channel->hardware map. See below for more info on the command.

## Mapping channels

The MML data is stored in the song message of the IT module. To invoke the mml parser you place the string "[[IT2NSF]]" in the song message. The following text will be treated as special MML data and parsed into the conversion process.

Comments (ignored data) may be stored anywhere in the mml file by prefixing the line with a "#".

The "map" command is required, and must be specified in all IT2NSF modules.

The example shown above maps channels 1 and 2 to the 2A03 pulse channels 1 and 2 (SQU, SQ2), channel 3 to the 2A03 "triangle" channel (TRI), channel 4 to the 2A03 noise channel (NSE), and channel 5 to the 2A03 DPCM channel (DMC).

Here is the full list of mappable channels:

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

The module may use upto 16 channels ONLY. Any more will be ignored/crash the conversion program [maybe].

## Programming instruments

To create instrument articulation info, use the 'instrument' command followed by instrument specific commands.

example instrument info:

```
# create instrument with 'myinstr' as name
instrument myinstr

# instrument type (REQUIRED)
type PULSE

# volume envelope (OPTIONAL)
volume 5 9 16

# pitch envelope (OPTIONAL)
pitch -0.5 -0.2 -0.1 0

# duty envelope (MAY BE REQUIRED)
duty 1 2
```

## Instrument articulation commands

### type

This is a required command that specifies what type an instrument is. Here is a table of the types that can be used:

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

### index

This command sets the current sample index. (next instrument will continue with this sample index + 1). By default,
the first sample is used, followed by the second, third, etc.

### mdelay [1..256]

Sets the delay before the instrument modulation begins. The time is specified as 1..256 which represents t/60 seconds.

### msweep [1..256]

Sets the sweep time for the instrument modulation. This 'period' controls the time it takes for the modulation depth to reach its desired depth from zero. The time is n/60*d seconds, where d is the desired depth level for the modulation.

### mdepth [0..15]

Sets the target instrument modulation depth. A value of 0 is the default implied value and will disable the modulation. The internal depth level for a channel is reset on new notes and then is slid towards this level during the sweep period. The depth value ranges from 0..15 representing 0 to +-0.94 semitones.

### mrate [0..64]

Sets the rate/speed of the instrument modulation. (how fast it "vibrates")

### sample <sample index>

Sets the sample used by the instrument. Valid for DPCM and FDS instruments only. Note that FDS sounds should be 64 samples long and they will be converted to 6-bit audio. DPCM samples will be converted to 1-bit dpcm and they should be kept as short as possible to avoid making huegor NSF.

## Instrument envelopes

Envelopes may be applied to certain instrument parameters.

There are three types of envelopes, "volume", "pitch", and "duty". The duty envelope is abused and also used for other purposes then setting the duty cycle for square waves. The envelopes are updated by the player at 60hz (each node has an interval of around 16.7ms).

Example volume envelope:
volume 16 15 14 { 13 13 12 12 } 11 8 | 4 2 2 3

The values of the volume envelope range from 0->16. The channel volume is scaled by this value (c*e/16). The {} marks in the envelope mark the sustain start/end points. Envelope sustain is optional. The sustain loop is terminated when the module channel encounters a key-off note "==". The pipe | marks the loop point. By default the last value will be looped, but the | symbol can be used to move the loop point to somewhere earlier in the envelope.

All instrument types except for DPCM can accept volume enveloples. The volume envelope for 2A03 triangle waves should contain values 16 and 0 only, because the triangle volume level is either ON or OFF.

The next type of envelope controls the pitch.

Example pitch envelopes:

```
# slide up from -0.5 semitones
pitch -0.5 -0.25 0
```

```
# minor arpeggio
pitch | 0 3 7
```

The pitch envelope values range from -32 to +31.25 semitones. All instruments except for NOISE and DPCM support the pitch envelope.

Finally there is the duty envelope. An example (for pulse):

```
duty 3 2 1 0 | 0 0 1 1 2 2 2 2 1 1 0 0
```

The duty envelope controls different parts of the hardware depending on the instrument type.

### PULSE

The duty envelope ranges from 0..3 and sets the square wave duty. (12.5%, 25%, 50%, 75%)

### NOISE

The duty envelope ranges from 0..31 and sets the noise period index (pitch!). Values 0..15 set period index 0..15 and values 16..31 set the same period index with the 'loop' flag also set.

### DPCM

The duty envelope ranges from 0..15 and sets the dpcm period index (pitch!).

### VRC6PULSE

The duty envelope ranges from 0..7 and sets the square wave duty. ((1+x)*6.25%)

### N106

The duty envelope controls the wavetable index. A bit complicated, here are the values for the default wavetable:

0..7: square wave ((1+d) * 6.25% duty)
24: triangle wave
32..39: sawtooth wave (with varying softness)

Custom wavetable entries may be added with the "n106wt" command.

Note that n106 instruments are fixed to 16 sample width.

### VRC7

The duty envelope controls the instrument index ... somehow. It ranges from 1..15.

If you're not doing something crazy (like sliding through instrument types), you can use the vrc7 specific "preset" command: `preset [1..15]`

The preset command is the same as specifying a single looped envelope value.

### MMC5

Same as PULSE.

### Other

The duty envelope is not used.

## VRC7 custom instrument

There is one more special command for the "VRC7C" instrument type:

```
fm [byte] [byte] [byte] [byte] [byte] [byte] [byte] [byte]
```

This command defines the custom instrument entry (8 bytes) for the VRC7 chip.

WARNING: Only one custom instrument may be used at a time. If two channels use two different custom instruments at the same time, then one of them will be overwritten with the options of the other channel.

## Other commands

```
n106 [index] [sample index]
```

This command (top-level command, like 'map') inserts data from a sample into the n106 wavetable at a certain position. There is a default wavetable that uses samples 0-55, leaving 56-127 for custom sounds. The wavetable contains 4-bit data, and the input sample will be down-converted to this format.

## Basic instruments

To create basic instruments without hassle, there are some special marks you can put in your samples (as the dos filename or sample name).

The sample/macro generating tool will generate a basic waveform if the sample does not supply any data.

|Name  |Description  |
|---------|---------|
|%%PULSEx|2A03 pulse instrument with duty 'x'. (ie %%PULSE0..%%PULSE3).|
|%%TRI|2A03 triangle instrument.|
|%%NOISE|2A03 noise instrument.|
|%%DPCM|2A03 dpcm instrument. The sample data will be used as the dpcm sample.|
|%%N106xx|N106 instrument with wavetable index X. (sound should be 16 samples wide for proper tuning)|
|%%VRC6x|VRC6 square wave with duty x (0..7).|
|%%VRC6SAW|VRC6 sawtooth wave.|
|%%VRC7xx|VRC7 instrument x (01..15)|
|%%MMC5x|MMC5 pulse instrument with duty 'x' (0..3)|
|%%FME7|FME7 pulse instrument.|
|%%FDS|FDS instrument. The sample data will be used as the FDS waveform (should be 64 samples wide).|

## SERIOUS WARNING

Only use proper compatible instruments in the channels. ie. don't use a triangle instrument in the pulse channels!