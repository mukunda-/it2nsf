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

#ifndef __IO_H
#define __IO_H

#include <stdio.h>
#include "basetypes.h"

//-----------------------------------------------------------------------------
namespace IO {
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
    typedef enum {
//-----------------------------------------------------------------------------
                MODE_READ,
        MODE_WRITE
    } FileAccessMode;

//-----------------------------------------------------------------------------
    class File {
//-----------------------------------------------------------------------------
    private:
        FILE *file;
        bool fIsOpen;
        int origin;
        FileAccessMode mode;

    public:
        File();

        File(const char *, FileAccessMode);

        ~File();

        bool open(const char *, FileAccessMode);

        void close();

        u8 read8();

        u16 read16();

        u32 read32();

        void write8(u8);

        void write16(u16);

        void write32(u32);

        // write non-terminated ascii
        void writeAscii(const char *);

        // write fixed length string (zero padded)
        void writeAsciiF(const char *, int);

        void writeBytes(const u8 *bytes, int length);

        void zeroFill(int);

        void writeAlign(u32 boundary);

        void resetOrigin();

        void skip(s32 amount);

        u32 tell();

        void seek(u32 offset);

        bool isOpen();

        int bankIndex();

        int bankRemaining();

        int bankOffset();
    };
//-----------------------------------------------------------------------------

    bool fileExists();

    u32 fileSize(const char *filename);

//-----------------------------------------------------------------------------
};
//-----------------------------------------------------------------------------

#endif
