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

#include "io.h"

namespace IO {

	File::File() {
		
	}

	File::File( const char *filename, FileAccessMode m ) {
		fIsOpen = false;
		open( filename, m );
	}

	File::~File() {
		close();
	}

	bool File::open( const char *filename, FileAccessMode m ) {
		if( fIsOpen ) {
			return false;
		}
		origin = 0;
		mode = m;
		file = fopen( filename, m ? "wb" : "rb" );

		fIsOpen = file ? true : false;
		return fIsOpen;
	}

	void File::close() {
		if( fIsOpen ) {
			fclose( file );
			file = 0;
			fIsOpen = false;
		}
	}

	u8 File::read8() {
		if( mode == MODE_WRITE || (!fIsOpen) ) return 0;
		u8 a[1];
		fread( &a, 1, 1, file );
		return a[0];
	}

	u16 File::read16() {
		u16 a = read8();
		a |= read8() << 8;
		return a;
	}

	u32 File::read32() {
		u32 a = read16();
		a |= read16() << 16;
		return a;
	}

	void File::skip( int amount ) {
		if( fIsOpen ) {
			if( mode == MODE_WRITE ) {
				for( u32 i = amount; i; i-- ) {
					write8( 0 );
				}
			} else {
				fseek( file, amount, SEEK_CUR );
			}
		}
	}

	void File::write8( u8 data ) {
		if( fIsOpen && mode == MODE_WRITE ) {
			fwrite( &data, 1, 1, file );
		}
	}

	void File::write16( u16 data ) {
		write8( data & 0xFF );
		write8( data >> 8 );
	}

	void File::write32( u32 data ) {
		write16( data & 0xFFFF );
		write16( data >> 16 );
	}

	void File::writeAscii( const char * str ) {
		for( int i = 0; str[i]; i++ ) {
			write8( str[i] );
		}
	}

	void File::writeAsciiF( const char * str, int length ) {
		int i;
		for( i = 0; str[i] && i < length; i++ ) {
			write8( str[i] );
		}
		for( ; i < length; i++ ) {
			write8( 0 );
		}
	}

	void File::writeBytes( const u8 *bytes, int length ) {
		if( fIsOpen && mode == MODE_WRITE ) {
			fwrite( bytes, 1, length, file );
		}
	}
	
	void File::zeroFill( int amount ) {
		for( int i = 0; i  < amount; i++ ) {
			write8( 0 );
		}
	}

	void File::writeAlign( u32 boundary ) {
		int skipa = tell() % boundary;
		if( skipa ) {
			skip( boundary - skipa );
		}
	}

	void File::seek( u32 amount ) {
		if( fIsOpen )
			fseek( file, origin + amount, SEEK_SET );
	}

	u32 File::tell() {
		if( fIsOpen )
			return ftell( file ) - origin;
		else
			return 0;
	}

	void File::resetOrigin() {
		if( fIsOpen )
			origin = ftell( file );
	}

	bool File::isOpen() {
		return fIsOpen;
	}
	
	bool fileExists( const char *filename ) {
		FILE *f = fopen( filename, "rb" );
		bool result = f ? true : false;
		fclose(f);
		return result;
	}

	u32 fileSize( const char *filename ) {
		FILE *f = fopen( filename, "rb" );
		if( !f ) {
			return 0;
		}
		fseek( f, 0, SEEK_END );
		int size = ftell(f);
		fclose(f);
		return size;
	}

	int File::bankIndex() {
		return tell() >> 12;
	}
	
	int File::bankRemaining() {
		return 4096 - (tell() & 4095);
	}
	
	int File::bankOffset() {
		return tell() & 4095;
	}
}
