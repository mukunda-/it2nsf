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

#include <string>
#include <math.h>
#include "mml.h"

namespace MML {

const double CFREQ = 261.625565;
const double PTAB_BASE = 4095.875;
const double FTAB_BASE = 16383.5;

// periods for c-5
const double C_PULSE = 426.560591;
const double C_VRC6 = 426.560591;
const double C_MMC5 = 426.560591;

const double C_FME7 = 213.2802955;

const double C_TRI = 213.749046;
const double C_NOISE = 213.749046;
const double C_DPCM = 213.749046;

const double C_VRC6S = 487.640676;

// frequencies for c-5
const double C_N106 = 18393.463199;
const double C_VRC7 = 2752.084952;
const double C_FDS = 613.115440;

typedef const char* strptr;

const u8 n106default[] = { // 64 samples
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 15, 15, 15, 15, 15, 15, 15, 15, // square
	7, 6, 5, 4, 3, 2, 1, 0,  // tri (borrows from next)
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, // saw
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

namespace {

enum {
	NAN = -99999997
};

const char * delimiters = " \t";
const char * commentchar = "#;\\";
const char * startmarker = "[[IT2NSF]]";

enum {
	CMD_MAP,
	CMD_N106,
	CMD_INSTRUMENT,
	CMD_TYPE,
	CMD_VOLUME,
	CMD_PITCH,
	CMD_DUTY,
	CMD_MDELAY,
	CMD_MSWEEP,
	CMD_MDEPTH,
	CMD_MRATE,
	CMD_PRESET,
	CMD_SAMPLE,
	CMD_FM,
	CMD_DEFVOL,
	CMD_DETUNE,
	CMD_SWIDTH,
	CMD_SHORTNOISE,
	CMD_INSFADE,
	CMD_UNKNOWN
};

const char *commands[] = {
	"map",
	"n106",
	"instr",
	"type",
	"vol",
	"pitch",
	"duty",
	"mdel",
	"msw",
	"mdep",
	"mrate",
	"preset",
	"sample",
	"fm",
	"defvol",
	"detune",
	"swidth",
	"shortnoise",
	"insfade"
};

const char *channelList[] = {
	"squ", "sq2", "tri", "nse",
	"dmc", "v61", "v62", "v6s",
	"n61", "n62", "n63", "n64",
	"n65", "n66", "n67", "n68",
	"v71", "v72", "v73", "v74",
	"v75", "v76", "mmc", "mm2",
	"f71", "f72", "f73", "fds", 0
};

const short defaultWidths[] = {
	32, 32, 32, 32,
	32, 32, 32, 32,
	32, 32, 32, 32,
	32, 32, 32, 32,
	32, 32, 32, 32,
	32, 32, 32, 32,
	32, 32, 32, 64
};

const char *instrumentTypes[] = {
	"pulse", "tri", "noise", "dpcm",
	"vrc6pulse", "vrc6saw", "n106",
	"vrc7", "vrc7c", "mmc5", "fme7",
	"fds", 0
};

bool isComment( char c ) {
	for( int i = 0; commentchar[i]; i++ ) {
		if( c == commentchar[i] )
			return true;
	}
	return false;
}

bool isEndl( char c ) {
	return (c == '\r') || (c == '\n') || isComment(c) || (c == 0);
}

bool isDelim( char c ) {
	for( int i = 0; delimiters[i]; i++ ) {
		if( c == delimiters[i] ) {
			return true;
		}
	}
	return false;
}

void findTerm( strptr &str ) {
	if( !(*str) ) return;
	bool delim;
	do {
		delim = false;
		for( int j = 0; delimiters[j]; j++ ) {
			if( (*str) == delimiters[j] ) {
				delim = true;
				str++;
				if( !(*str) ) return;
			}
		}
	} while( delim );
}

void nextLine( strptr &str ) {
	if( !(*str) ) return;
	while( (*str) ) {
		if( (*str) != '\r' && (*str) != '\n' ) {
			str++;
		} else {
			break;
		}
	}
	if( *str == '\r' ) str++;
	if( *str == '\n' ) str++;
//	while( (*str) == '\r' || (*str) == '\n' ) {
//		str++;
//	}
}

void findStart( strptr &str ) {
	if( !(*str) ) return;

	int matches=0;
	while( *str ) {
		if( (*str++) == startmarker[matches] ) {
			matches++;
			if( startmarker[matches] == 0 ) {
				return;
			}
		} else {
			matches = 0;
		}
	}
}                 

void getTerm( std::string &str, strptr &text ) {
	str.erase();
	while( *text ) {
		if( isDelim(*text) ) {
			break;
		}
		if( isComment(*text) ) {
			break;
		}
		if( isEndl( *text ) ) {
			break;
		}
		str += *text++;
	}
}

void lcaseString( std::string &str ) {
	for( u32 i = 0; i < str.size(); i++ ) {
		if( str[i] >= 'A' && str[i] <= 'Z' ) {
			str[i] += 'a' - 'A';
		}
	}
}

int findCommand( std::string &str ) {
	lcaseString( str );
	int cmd;
	for( cmd = 0; cmd < CMD_UNKNOWN; cmd++ ) {
		bool found = true;
		for( u32 i = 0; i < str.size(); i++ ) {
			if( (commands[cmd][i] != str[i]) || (commands[cmd][i] == 0) ) {
				
				found = false ;
				break;
			}
		}
		if( found )
			break;
	}
	return cmd;
}

void getArgs( std::vector<std::string> &args, strptr &text ) {

	std::string term;

	args.clear();
	findTerm( text );

	while( !isEndl( *text ) ) {

		getTerm( term, text );
		if( !term.empty() )
			args.push_back( term );
		findTerm( text );
	};
}

void getArgString( std::string &result, strptr &text ) {
	result.clear();
	findTerm( text );
	int length = 0;
	while( !isEndl( text[length] ) ) {
		length++;
	}
	while( isDelim( text[length] ) ) {
		length--;
	}
	while( length ) {
		result += *text++;
		length--;
	}
}

int getChannelIndex( std::string &s ) {
	lcaseString( s );
	for( int i = 0; channelList[i]; i++ ) {
		if( s == channelList[i] ) {
			return i;
		}
	}
	return -1;
}

int getInstrumentType( std::string &s ) {
	lcaseString( s );
	for( int i = 0; instrumentTypes[i]; i++ ) {
		if( s == instrumentTypes[i] ) {
			return i;
		}
	}
	return -1;
}

int hex2num( char c ) {
	if( c >= '0' && c <= '9' ) {
		return c - '0';
	} else if( c >= 'a' && c <= 'f' ) {
		return c - 'a' + 10;
	} else if( c >= 'A' && c <= 'F' ) {
		return c = 'A' + 10;
	}
	return -1;
}

int dec2num( char c ) {
	if( c >= '0' && c <= '9' ) {
		return c - '0';
	}
	return -1;
}

int toFpNumber( std::string s, int frac ) {
	int result = 0;
	int decplaces = 1;
	if( s.empty() )
		return NAN;

	bool hex;

	if( s[s.size()-1] == 'h' ) {
		hex = true;
		s = s.substr( 0, s.size() - 1 );
	} else {
		hex = false;
	}

	if( s.empty() )
		return NAN;

	bool negative = false;
	if( s[0] == '-' ) {
		negative = true;
		s = s.substr( 1 );
		if( s.empty() )
			return NAN;
	}

	if( hex ) {
		while( !s.empty() ) {
			result *= 16;
			
			int a = hex2num( s[0] );
			if( a == -1 )
				return NAN;
			result += a;
			s = s.substr( 1 );
		}
	} else {
		bool dec = false;
		while( !s.empty() ) {

			if( s[0] == '.' ) {
				dec = true;
				s = s.substr(1);
				continue;
			}

			result *= 10;
			int a = dec2num( s[0] );
			if( a == -1 )
				return NAN;

			result += a;
			if( dec )
				decplaces *= 10;

			s = s.substr( 1 );
		}
	}
	int final = ((result << frac) + (decplaces>>1)) / decplaces;
	return negative ? -final : final;
}

int toNumber( std::string s ) {
	return toFpNumber( s, 0 );
}

void parseEnvelope( Envelope &env, strptr &text, int &ln, int min, int max, int frac ) {
	env.loop = -1;
	env.nodes.clear();
	env.susStart = -1;
	env.susEnd = -1;
	env.valid = true;
	findTerm( text );
	if( isEndl( *text ) ) {
		env.setSingle( 0 );
		return;
	}

pood:
	while( !isEndl( *text ) && (*text) ) {
		std::string s;
		getTerm( s, text );

		if( s == "{" ) {
			env.susStart = env.nodes.size();
		} else if( s == "}" ) {
			env.susEnd = env.nodes.size();
		} else if( s == "|" ) {
			env.loop = env.nodes.size();
		} else {
		
			int node = toFpNumber( s, frac );
			if( node < min || node > max ) {
				printf( "envelope error (line %i): node entry \"%s\" is out of range (range=%i..%i).\n", ln, s.c_str(), min >> frac, max >> frac );
				node = 0;
			}
			
			env.nodes.push_back( (u8)node );
		}

		findTerm( text );
	}

	if( (*text) == '\\' ) {
		ln++;
		nextLine( text );
		goto pood;
	}
	if( env.loop == -1 ) {
		env.loop = env.nodes.size() - 1;
	}
}

}

Data::Data( const ToadLoader::Module &source ) {
	
	printf( "parsing mml...\n" );

	defaultFadeout = 0;
	
	for( int i = 0; i < 64; i++ )
		n106table[i] = n106default[i];
	for( int i = 0; i < 64; i++ )
		n106table[i+64] = 0;
	
	if( source.messageLength == 0 || (!source.message) )
		return;
	
	const char *text = source.message;

	// search for special marker
	findStart( text );
	
	if( !(*text) ) {
		printf( "error: no mml data!\n" );
		
		return; // no data
	}
	

	// get mml data
	nextLine( text );

	int lineNumber = 1;

	int cmd;
	std::string cmdStr;

	std::vector<std::string> args;

	expansionChips = 0;
	
	for(;;) {

		findTerm(text);
		
		if( isEndl( *text ) ) {
	
			if( !(*text) ) {
				return;
			}
			
			nextLine( text );
			lineNumber++;
			continue;
		}
		if( !(*text) ) {
			return;
		}
	
		getTerm( cmdStr, text );
		


		cmd = findCommand( cmdStr );

		
		if( cmd == CMD_UNKNOWN ) {
			printf( "unknown command \"%s\"\n", cmdStr.c_str() );
			nextLine( text );
			lineNumber++;
			continue;
		}
	
#define instrCheck( reason ) if( instruments.empty() ) { printf( "error (line %i): " reason " (no current instrument!)\n", lineNumber ); break; } Instrument *ins = instruments.at( instruments.size()-1 );
#define getXargs( number ) getArgs( args, text ); if( args.size() != (number) ) { printf( "error (line %i): wrong number of arguments (expected %i).\n", lineNumber, number ); break; }
#define printerr( text ) printf( "error (line %i): " text "\n", lineNumber ); break;
#define checkfirstnum( min, max ) int num = toNumber( args[0] ); if( num == NAN || num < (min) || num > (max) ) { printerr( "arg is out of range or not valid.\n" ); }


		switch( cmd ) {
		case CMD_MAP:
			
			getArgs( args, text );
			channelCount = 0;
			for( u32 i = 0; i < args.size(); i++ ) {
				if( i == 16 ) {
					printerr( "too many MAP args. 16 max!" );
				}
				int ci = getChannelIndex( args[i] );
				channelCount++;
				if( ci == -1 ) {
					printf( "unknown map entry: \"%s\"\n", args[i].c_str() );
				} else {
					channelMap[i] = ci;

					if( ci >= 5 && ci <= 7 ) // vrcvi
						expansionChips |= 1;
					else if( ci >= 8 && ci <= 15 ) // n106
						expansionChips |= 16;
					else if( ci >= 16 && ci <= 21 ) // vrcvii
						expansionChips |= 2;
					else if( ci >= 22 && ci <= 23 ) // mmc5
						expansionChips |= 8;
					else if( ci >= 24 && ci <= 26 ) // fme-07
						expansionChips |= 32;
					else if( ci == 27 ) // fds
						expansionChips |= 4;
					
				}
			}
			break;
		case CMD_N106:
			{
				getXargs( 2 );
				int start = toNumber( args[0] );
				int sample = toNumber( args[1] );
				if( start == NAN || sample == NAN || sample < 1 || start > 127 ) {
					printerr( "invalid argument(s)." );
				}
				sample--;

				ToadLoader::SampleData &samp = source.samples[sample]->data;
				if( samp.bits16 ) {
					for( int i = 0; i < samp.length; i++ ) {
						int a = samp.data16[i]>>12;
						if( a > 7 ) a = 7;
						if( a < -8 ) a = -8;
						n106table[start+i] = a+8;
						if( (start+i) == 127 ) break;
					}
				} else {
					for( int i = 0; i < samp.length; i++ ) {
						int a = samp.data8[i]>>4;
						if( a > 7 ) a = 7;
						if( a < -8 ) a = -8;
						n106table[start+i] = a+8;
						if( (start+i) == 127 ) break;
					}
				}
			}
			break;
		case CMD_INSTRUMENT:
			{
				
				Instrument *ins = new Instrument();
				std::string s;
				getArgString( s, text );
				ins->name = s;
				instruments.push_back( ins );

			}
			break;
		case CMD_TYPE:
			{
				instrCheck( "" );
				getXargs( 1 );
				int typ = getInstrumentType( args[0] );
				if( typ == -1 ) {
					printf( "unknown instrument type: %s\n", args[0].c_str() );
					break;
				}
				ins->setType( typ );
				if( typ == 8 ) {
					ins->envDuty.setSingle( 0 );
				}
			}
			break;
		case CMD_VOLUME:
			{
				
				instrCheck( "cannot add volume envelope" );
				parseEnvelope( ins->envVol, text, lineNumber, 0, 16, 0 );
				
			}
			break;
		case CMD_PITCH:
			{
				instrCheck( "cannot add pitch envelope" );
				parseEnvelope( ins->envPitch, text, lineNumber, -128*4, 125*4, 2 );
				ins->envPitch.unsignData();
			}
			break;
		case CMD_DUTY:
			{
				instrCheck( "cannot add duty envelope" );
				int rmin=-1, rmax=-1;
				switch( ins->type ) {
				case 0: // pulse
				case 9: // mmc
					rmin = 0; rmax=3;
					break;
				case 2: // noise
					rmin = 0; rmax = 31;
					break;
				case 3: // dpcm
					rmin = 0; rmax = 15;
					break;
				case 4: // vrc6 pulse
					rmin = 0; rmax = 7;
					break;
				case 6: // n106
					rmin = 0; rmax = 63;
					break;
				case 7: // vrc7
					rmin = 1; rmax = 15;
					break;
				}
				if( rmin == -1 ) {
					printerr( "instrument type doesnt support duty envelope.\n" );
				}
				parseEnvelope( ins->envDuty, text, lineNumber, rmin, rmax, 0 );
			}
			break;
		case CMD_MDELAY:
			{
				instrCheck( "cannot set modulation delay" );
				getXargs( 1 );
				checkfirstnum( 1, 256 )
				ins->mdelay = num;
			}
			break;
		case CMD_MSWEEP:
			{
				instrCheck( "cannot set modulation sweep" );
				getXargs( 1);
				checkfirstnum( 1, 255 );
				ins->msweep = num;
			}
			break;
		case CMD_MDEPTH:
			{
				instrCheck( "cannot set modulation depth" );
				getXargs( 1 );
				checkfirstnum( 0, 15 );
				ins->mdepth = num;
			}
			break;
		case CMD_MRATE:
			{
				instrCheck( "cannot set modulation rate" );
				getXargs( 1 );
				checkfirstnum( 0, 64 );
				ins->mrate = num;
			}
			break;
		case CMD_SAMPLE:
			{
				instrCheck( "" );
				getXargs( 1 );
				checkfirstnum( 1,  (int)source.samples.size() );
				ins->sample = num-1;
			}
			break;
		case CMD_FM:
			{
				instrCheck( "" );
				getXargs( 8 );
				for( u32 i = 0; i < 8 && i < args.size(); i++ ) {
					int num = toNumber( args[i] );
					if( num == NAN || num < 0 || num > 255 ) {
						printf( "error (line %i): arg #%i is out of range or not valid.\n", lineNumber, i+1 );
						num = 0;
					}
					ins->fm[i] = num;
				}
			}
			break;
		case CMD_PRESET:
			{
				instrCheck( "" );
				getXargs( 1) ;
				checkfirstnum( 0, 255 );
				ins->envDuty.setSingle( num );
			}
			break;
		case CMD_DEFVOL:
			{
				instrCheck( "" );
				getXargs( 1 );
				checkfirstnum( 0, 64 );
				ins->defaultVolume = num;
			}
			break;
		case CMD_DETUNE:
			{
				instrCheck( "" );
				getXargs( 1 );
				
				int num = toFpNumber( args[0], 12 );
				if( num == NAN ) {
					printerr( "arg is not valid" );
				}
				ins->detune( num );
			}
			break;
		case CMD_SWIDTH:
			{
				instrCheck( "" );
				getXargs( 1 );
				checkfirstnum( 8, 256 );
				ins->setSwidth( num );
				
			}
			break;
		case CMD_SHORTNOISE:
			{
				instrCheck( "" );
				ins->shortnoise = true;
			}
			break;
		case CMD_INSFADE:
			getXargs( 1 );
			checkfirstnum( 0, 99999 );
			defaultFadeout = num;
		}

		nextLine( text );
		lineNumber++;
	}

	if( !(*text) )
		return;
}

void Instrument::setSwidth( int width ) {
	swidth = width;
	cfreq = CFREQ * (double)width;
}

void Instrument::setType( int typ ) {
	if( typ < 0 || typ > 27 ) typ = 0;
	type = typ;
	setSwidth( defaultWidths[typ] );
}

void Instrument::detune( int amount ) {
	double a = amount;
	fdetune = a / 4096.0;
//	cfreq = cfreq * pow(2.0, (a / 12.0) );
}

int Instrument::getPitchBase() const {
	// c5 note = 60
	
#define pitchbase_p(cperiod) -(int)floor( log( ((cperiod) * pow(2.0, -fdetune/12.0)) / (PTAB_BASE / 32.0) ) / log(2.0) * 768.0 + 0.5 )
#define pitchbase_f(cfreq,mult) (int)floor( log( ((cfreq) * pow(2.0, fdetune/12.0)) / (FTAB_BASE / 1024.0*(mult)) ) / log(2.0) * 768.0 + 0.5 )
	
	switch( type ) {
	case 0: // 2a03 pulse, vrc6, mmc5
	case 4:
	case 9:
		return pitchbase_p(C_PULSE);
	case 10: // fme7
		return pitchbase_p(C_FME7);
	case 1: // tri
		return pitchbase_p(C_TRI);
	case 2: // noise, dpcm
	case 3:
		return 0;
	case 5: // vrc6 sawtooth
		return pitchbase_p(C_VRC6S);
	case 11: // fds sound
		return pitchbase_f(C_FDS, 1.0);
	case 7: //vrc7
	case 8:
		return pitchbase_f(C_VRC7,1.0);
	case 6: // n106
		return pitchbase_f(C_N106,8.0);
	}
	return 0;
}

bool Instrument::isSimple() {
	// vrc7 certainly ISNT simple.
	if( type == 7 || type == 8 ) return false; 

	if( envVol.isSimple() && envPitch.isSimple() && envDuty.isSimple() && (mdepth == 0 || mrate == 0) )
		return true;

	return false;

}

Instrument::Instrument() {
	name.clear();
	type = 0;
	mdelay = 1;
	msweep = 1;
	mdepth = 0;
	mrate = 0;
	defaultVolume = 64;
	for( int i = 0; i < 8; i++ )
		fm[i] = 0;
	sample = 0;
	envVol.valid = false;
	envPitch.valid = false;
	envDuty.valid = false;
	swidth = 32;
	cfreq = 8363;
	fdetune = 0;
	shortnoise=false;
}

void Envelope::unsignData() {
	for( u32 i = 0; i < nodes.size(); i++ ) {
		nodes[i] ^= 0x80;
	}
}

bool Envelope::isSimple() {
	if( !valid )
		return true;
	else {
		int v = nodes[0];
		for( int i = 0; i < (int)nodes.size(); i++ ) {
			if( nodes[i] != v )
				return false;
		}
		return true;
	}
}

bool Envelope::isLoopSimple() {
	if( !valid )
		return true;
	else {
		int v = nodes[loop];
		for( int i = loop; i < (int)nodes.size(); i++ ) {
			if( nodes[i] != v )
				return false;
		}
		return true;
	}
}

}
