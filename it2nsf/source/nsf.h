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

#ifndef IT2SPC_H
#define IT2SPC_H

#include "toad.h"
#include "mml.h"

namespace NSF {

class Bank;
class Module;

//--------------------------------------------------------------
typedef struct {
//--------------------------------------------------------------
	u16 address;
	u8 bank;
} dataPointer;

//--------------------------------------------------------------
class DPCMSample {
//--------------------------------------------------------------
private:
	
	
public:
	DPCMSample();
	DPCMSample( const ToadLoader::SampleData & );

	void createFromSample( const ToadLoader::SampleData & );
	void reset();

	std::vector<u8> data;
	int		dataLength;
	bool	loop;
	dataPointer pointer;

	u8 dpcmLen; // for export

	bool	operator ==( const DPCMSample& ) const;
	void	exportData( IO::File & );
	
	int getDMClength() const {
		return data.size() / 16;
	}

	int getDataLength() const {
		return data.size();
	}

	int getExportSize() const {
		return data.size();
	}
};

//---------------------------------------------
class Envelope {
//---------------------------------------------

public:
	std::vector<u8> nodes;

	void exportData( IO::File &, int convert );
};

//---------------------------------------------
class Instrument {
//---------------------------------------------
private:
	u8		type;
	u8		defaultVolume;
	u16		pitchBase;
	u8		modDelay;
	u8		modSweep;
	u8		modDepth;
	u8		modRate;
	u8		envelopemask;
	
	u8		fdsSample[64]; // (fds only)
	
	int		dpcmIndex; // (dpcm only)
	u8		vrc7custom[8]; // (vrc7 only)

	bool	shortnoise; // (noise only)
	
	Envelope envVolume;
	Envelope envPitch;
	Envelope envDuty;

	dataPointer pointer;

	Module *parent;

	bool convertEnvelope( Envelope &dest, const MML::Envelope &src );
	
public:
	Instrument( Module *fparent, const ToadLoader::Module &, const MML::Instrument & );

	int getExportSize();

	const dataPointer *getPointer() {
		return &pointer;
	}
	
	void exportData( IO::File & );
};

//---------------------------------------------
class Pattern {
//---------------------------------------------
private:
	u8		rows;
	u8		numChannels;
	std::vector<u8> data;

	dataPointer pointer;

public:
	Pattern();
	Pattern( ToadLoader::Pattern &, int chcount );

	const dataPointer *getPointer() {
		return &pointer;
	}
	
	void exportData( IO::File &file );

	u32 getCRC();

	int getExportSize() {
		return data.size() + 1;
	}
};

//---------------------------------------------
class Module {
//---------------------------------------------
	u8	initialVolume;
	u8	initialTempo;
	u8	initialSpeed;
	
	u8	sequence[256];
	u8	length;

	u8	n106wavetable[128];

	u8	expansionChips;

	u8	numChannels;
	u8	channelMap[16];

	dataPointer pointer;

	void Module::parseSMOption( const char * );
	void Module::parseSMOptions( const ToadLoader::Module & );

	Bank *parent;
	
public:
	Module( 
		Bank *fParent,
		const ToadLoader::Module & );

	~Module();
	
	void exportData( IO::File &file );
	void exportAddressTable( IO::File & ) const;

	int expChipsUsed() {
		return expansionChips;
	}

	int getExportSize();

	std::vector<Pattern*> patterns;
	std::vector<Instrument*> instruments;

	Bank *getParent() {
		return parent;
	}
	
	const dataPointer *getPointer() {
		return &pointer;
	}
};

//---------------------------------------------
class Bank {
//---------------------------------------------
	std::vector<Module*> modules;
	std::vector<DPCMSample*> dpcm_samples;
	
	const char *getAnimal() const;
	
public:
	Bank();
	Bank( const ToadLoader::Bank &, const ConversionInput::OperationData & );
	~Bank();

	void addModule( const ToadLoader::Module & );
	void exportNSF( const char * ) const;

	int getDPCMbank( int );
	int getDPCMoffset( int );
	int getDPCMlength( int );
	int getDPCMloop( int );

	
	int Bank::addDPCM( const ToadLoader::Sample &samp );

	const char *strTitle;
	const char *strArtist;
	const char *strCopyright;
};

void getFDSsound( u8 *fdsSample, ToadLoader::Sample &source );
void getDefaultWavetable( u8 *dest );

}

#endif // IT2SPC_H
