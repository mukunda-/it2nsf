/*********************************************************************************************
 * IT2NSF
 * 
 * Copyright (C) 2009, mukunda, coda, madbrain, reduz
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

#include "toad.h"
#include "mml.h"
#include "nsf.h"
#include "emu2413.h"
#include <math.h>

#define pi 3.14159

#define master_volume 1.0

// levels
#define v2a03_mixlevel 1.0
#define square_level 0.1393
#define tri_level 0.2464
#define noise_level 0.1744
#define dpcm_level 0.5743

#define vrc6_mixlevel 1.0
#define vrc6p_level 0.25 // pulse
#define vrc6s_level 0.5 // saw

#define n106_level 0.25

#define fme7_level 0.25

#define fds_level 0.50

#define vrc7_level 5.0

extern bool QUIET; 

namespace Packify {

const char * const GEN_SAMP_NAME = "%%GEN%%";

// quality options
double c5freq = 261.625;
double defSamplingRate = 8372.0;
int sampleSize = 12000;
double NTSC = 60.0;
bool gen16 = true;
extern "C" {
int maxmulti = 10000;
}
// pulse, mmc
// C-3, C-4, C-5, C-6, C-7, C-8, C-9, C-10
const u8 samplingMap1[] = { 36, 48, 60, 72, 84, 96, 108, 120, 255 };
const double samplingRates1[] = {
	defSamplingRate, defSamplingRate, defSamplingRate,defSamplingRate*2,
	defSamplingRate*2, defSamplingRate*2, defSamplingRate*4, defSamplingRate*8 
};

// tri, vrc6, fme7
// C-2, C-3, C-4, C-5, C-6, C-7, C-8, C-9, C-10
const u8 samplingMap2[] = { 24, 36, 48, 60, 72, 84, 96, 108, 120, 255 };
const double samplingRates2[] = {
	defSamplingRate, defSamplingRate, defSamplingRate, defSamplingRate,defSamplingRate*2,
	defSamplingRate*2, defSamplingRate*2, defSamplingRate*4, defSamplingRate*8 
};

// n106
// C-0, C-1, C-2, C-3, C-4, C-5, C-6, C-7, C-8, C-9
const u8 samplingMap3[] = { 0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 255 };
const double samplingRates3[] = {
	defSamplingRate/2, defSamplingRate/2, defSamplingRate,defSamplingRate,
	defSamplingRate,defSamplingRate,defSamplingRate*2,defSamplingRate*2,
	defSamplingRate*4,defSamplingRate*4
};

// vrc7
const u8 samplingMap9[] = { 0, 12, 24, 36, 48, 60, 72, 84, 96, 255 };
const double samplingRates9[] = {
	49772, 49772, 49772,49772,
	49772,49772,49772,49772,
	49772,49772
};

// fds
// C-0, C-1, C-2, C-3, C-4, C-5, C-6, C-7, C-8
const u8 samplingMap4[] = { 0, 12, 24, 36, 48, 60, 72, 84, 96, 255 };
const double samplingRates4[] = {
	defSamplingRate/2, defSamplingRate/2, defSamplingRate,defSamplingRate,
	defSamplingRate,defSamplingRate,defSamplingRate*2,defSamplingRate*2,
	defSamplingRate*4
};

// simple pulse,mmc, tri,vrc6,fme7,fds,n106,vrc7,dpcm
// C-6
const u8 samplingMap5[] = {72, 255};
const double samplingRates5[] = {
	defSamplingRate*2
};

// dpcm
const u8 samplingMap6[] = {60, 255}; 
const double samplingRates6[] = {defSamplingRate};

// noise
// c-0, b-0, b-1, b-2, b-3, b-4, b-5, b-6, b-7, b-8, b-9
const u8 samplingMap7[] = { 0, 11, 23, 35, 47, 59, 71, 83, 95, 107, 119, 255 };
const double samplingRates7[] = {
	defSamplingRate,defSamplingRate,defSamplingRate,defSamplingRate,
	defSamplingRate,defSamplingRate,defSamplingRate*2,defSamplingRate*4,
	defSamplingRate*8,defSamplingRate*8,defSamplingRate*8
};

// noise
// c-0, b-0, b-1, b-2, b-3, b-4, b-5, b-6, b-7, b-8, b-9
const u8 samplingMap8[] = { 60, 255 };
const double samplingRates8[] = {
	defSamplingRate*8
};

//const u8 samplingMap2[] = { 48, 55, 60, 67, 72, 79, 84 }; // c4 g4 c5 g5 c6 g6 c7 (for DPCM)
//const u8 samplingMap3[] = { 47 }; // b3 (simple noise)

class Envelope {

	// current position
	int mark;
	int position;
	int defaultData;
	MML::Envelope source;

public:
	// current data
	int data;

	// set if reached loop region
	bool reachedLoop;
	bool reachedSusStart;
	bool reachedSusEnd;

	// initialize
	Envelope::Envelope();

	// reset position
	void reset();

	// set envelope source (and reset)
	void setSource( MML::Envelope & );

	// set default data for invalid envelopes
	void setDefaultData( int );

	// set mark for 
	void setMark();

	void read();

	// advance position
	// returns true if position == mark or
	// if envelope is disabled
	bool advance();

	bool isLoopSimple() { return source.isLoopSimple(); }
};

class Modulator {
	
private:
	int sweep;
	int depth;
	int position;
	int markedPosition;
	int delay;

	s16 idepth;
	s16 isweep;
	s16 irate;

	
public:

	void read();
	void advance();
	void setMark();
	bool testLoopPoint();

	// reset system
	void reset( MML::Instrument &program );

	// get frequency multiplier
	double pitchadd;

	// if reached full depth
	bool maxed;

	int getPhase() {
		return position;
	}
};

class Voice {
	
protected:

	int duty;
	double note;
	double samplingRate;

	double pos;
	double freq;
	double mod;
	double freqmod;
	bool freqdirty;
	bool dutydirty;
	int volume;
	
	double mark;

public:

	bool ended; // sound has ENDED.
	bool reachedLoop;
	bool hasLooped; // played entire sample

	Voice();

	virtual void reset();

	virtual void setSamplingRate( double );
	virtual void setNote( double );

	// set modulation (1.0 = +1 semitone)
	void modulate( double );

	// specific behavior
	void setDuty( int );

	// volume 0..16
	void setVolume( int );

	// change wavetable (used by n106,fds)
	void setWavetable( int* );

	// get 16-bit sample
	virtual s16 sample();

	virtual void setMark();
	virtual bool testLoopPoint();

	virtual void unload() {}

};

#define instrDef( name ) class name : public Voice { public: name():Voice() {} s16 sample(); }

class V_2A03Square : public Voice {
public:
	V_2A03Square():Voice() {} 
	s16 sample(); 
};

class V_2A03Triangle : public Voice {
public:
	V_2A03Triangle():Voice() {} 
	s16 sample(); 
};

class V_2A03Noise : public Voice {

	s16 prev;
	int shifter;
	int pindex;
	bool shortnoise;
public:
	void reset();
	void setshort() {
		shortnoise = true;
	}
	V_2A03Noise( bool shortn ):Voice() {
		shortnoise=shortn;
		reset();
	} 
	s16 sample(); 

	void setNote( double );

	void setMark();
	bool testLoopPoint();
};

class V_2A03DPCM : public Voice {

	int dac;
	int bitpos;
	int bytepos;
	NSF::DPCMSample *dpcm;
public:
	void reset();
	V_2A03DPCM( NSF::DPCMSample *d ):Voice() {
		dpcm = d;
		reset();
		
	} 
	void setNote( double );
	s16 sample(); 
	void setMark();
	bool testLoopPoint();
};

class V_VRC6Pulse : public Voice {
public:
	V_VRC6Pulse():Voice() {} 
	s16 sample(); 
};

class V_VRC6Saw : public Voice {
public:
	V_VRC6Saw():Voice() {} 
	s16 sample(); 
};

class V_N106 : public Voice {

	u8 *wavetable;
public:
	V_N106( u8 *wt ):Voice() {
		wavetable = wt;
	} 
	s16 sample(); 
};

class V_VRC7 : public Voice {

	OPLL* opll;
public:
	V_VRC7( u8 *FMparams );//:Voice();
	void setNote(double);
	void unload();
	s16 sample(); 
};

class V_FME7 : public Voice {
public:
	V_FME7():Voice() {} 
	s16 sample(); 
};

class V_FDS : public Voice {

	u8 *source;
public:
	V_FDS( u8 *samp ):Voice() {
		source = samp;
	} 
	s16 sample(); 
};

class Synth {

public:

	typedef enum {
		TYPE_PULSE,
		TYPE_TRI,
		TYPE_NOISE,
		TYPE_DPCM,
		TYPE_VRC6PULSE,
		TYPE_VRC6SAW,
		TYPE_N106,
		TYPE_VRC7,
		TYPE_VRC7C,
		TYPE_MMC5,
		TYPE_FME7,
		TYPE_FDS
	} InstrumentType;

private:
	InstrumentType type;

	// sampling
	double position;

	// modulation
	Modulator mod;

	// envelopes
	Envelope evol;
	Envelope epitch;
	Envelope eduty;
	
	// sources
	u8 nWaveTable[128];
	u8 fdsSource[64];
	NSF::DPCMSample dpcmSource;

	// current program
	MML::Instrument program;

	bool updateEnvelopes();
	//void updateEnvelope( int &result, int &pos, );

	bool envelopeLoopsAreSimple();

public:

	Synth();

	// program change
	void programChange( MML::Instrument &program );

	// create dpcm for sampling
	void setDPCMsource( ToadLoader::Sample &source );

	// create fds for sampling
	void setFDSsource( ToadLoader::Sample &source );

	// set n106 wavetable, table[128]
	void Synth::setN106table( u8 *table );

	// create a sample at the desired note
	ToadLoader::Sample * sample( int note, double samplingRate );
};

namespace {

int roundint( double v ) {
	return (int)floor(v+0.5);
}

void buildNotemap( ToadLoader::notemapEntry* dest, int base, const u8 *samplingMap, bool fixedpitch ) {

	if( !fixedpitch ) {
		for( int note = 0; note < 120; note++ ) {
			int bestindex = 0;
			int notediff = 200;
			for( int sindex = 0; samplingMap[sindex] != 255 && sindex < maxmulti; sindex++ ) {
				int snote = samplingMap[sindex];
				int diff = abs( snote - note );
				if( diff <= notediff ) { // try < for different bias
					bestindex = sindex;
					notediff = diff;
					if( diff == 0 )
						break;
				}
			}
			
			dest[note].note = note;
			dest[note].sample = base + bestindex;
		}
	} else {
		for( int note = 0; note < 120; note++ ) {
			dest[note].note = samplingMap[0];
			dest[note].sample = base;
		}
	}
}

}

void runSampler( ToadLoader::Module &mod, const u8 *sampleMap, const double *samplingRates, Synth &synth, ToadLoader::Instrument &ins, bool fixedpitch ) {
	int base = mod.samples.size() + 1;
	for( int i = 0; sampleMap[i] != 255 && i < maxmulti; i++ ) {
		mod.samples.push_back( synth.sample( sampleMap[i], samplingRates[i] ) );
	}
	buildNotemap( ins.notemap, base, sampleMap, fixedpitch );
}

const char *getface( int samplecount ) {
	if( samplecount < 50 )
		return "";
	else if( samplecount < 150 )
		return ":O";
	else
		return ":D";
}

void run( const char *input, const char *output ) {


	if( !QUIET )
		printf( "creating pack!\n" );
	
	ToadLoader::Module mod;
	mod.load( input );

	mod.purgeInstruments();
	mod.purgeMarkedSamples( GEN_SAMP_NAME ); // generated sample signature

	mod.instrumentMode = true;

	MML::Data mml( mod ); // get mml data

	Synth synth;

	synth.setN106table( mml.n106table );
	
	for( u32 i = 0; i < mml.instruments.size(); i++ ) {

		if( !QUIET )
			printf( "generating instrument... \"%s\"\n", mml.instruments[i]->name.c_str() );

		MML::Instrument &ins = *mml.instruments[i];
		synth.programChange( ins );

		ToadLoader::Instrument *newinstr = new ToadLoader::Instrument();
		newinstr->setName( ins.name.c_str() );
		newinstr->fadeout = mml.defaultFadeout;

		if( ins.type == Synth::TYPE_FDS ) {
			synth.setFDSsource( *mod.samples[ins.sample] );
		}

		if( ins.type == Synth::TYPE_DPCM ) {
			synth.setDPCMsource( *mod.samples[ins.sample] );
		}
		
		mod.instruments.push_back( newinstr );

		int sampleIndex0 = mod.samples.size();

		switch( ins.type ) {
			case Synth::TYPE_PULSE:
			case Synth::TYPE_MMC5:
				if( ins.isSimple() )
					runSampler( mod, samplingMap5, samplingRates5, synth, *newinstr, false );
				else
					runSampler( mod, samplingMap1, samplingRates1, synth, *newinstr, false );
				break;
			case Synth::TYPE_TRI:
			case Synth::TYPE_VRC6PULSE:
			case Synth::TYPE_VRC6SAW:
			case Synth::TYPE_FME7:
				if( ins.isSimple() )
					runSampler( mod, samplingMap5, samplingRates5, synth, *newinstr, false );
				else
					runSampler( mod, samplingMap2, samplingRates2, synth, *newinstr, false );
				break;
			case Synth::TYPE_FDS:
				if( ins.isSimple() )
					runSampler( mod, samplingMap5, samplingRates5, synth, *newinstr, false );
				else
					runSampler( mod, samplingMap4, samplingRates4, synth, *newinstr, false );
				break;
			case Synth::TYPE_N106:
				if( ins.isSimple() ) {
					runSampler( mod, samplingMap5, samplingRates5, synth, *newinstr, false );
				} else {
					runSampler( mod, samplingMap3, samplingRates3, synth, *newinstr, false );
				//	const u8 testmap[] = {60, 255};
				//	const double testRates[] = {49772};
				//	runSampler( mod, testmap, testRates, synth, *newinstr, false );
				}
				break;
			case Synth::TYPE_VRC7:
			case Synth::TYPE_VRC7C:
				if( ins.isSimple() ) {
					runSampler( mod, samplingMap5, samplingRates5, synth, *newinstr, false );
				} else {
					runSampler( mod, samplingMap9, samplingRates9, synth, *newinstr, false );
				}
				break;
			case Synth::TYPE_DPCM:
				runSampler( mod, samplingMap6, samplingRates6, synth, *newinstr, false );
				break;
			case Synth::TYPE_NOISE:
				if( ins.envDuty.valid ) {
					runSampler( mod, samplingMap8, samplingRates8, synth, *newinstr, true );
				} else {
					runSampler( mod, samplingMap7, samplingRates7, synth, *newinstr, false );
				}
		}

	}

	if( !QUIET ) {
		printf( "total instruments: %i\n", mod.instruments.size() );
		printf( "total samples... %i %s\n", mod.samples.size(), getface( mod.samples.size() ) );
		printf( "saving pack.\n" );
	}

	mod.save( output );

	if( !QUIET )
		printf( "operation complete!\n" );
}

//-------------------------------------------------------------------------------
// SYNTH COMING THROUGH
//-------------------------------------------------------------------------------

Synth::Synth() {
	evol.setDefaultData( 16 );
	epitch.setDefaultData( 128 );
	eduty.setDefaultData( 128 );
	//NSF::getDefaultWavetable( nWaveTable );
}

void Synth::programChange( MML::Instrument &p ) {
	program = p;
	type = (InstrumentType)p.type;
}

void Synth::setN106table( u8 *table ) {
	for( int i = 0; i < 128; i++ )
		nWaveTable[i] = table[i];
}

void Synth::setDPCMsource( ToadLoader::Sample &source ) {
	dpcmSource.createFromSample( source.data );
}

void Synth::setFDSsource( ToadLoader::Sample &source ) {
	NSF::getFDSsound( fdsSource, source );
}

ToadLoader::Sample *Synth::sample( int note, double samplingRate ) {
	
	// reset synth
	mod.reset( program );
	evol.setSource( program.envVol );
	epitch.setSource( program.envPitch );
	eduty.setSource( program.envDuty );
	position = 0;
	
	std::vector<s16> data;
	
	std::string insname;
	
	Voice *voice;
	switch( type ) {
		case TYPE_PULSE:
		case TYPE_MMC5:
			voice = new V_2A03Square();
			break;
		case TYPE_TRI:
			voice = new V_2A03Triangle();
			break;
		case TYPE_NOISE:
			voice = new V_2A03Noise( program.shortnoise );
			break;
		case TYPE_DPCM:
			voice = new V_2A03DPCM( &dpcmSource );
			break;
		case TYPE_VRC6PULSE:
			voice = new V_VRC6Pulse();
			break;
		case TYPE_VRC6SAW:
			voice = new V_VRC6Saw();
			break;
		case TYPE_N106:
			voice = new V_N106( nWaveTable );
			break;
		case TYPE_VRC7:
		case TYPE_VRC7C:
			voice = new V_VRC7( program.fm );
			break;
		case TYPE_FME7:
			voice = new V_FME7();
			break;
		case TYPE_FDS:
			voice = new V_FDS( fdsSource );
			break;
		default:
			voice = 0;
	}

	//------------------------------------------------------------
	// generate sound
	//------------------------------------------------------------

	voice->setSamplingRate( samplingRate );
	voice->setNote( (double)note );
	
	double frametime = samplingRate / NTSC;
	double timer = 0;
	bool allEnteredLoop = false;

	int loopStart;
	bool sampleHasLoop = true;

	bool tryExitOnSimple = false;
	bool finishedSampling = false;

	//double susStartPhase;

	for( ;; ) {
		evol.read();
		epitch.read();
		eduty.read();
		mod.read();

		voice->modulate( (((double)epitch.data-128.0) / 4.0) + mod.pitchadd + program.fdetune );
		voice->setDuty( eduty.data );
		voice->setVolume( evol.data );
		
		if( !allEnteredLoop ) {
			if(	evol.reachedLoop && 
				epitch.reachedLoop && 
				eduty.reachedLoop && 
				mod.maxed &&
				voice->reachedLoop ) {

				allEnteredLoop = true;
				evol.setMark();
				epitch.setMark();
				eduty.setMark();
				voice->setMark();
				mod.setMark();

				if( envelopeLoopsAreSimple() ) {
					tryExitOnSimple = true;
				}

				loopStart = data.size();
			}
		}

		/*if( !gotSusStart ) {
			if( evol.reachedSusStart ) {
				susStartPhase =voice->position
			}
		}*/
		
		
		// break on volume loop zero ending
		if( evol.reachedLoop && evol.isLoopSimple() && evol.data == 0 ) {
			sampleHasLoop = false;
			break;
		}

		// break on sample end
		if( voice->ended ) {
			sampleHasLoop = false;
			break;
		}
		
		// render a frame
		timer += frametime;
		while( timer >= 0.0 ) {
			timer -= 1.0;
			data.push_back( voice->sample() );
			
			// break on sample end
			if( voice->ended ) {
				sampleHasLoop = false;
				finishedSampling=true;
				break;
			}

			if( tryExitOnSimple ) {
				if( voice->testLoopPoint() && mod.testLoopPoint() ) {
					if( data.size() - loopStart > 16 ) {
						finishedSampling = true;
					}
					break;
				}
			}
		}

		bool vol_el = evol.advance();
		bool pitch_el = epitch.advance();
		bool duty_el = eduty.advance();
		bool elooped = vol_el && pitch_el && duty_el;
		mod.advance();

		if( allEnteredLoop && elooped ) {
			if( (int)data.size() >= sampleSize && (type != TYPE_DPCM) ) { // extra hack to prevent dpcm samples from pooping
				break;
			}
		}
		
		if( finishedSampling )
			break;
	}


	//------------------------------------------------------------
	if( voice ) {
		voice->unload();
		delete voice;
	}

	//------------------------------------------------------------
	// load data into sample
	//------------------------------------------------------------
	ToadLoader::Sample *samp = new ToadLoader::Sample();
	samp->setName( GEN_SAMP_NAME );
	samp->data.bits16 = gen16;
	samp->data.length = data.size();
	samp->data.loopStart = sampleHasLoop ? loopStart : 0;
	samp->data.loopEnd = sampleHasLoop ? samp->data.length : 0;
	samp->data.loop = sampleHasLoop;

	if(type != TYPE_VRC7 && type != TYPE_VRC7C)
	if( program.envVol.valid && program.envVol.susEnd != -1 ) {
		samp->data.sustain = true;
		samp->data.sustainStart = (int)(floor((double)program.envVol.susStart * frametime+0.5));
		int se = (int)(floor((double)program.envVol.susEnd * frametime+0.5));
		int tpe = 32*samplingRate/8372;
		se = (se - samp->data.sustainStart) / tpe;
		se *= tpe;
		se += samp->data.sustainStart;
		samp->data.sustainEnd = se;//( program.envVol.susEnd * samplingRate) / 60;
		samp->data.sustainStart++;
		samp->data.sustainEnd++;
	}

	if( gen16 ) {
		samp->data.data16 = new s16[data.size()];
		
		for( u32 i = 0; i < data.size(); i++ ) {
			samp->data.data16[i] = data[i];
		}
	} else {
		samp->data.data8 = new s8[data.size()];
		for( u32 i = 0; i < data.size(); i++ ) {
			int d = data[i] + 32768;
			samp->data.data8[i] = (s8)((d >> 8) - 128);
		}
	}
	samp->centerFreq = (int)(pow( 2.0, -(double)(note-60)/12.0 ) * samplingRate);
	// return new sample
	return samp;
}

