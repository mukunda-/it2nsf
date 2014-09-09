/*********************************************************************************************
 * mml2its
 * 
 * Copyright 2009, coda
 *
 * All rights reserved.                                      
 *  
 * Redistribution and use in source and binary forms, with or without modification, are 
 * permitted provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright notice, this list of 
 *      conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright notice, this list of 
 *      conditions and the following disclaimer in the documentation and/or other materials 
 *      provided with the distribution.
 *    * Neither the name of the owners nor the names of its contributors may be used to endorse
 *      or promote products derived from this software without specific prior written 
 *      permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS      
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *********************************************************************************************/

// duty = 0 | 0 1 2 3 2 1
// volume = 10 10 9 | 7
// pitch = 0 12 0 | 0 4 7

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <libgen.h>

#define skipws(s) do { while(isspace((s)[0])) (s)++; } while(0);

int pitch_position = 0;
int duty_position = 0;
int volume_position = 0;

// for loop detection
int pitch_position_2 = 0;
int duty_position_2 = 0;
int volume_position_2 = 0;

typedef struct {
	int length;
	int restart;
	int value[1024];
} envelope_t;

envelope_t pitch;
envelope_t duty;
envelope_t volume;

void str_replace(char *str, char from, char to) {
	char *found;
	while((found = strchr(str, from))) {
		*found = to;
		str = found + 1;
	}
}

void syntax_error(char *msg, int lineno) {
	fprintf(stderr, "Error: syntax error on line %d, %s\n", lineno, msg);
}

int read_envelope(envelope_t *env, char* str, int min, int max, int lineno) {
	int error = 0;
	char *endptr = str;
	env->restart = -1;
	skipws(str);
	if(str[0] != '=') {
		error = 1;
		syntax_error("expected '='", lineno);
	}
	str++;
	while(1) {
		long int value;
		skipws(str);
		if(str[0] == '|') {
			env->restart = env->length;
			str++;
		}
		if(str[0] == 0 )
			break;
		value = strtol(str, &endptr, 10);
		if(endptr == str) {
			error = 1;
			syntax_error("expected number", lineno);
			break;
		}
		str = endptr;
		if(value < min || value > max) {
			error = 1;
			fprintf(stderr, "Error: syntax error on line %d, number %ld is outside of range [%d , %d]\n", lineno, value, min, max);
			break;
		}
		
		if(env->length < 1024) {
			env->value[env->length] = value;
			env->length++;
		}
		else {
			error = 1;
			syntax_error("more than 1024 envelope points", lineno);
			break;			
		}
	}
	// we'll just fix this and pretend it's OK
	if(env->restart == env->length)
		env->restart--;
	// default restart position
	if(env->restart < 0)
		env->restart = env->length - 1;

	//fprintf(stderr, "Debugging: read %d envelope points, restart position is %d!\n", env->length, env->restart);
	return error;
}

int load_envelopes(char *filename) {
	int error = 0, lineno=0;
	char line[4096];
	FILE* file = fopen(filename, "rb");
	if(!file) { fprintf(stderr, "Error: couldn't open %s\n", filename); return 1; }
	while(1) {
		lineno++;
		if(!fgets(line, sizeof(line), file)) {
			break;
		}
		if(strlen(line)==0) {
			break;
		}
		if(strlen(line)>=4095) {
			syntax_error("line greater than 4096 characters long", lineno);
			break;
		}
		if(line[0]=='\n' || line[0]==';') // blank line or comment
			continue;
		// end line at ; or \n, whichever comes first...
		str_replace(line, ';', 0);
		str_replace(line, '\n', 0);
		if(strncmp(line, "duty", 4)==0)
			error |= read_envelope(&duty, line+4, 0, 3, lineno);
		else if(strncmp(line, "pitch", 5)==0)
			error |= read_envelope(&pitch, line+5, -128, 127, lineno);
		else if(strncmp(line, "volume", 6)==0)
			error |= read_envelope(&volume, line+6, 0, 15, lineno);
		else {
			error = 1;
			syntax_error("expected 'duty', 'pitch', or 'volume'", lineno);
			break;
		}
	}
	if(pitch.length == 0) {
		error = 1;
		fprintf(stderr, "Error: No pitch envelope found!\n");
	}
	if(duty.length == 0) {
		error = 1;
		fprintf(stderr, "Error: No duty envelope found!\n");
	}
	if(volume.length == 0) {
		error = 1;
		fprintf(stderr, "Error: No volume envelope found!\n");
	}		
	fclose(file);
	return error;
}


