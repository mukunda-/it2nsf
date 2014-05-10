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

#ifndef __TOADS_____
#define __TOADS_____

#include "inputdata.h"
#include "io.h"

//-----------------------------------------------------------------------------
namespace ToadLoader {
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
class Pattern {
//-----------------------------------------------------------------------------
public:
	Pattern();
	~Pattern();

	void load( IO::File & );
	void save( IO::File & );
	void deleteData();

	u16 dataLength;
	u16 rows;
	u8 *data;
};

//-----------------------------------------------------------------------------
class SampleData {
//-----------------------------------------------------------------------------
public:

	SampleData();
	~SampleData();

	void load( IO::File &, int convert );
	void save( IO::File & );
	bool compressed; //(for loading)

	bool bits16;
	bool stereo;
	
	int length;
	int loopStart;
	int loopEnd;
	int sustainStart;
	int sustainEnd;
	bool loop;
	bool sustain;
	bool bidiLoop;
	bool bidiSustain;

	union {
		s8* data8;
		s16* data16;
	};
};

//-----------------------------------------------------------------------------
class Sample {
//-----------------------------------------------------------------------------
	
public:
	Sample();

	void load( IO::File & );
	void save( IO::File & );
	bool testNameMarker( const char *str );
	void setName( const char * );
	u8 buildFlags();

	char name[27];
	char DOSFilename[13];
	
	int centerFreq;
	u8 globalVolume;
	u8 defaultVolume;
	u8 defaultPanning;

	u8 vibratoSpeed;
	u8 vibratoDepth;
	u8 vibratoForm;
	u8 vibratoRate;

	SampleData data;
};

//-----------------------------------------------------------------------------
typedef struct {
//-----------------------------------------------------------------------------
	u8	note;
	u8	sample;
} notemapEntry;

//-----------------------------------------------------------------------------
typedef struct {
//-----------------------------------------------------------------------------
	u8 y;
	u16 x;
} envelopeEntry;

//-----------------------------------------------------------------------------
class Envelope {
//-----------------------------------------------------------------------------

public:
	Envelope();

	void load( IO::File & );
	void save( IO::File & );

	bool enabled;
	bool loop;
	bool sustain;
	bool isFilter;
	u8 buildFlags();
	
	int length;
	
	int loopStart;
	int loopEnd;
	int sustainStart;
	int sustainEnd;
	
	envelopeEntry nodes[25];
};

//-----------------------------------------------------------------------------
class Instrument {
//-----------------------------------------------------------------------------
public:
	Instrument();

	void load( IO::File & );
	void save( IO::File & );
	void setName( const char * );

	char name[27];
	char DOSFilename[13];

	u8	newNoteAction;
	u8	duplicateCheckType;
	u8	duplicateCheckAction;

	u16	fadeout;
	u8	PPS;
	u8	PPC;
	u8	globalVolume;
	u8	defaultPan;
	u8	randomVolume;
	u8	randomPanning;
	u16	trackerVersion;
	u8	initialFilterCutoff;
	u8	initialFilterResonance;

	u8	midiChannel;
	u8	midiProgram;
	u16 midiBank;

	notemapEntry notemap[ 120 ];

	Envelope volumeEnvelope;
	Envelope panningEnvelope;
	Envelope pitchEnvelope;
};

//-----------------------------------------------------------------------------
class Module {
//-----------------------------------------------------------------------------
public:
	Module();
	~Module();

	void load( const char *filename );
	void save( const char *filename );
	void purgeInstruments();
	void purgeMarkedSamples( const char *marker );
	void trimSampleVector();
	void setTitle( const char * );

	std::string filename;
	
	char title[27];
	
	u16 patternHighlight;
	u16 length;
	u16 cwtv;
	u16 cmwt;
	
	bool flgStereo;
	bool instrumentMode;
	bool vol0MixOptimizations;
	bool linearSlides;
	bool oldEffects;
	bool gxxCompat;
	u8 buildFlags();

//	int special;(use message for bit0)
	u8 globalVolume;
	u8 mixingVolume;
	u8 initialSpeed;
	u8 initialTempo;
	u8 sep;
	u8 PWD;
	
	int messageLength;
	char *message;

	int channelPan[64];
	int channelVolume[64];

	int orders[256];

	std::vector<Instrument*> instruments;
	std::vector<Sample*> samples;
	std::vector<Pattern*> patterns;
};

//-----------------------------------------------------------------------------
class Bank {
//-----------------------------------------------------------------------------	
public:
	Bank( const std::vector<const char *> &files );
	~Bank();
	void addModule( const char *filename );

	std::vector<Module*> modules;
};

//-----------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------

#endif