bool Synth::envelopeLoopsAreSimple() {
	return evol.isLoopSimple() && epitch.isLoopSimple() && eduty.isLoopSimple();
}

Envelope::Envelope() {
	reset();
	defaultData = 0;
}

void Envelope::reset() {
	position = 0;
	data = 0;
	reachedLoop = false;
}

void Envelope::setSource( MML::Envelope &e ) {
	source = e;
	reset();
}

void Envelope::setDefaultData( int dd ) {
	defaultData = dd;
}

void Envelope::read() {
	if( source.valid ) {
		data = source.nodes[position];
		if( position >= source.loop ) {
			reachedLoop = true;
		}
		if( source.susEnd != 0 ) {
			if( position >= source.susStart ) {
//				reachedSusLoopStart = true;
			}
			if( position >= source.susEnd ) {
//				reachedSusLoopEnd = true;
			}
		}
	} else {
		data = defaultData;
		reachedLoop = true;
	}
}

bool Envelope::advance() {
	if( source.valid ) {
		position++;
		
		if( position >= (int)source.nodes.size() ) {
			position = source.loop;
			return position == mark;
		} else {
			return position == mark;
		}
	} else {
		return true;
	}
}

void Envelope::setMark() {
	mark = position;
}

void Modulator::reset( MML::Instrument &program ) {
	delay = program.mdelay;
	delay = delay == 0 ? 256 : delay;
	idepth = program.mdepth;
	isweep = program.msweep;
	irate = program.mrate;
	

	sweep = isweep;
	depth = 0;
	position = 0;
	pitchadd = 0.0;
	maxed = false;
}