void write32le(FILE* fp, int data) {
	fputc(data & 0xFF, fp);
	fputc((data & 0xFF00) >> 8, fp);
	fputc((data & 0xFF0000) >> 16, fp);
	fputc((data & 0xFF000000) >> 24, fp);
}
void write16le(FILE* fp, int data) {
	fputc(data & 0xFF, fp);
	fputc((data & 0xFF00) >> 8, fp);
}

// from LAME
void WriteWaveHeader(FILE * const fp, const int pcmbytes,
                const int freq, const int channels, const int bits) {
    int     bytes = (bits + 7) / 8;
    /* quick and dirty, but documented */
    fwrite("RIFF", 1, 4, fp); /* label */
    write32le(fp, pcmbytes + 44 - 8 + 9 * 4 + 6 * 4 + 8); /* length in bytes without header */
    fwrite("WAVEfmt ", 2, 4, fp); /* 2 labels */
    write32le(fp, 2 + 2 + 4 + 4 + 2 + 2); /* length of PCM format declaration area */
    write16le(fp, 1); /* is PCM? */
    write16le(fp, channels); /* number of channels */
    write32le(fp, freq); /* sample frequency in [Hz] */
    write32le(fp, freq * channels * bytes); /* bytes per second */
    write16le(fp, channels * bytes); /* bytes per sample time */
    write16le(fp, bits); /* bits per sample */
    fwrite("data", 1, 4, fp); /* label */
    write32le(fp, pcmbytes); /* length in bytes of raw PCM data */
}

// not from LAME
void WriteSamplerData(FILE * fp, int loopstart, int loopend) {
	fwrite("smpl", 1, 4, fp);
	write32le(fp, 9 * 4 + 6 * 4); // remaining chunk size
	write32le(fp, 0); // manufacturer
	write32le(fp, 0); // product
	write32le(fp, 119574); // sample period in nanoseconds (1/8363 * 10^9)
	write32le(fp, 60); // middle C note number
	write32le(fp, 0); // middle C finetune
	write32le(fp, 0); // SMPTE format (unused)
	write32le(fp, 0); // SMPTE offset
	write32le(fp, 1); // number of sampleloops
	write32le(fp, 0); // additional data size
	// single sampleloop:
	write32le(fp, 0); // loop identifier
	write32le(fp, 0); // loop type (forward)
	write32le(fp, loopstart);
	write32le(fp, loopend);
	write32le(fp, 0); // fractional loop point?
	write32le(fp, 0); // loopcount (infinite);
}

double pulse_phase = 0;
int pulse(double phase, int duty) {
	phase = fmod(phase, 1.0);
	if(duty == 0 && phase < 0.125)
		return -1;
	if(duty == 1 && phase < 0.25)
		return -1;
	if(duty == 2 && phase < 0.5)
		return -1;
	if(duty == 3 && phase < 0.75)
		return -1;
	return 1;
}

int envelopes_looped = 1;

void advance_envelope(envelope_t *env, int *pos) {
	(*pos)++;
	if(*pos >= env->length) {
		*pos = env->restart;
		//n_envelopes_looped++;
	}
}

