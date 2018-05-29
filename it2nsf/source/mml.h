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

#ifndef __MML_H_
#define __MML_H_

#include "toad.h"

namespace MML {

    class Envelope {

    public:
        std::vector <u8> nodes;
        int susStart;
        int susEnd;
        int loop;
        bool valid;

        void setSingle(int value) {
            nodes.clear();
            nodes.push_back(value);
            susStart = -1;
            susEnd = -1;
            loop = 0;
            valid = true;
        }

        void unsignData();

        bool isSimple();

        bool isLoopSimple();
    };

    class Instrument {

    public:

        Instrument();

        std::string name;

        u8 type;
        u8 mdelay;
        u8 msweep;
        u8 mdepth;
        u8 mrate;

        u8 fm[8];

        u8 defaultVolume;

        int swidth;
        double cfreq;
        double fdetune;

        int sample;

        bool shortnoise;

        Envelope envVol;
        Envelope envPitch;
        Envelope envDuty;

        bool isSimple();

        void detune(int amount);

        void setType(int typ);

        void setSwidth(int width);

        int getPitchBase() const;
    };

    class Data {

    public:
        Data(const ToadLoader::Module &source);

        u8 channelMap[16];
        int channelCount;
        u8 n106table[128];
        u8 expansionChips;

        int defaultFadeout;

        std::vector<Instrument *> instruments;

    };

}

#endif