void Modulator::read() {
	if( depth ) {
		pitchadd = sin( (double)position * pi * 2.0 / 64.0 ) * (double)depth / 16.0;
	} else {
		pitchadd = 0.0;
	}

	if( !idepth ) {
		maxed = true;
		return;
	}
}

void Modulator::advance() {

	if( delay ) {
		delay--;
	} else {

		if( depth < idepth ) {
			sweep--;
			if( sweep == 0 ) {
				sweep = isweep;
				depth++;
				if( depth == idepth )
					maxed = true;
			}
		}
		
		position += irate;
		position &= 63;

		
	}
}

void Modulator::setMark() {
	markedPosition = position;
}

bool Modulator::testLoopPoint() {
	return abs(position - markedPosition) < 3;
}

Voice::Voice() {
	reset();
}

void Voice::setSamplingRate( double rate ) {
	samplingRate = rate;
}

void Voice::setNote( double pnote ) {
	note = pnote;
	freq = c5freq * pow( 2.0, (double)(note-60.0)/12.0 ) / samplingRate;
	freqmod = freq;
	mod = 0;
}

void Voice::modulate( double f ) {
	mod = f;
	freqdirty = true;
	freqmod = freq * pow( 2.0, mod / 12.0 );
}

void Voice::setDuty( int d ) {
	duty = d;dutydirty = true;
}