// actual:
//#define MIDDLE_C 261.625565
// what you get with a 32-sample loop at 8363hz:
#define MIDDLE_C 261.34375
void generate_tick(FILE* file) {
	int sample, i;
	fprintf(stderr, "tick! pitch %d vol %d duty %d\n", pitch.value[pitch_position], volume.value[volume_position], duty.value[duty_position]);
	for(i=0;i<139;i++) {
		sample = pulse(pulse_phase, duty.value[duty_position]) * volume.value[volume_position] * 32767 * 3/4 / 16;
		//fprintf(stderr, "sample: %d\n", sample);
		write16le(file, sample);
		pulse_phase += MIDDLE_C * pow(2, pitch.value[pitch_position]/12.0) / 8363;
	}
	envelopes_looped = 0;
	advance_envelope(&pitch, &pitch_position);
	advance_envelope(&duty,  &duty_position);
	advance_envelope(&volume,&volume_position);
	
	advance_envelope(&pitch, &pitch_position_2);
	advance_envelope(&duty,  &duty_position_2);
	advance_envelope(&volume,&volume_position_2);
	advance_envelope(&pitch, &pitch_position_2);
	advance_envelope(&duty,  &duty_position_2);
	advance_envelope(&volume,&volume_position_2);
	
	if(pitch_position_2 == pitch_position &&
	duty_position_2 == duty_position &&
	volume_position_2 == volume_position) {
		envelopes_looped = 1; 
	}
	
}

int loop_start = -1;
int loop_end = -1;
int extra_cycles = 16;
double loop_phase;

void generate(char *name) {
	int pcmbytes = 0;
	int i;
	char outname[23] = {0};
	strncpy(outname, name, 22);
	i = strlen(name);
	outname[i-4] = '.';
	outname[i-3] = 'i';
	outname[i-2] = 't';
	outname[i-1] = 's';
	FILE* file = fopen(outname, "wb");
	if(!file) return;
	fprintf(file, "IMPS");
	for(i=0;i<12;i++) {
		fputc(0, file); // dos filename (blank)
	}
	fputc(0, file); // reserved
	fputc(64, file); // global volume
	fputc(0x13, file); // sample present | 16-bit | use loop
	fputc(64, file); // default volume
	
	i = 22 - strlen(name);
	if(i < 0) { // this is bad, name too long
		fprintf(file, "MML:%22s", name);
	}
	else {
		fprintf(file, "MML:%s", name);
		for(;i;i--) fputc(0, file);
	}
	fputc(1, file); // signed
	fputc(32, file); // default pan off
	
	write32le(file, 0); // length 
	write32le(file, 0); // loop start
	write32le(file, 0); // loop end
	write32le(file, 8363); // c5 speed
	write32le(file, 0); // susloop start
	write32le(file, 0); // susloop end
	write32le(file, 0x50); // sample start (at end of header)
	write32le(file, 0); // sample vibrato params
	

	while(1) {
		generate_tick(file);
		pcmbytes += 139*2;		
		if(envelopes_looped == 1 && loop_start == -1) {
			fprintf(stderr, "loop start found!\n");
			loop_start = pcmbytes/2;
			loop_phase = fmod(pulse_phase,1.0);
		}
		else if(envelopes_looped == 1 && (1 || (fabs(fmod(pulse_phase,1.0) - loop_phase)<0.001))  ) {
			fprintf(stderr, "loop end found! %d cycles remaining\n", extra_cycles-1);
			loop_end = pcmbytes/2;
			if(--extra_cycles == 0)
				break;
		}
	}
	
	fseek(file, 0x30, SEEK_SET);
	write32le(file, pcmbytes/2); // length 
	write32le(file, loop_start); // loop start
	write32le(file, loop_end); // loop end
	
	fclose(file);
}

int mml2wav(char *filename) {
	if(load_envelopes(filename))
		return 1;
	generate(basename(filename));
	return 0;
}
int main(int argc, char *argv[]) {
	return mml2wav(argv[1]);
}
