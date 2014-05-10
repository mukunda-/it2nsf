/*********************************************************************************************
 * IT2NSF
 * 
 * Copyright (C) 2009, Mukunda Johnson, Ken Snyder, Hubert Lamontagne, Juan Linietsky
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

#include "inputdata.h"

extern std::string PATH;
extern "C" {
extern int maxmulti;
}
namespace ConversionInput {


	template<typename T> static void deletePtrVector( std::vector<T*> &vecs ) {
		
		for(typename std::vector<T*>::iterator iter = vecs.begin(), ending = vecs.end();
			iter != ending; 
			iter++ ) {
		
			if( *iter )
				delete (*iter);
		}
		
		vecs.clear();
	}

	enum {
		ARG_OPTION,
		ARG_INPUT,
		ARG_INVALID
	};
	
	static int get_arg_type( const char *str ) {
		if( str[0] ) {
			if( str[0] == '-' ) {
				return ARG_OPTION;
			} else {
				return ARG_INPUT;
			}
		} else {
			return ARG_INVALID;
		}
	}

	static bool strmatch( const char *source, const char *test ) {
		for( int i = 0; test[i]; i++ )
			if( source[i] != test[i] )
				return false;
		return true;
	}
	
	OperationData::OperationData( int argc,  char *argv[] ) {

		help = false;
		quiet = false;
		nsfindex = 0;
		packmode = false;
		reverse = false;

		strTitle = strCopyright = strArtist = 0;
		
		// search for params
		for( int arg = 1; arg < argc; ) {
			int ptype = get_arg_type( argv[arg] );
			switch( ptype ) {
				case ARG_INVALID:
					// nothing
					arg++;
					break;
				case ARG_OPTION:
					// long command

#define TESTARG2(str1,str2) (strmatch( argv[arg], str1 ) || strmatch( argv[arg], str2 ))
					
					if( TESTARG2( "--output", "-o" ) ) {

						arg++;
						if( arg == argc ) {
							printf( "some kind of error.\n" );
							return;
						}
						output = argv[arg];

					} else if( TESTARG2( "--quiet", "-q" )) {
						quiet = true;
					} else if( strmatch( argv[arg], "--help" ) ) {
						help = true;
					} else if( TESTARG2( "--title", "-t" )) {
						arg++;
						if( arg == argc ) {
							printf( "some kind of error.\n" );
							return;
						}
						strTitle = argv[arg];
					} else if( TESTARG2( "--author", "-a" )) {
						arg++;
						if( arg == argc ) {
							printf( "some kind of error.\n" );
							return;
						}
						strArtist = argv[arg];
					} else if( TESTARG2( "--copyright", "-c" )) {
						arg++;
						if( arg == argc ) {
							printf( "some kind of error.\n" );
							return;
						}
						strCopyright = argv[arg];
					} else if( TESTARG2( "--pack", "-p" ) ) {
						packmode = true;
					} else if( TESTARG2( "--unconv", "-u" ) ) {
						arg++;
						if( arg == argc ) {
							printf( "some kind of error.\n" );
							return;
						}
						reverse = true;
						nsfindex = atoi(argv[arg]) - 1;
					} else if( TESTARG2( "--multisamples", "-m" ) ) {
						arg++;
						if( arg == argc ) {
							printf( "some kind of error.\n" );
							return;
						}
						maxmulti = atoi(argv[arg]) - 1;
					}

					arg++;
					break;
				case ARG_INPUT:
					// input

					files.push_back( argv[arg] );
					arg++;
					break;
			}
		}
		
		// use first file as output nsf base filename if output is empty
		if( files.size() != 0 ) { 
			if( output.empty() ) {
				output = files[0];
				int extstart = output.find_last_of('.');
				output = output.substr( 0, extstart );

				if( packmode || reverse )
					output += ".it";
				else
					output += ".nsf";
			}
		}
	}
	
}