void Voice::reset() {
	pos = 0.0;
	freq = 0.0;
	mod = 0.0;
	duty = 0;
	freqmod = 0.0;
	freqdirty = true;
	ended = false;
	hasLooped = false;
	reachedLoop = true;
}

void Voice::setVolume( int v ) {
	volume = v;
}

s16 Voice::sample() {
	return 0;
}

void Voice::setMark() {
	mark = pos;
}

bool Voice::testLoopPoint() {
	return fabs(pos-mark) < 0.001;
}

s16 V_2A03Square::sample() {
	double d;
	switch( duty ) {
		case 0:
			d = 0.125;
			break;
		case 1:
			d = 0.25;
			break;
		case 2:
			d = 0.5;
			break;
		case 3:
			d = 0.75;
			break;
	}
	double samp = pos < d ? -square_level : square_level;
	s16 s = roundint(samp * v2a03_mixlevel * 32767.0 * ((double)volume / 16.0) * master_volume);
	pos += freqmod;
	if( pos >= 1.0 ) hasLooped = true;
	pos -= floor(pos);
	return s;
}

s16 V_2A03Triangle::sample() {
	double samp;
	s16 s;
	if( volume == 0 )
		samp = 0.0;
	else
		samp = (fabs(pos-0.5)*2.0 - 0.5) * tri_level * 2.0;

	s = roundint(samp * v2a03_mixlevel * 32767.0 * master_volume);

	pos += freqmod;
	if( pos >= 1.0 ) hasLooped = true;
	pos -= floor(pos);
	return s;
}

void V_2A03Noise::reset() {
	shifter = 23367; //1;
	prev =0;
	hasLooped = true;
	mark = 0;
}

void V_2A03Noise::setNote( double n ) {
	note = n;
	freq = 7046.3484 * pow( 2.0, (double)(note-49) / 12.0 ) / samplingRate;
}

void V_2A03Noise::setMark() {
	mark =0;
}

bool V_2A03Noise::testLoopPoint() {
	return mark >= 12000;
}

s16 V_2A03Noise::sample() {

	//const int noisenotes[] = { 119, 107, 95, 83, 71, 64, 59, 55, 51, 47, 40, 35, 28, 23, 11, 0 };
	const int noiseperiods[] = { 4,8,16,32,64,96,128,160,202,254,380,508,762,1016,2034,4068 };

	if( duty != 128 ) {
		//freq = 7046.3484 * pow( 2.0, (double)(noisenotes[duty]-49) / 12.0 ) / samplingRate;
		freq = (1789772.5 / (double)noiseperiods[duty&15]) / samplingRate;
	}
	//s16 s;
	pos += freq;
	int samples = (int)floor(pos);
	pos -= samples;


	if( samples == 0 )
		return prev;
	else {
		if( mark < 12000 )
			mark += samples;
		double result = 0.0;
		for( int i = 0; i < samples; i++ ) {

			
			int bit0 = shifter & 1;
			bool outputhi = !!bit0;
			int otherbit = (((duty & 16) || shortnoise) ? (shifter >> 6) : (shifter >> 1)) & 1;
			shifter |= (bit0 ^ otherbit) << 15;
			shifter >>= 1;

			if( outputhi ) {
				result += volume / 16.0;
			}
			
		}
		result /= (double)samples;
		result -= volume / 16.0 / 2.0;
		result = roundint(result * noise_level * v2a03_mixlevel * 32767.0 * 2.0);
		prev = (s16)result;
		return (s16)result;
	}
	
	return 0;
}

void V_2A03DPCM::reset() {
	Voice::reset();

	dac = 32;
	bitpos = 0;
	bytepos = 0;
}

void V_2A03DPCM::setNote( double pnote ) {
	note = pnote;
	freq = c5freq * 32.0 * pow( 2.0, (double)(note-60.0)/12.0 ) / samplingRate;
	freqmod = freq;
	mod = 0;
}

void V_2A03DPCM::setMark() {
	mark = ((double)bytepos * 8.0 + (double)bitpos + pos);
}

bool V_2A03DPCM::testLoopPoint() {

	return fabs(((double)bytepos * 8.0 + (double)bitpos + pos)-mark) < 2.0;
}

s16 V_2A03DPCM::sample() {
	s16 s = roundint(((double)dac/63.0 - 0.5) * 32767.0 * dpcm_level * 2.0 * v2a03_mixlevel * master_volume);

	const int dpcmNotemap[] = { 48, 50, 52, 53, 55, 57, 59, 60, 62, 65, 67, 69, 72, 76, 79, 84 };

	if( duty == 128 ) {
		pos += freq;
	} else {
		pos += c5freq * 32.0 * pow( 2.0, (double)(dpcmNotemap[duty]-60) / 12.0 ) / samplingRate;
	}

	while( pos >= (1.0) ) {
		u8 data;
		if( bytepos < (int)dpcm->data.size() ) {
			data = dpcm->data[bytepos];
			if( (data >> bitpos) & 1 ) {
				dac++;
				if( dac >= 63 ) dac = 63;
			} else {
				dac--;
				if( dac < 0 ) dac = 0;
			}
			bitpos++;
			if( bitpos == 8 ) {
				bitpos = 0;
				bytepos++;
				if( bytepos == dpcm->data.size() ) {
					if( dpcm->loop ) {
						bytepos = 0;
					}
				}
				hasLooped = true;
			}
		} else {
			ended = true;
		}
		pos -= 1.0;
	}
	return s;
}

s16 V_VRC6Pulse::sample() {
	double d = (double)(duty+1) * 0.0625;
	double samp = pos < d ? -vrc6p_level : vrc6p_level;
	s16 s = roundint( samp * vrc6_mixlevel * 32767.0 * (double)volume/16.0 * master_volume );
	pos += freqmod;
	if( pos >= 1.0 ) hasLooped = true;
	pos -= floor(pos);
	return s;
}

s16 V_VRC6Saw::sample() {
	s16 s = roundint( (pos* (252.0/256.0)- 0.5) * vrc6s_level * 2.0 * 32767.0 * (double)volume / 16.0 * master_volume);
	pos = pos + freqmod;
	if( pos >= 1.0 ) hasLooped = true;
	pos = fmod( pos, 1.0 );
	return s;
}

s16 V_N106::sample() {
	s16 s = roundint( ((double)wavetable[duty + (int)floor(pos * 16)] / 15.0 - 0.5) * (double)volume / 16.0 * 32767 * n106_level * 2.0 * master_volume);
	pos = pos + freqmod;
	if( pos >= 1.0 ) hasLooped = true;
	pos = fmod( pos, 1.0 );
	return s;
}

s16 V_FME7::sample() {
	s16 s = roundint((pos < 0.5 ? -fme7_level : fme7_level) * 32767.0 * ((double)volume / 16.0) * master_volume);
	pos = pos + freqmod;
	if( pos >= 1.0 ) hasLooped = true;
	pos = fmod( pos, 1.0 );
	return s;
}

s16 V_FDS::sample() {
	s16 s = roundint( ((double)source[(int)floor(pos * 64)] / 64.0 - 0.5) * (double)volume / 16.0 * 32767.0 * fds_level * 2.0 * master_volume);
	pos = pos + freqmod;
	if( pos >= 1.0 ) hasLooped = true;
	pos = fmod( pos, 1.0 );
	return s;
}

V_VRC7::V_VRC7( u8 *FMparams ) : Voice() {
	Voice::reset();


	opll = OPLL_new( 3579545 );
	OPLL_reset( opll );

	// copy fm parms
	for( int i = 0; i < 8; i++ ) {
		OPLL_writeReg( opll, i, FMparams[i] );
	}

	//OPLL_writeReg( opll, 
	reachedLoop = false;
}

void V_VRC7::unload() {
	OPLL_delete( opll );
}

void V_VRC7::setNote( double n ) {
	note = n;
	freq = 2752.084952 * pow( 2.0, (double)(note-60.0)/12.0 );// / samplingRate;
	freqdirty=true;
}

s16 V_VRC7::sample() {

	if( dutydirty ) {
		dutydirty=false;
		if( duty & 128 ) {
			OPLL_writeReg( opll, 0x30, 0x0 );
		} else {
			OPLL_writeReg( opll, 0x30, (duty << 4) );
		}
	}
	
	if( freqdirty ) {
		freqdirty = false;
		
		int oplfreq = roundint(freqmod);//roundint(freqmod * 2752.08495 / c5freq * samplingRate);
		int octave = 0;
		while( oplfreq > 511 ) {
			oplfreq >>= 1;
			octave++;
		}
		OPLL_writeReg( opll, 0x10, oplfreq & 255 );
		OPLL_writeReg( opll, 0x20, (oplfreq >> 8) | (octave << 1) | 0x30 );
	}
	
	s16 s = roundint(((double)OPLL_calc(opll)*(double)volume / 16.0*vrc7_level));
	
	if( pos < 90000 ) {

		pos += 1.0;
		if( pos >= 5000 ) {
			reachedLoop = true;
		}
	}
	return s;
}

}
