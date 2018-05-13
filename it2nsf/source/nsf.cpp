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

#include "nsf.h"
#include "inputdata.h"
#include "toad.h"
#include "math.h"
#include "mml.h"

//#define DEBUG
#define DEBUG_PROGRAM "../driver/driver.nsf"

extern bool QUIET;

namespace NSF {

#include "nsf_program.h"

    void getFDSsound(u8 *fdsSample, ToadLoader::Sample &samp) {
        for (int i = 0; i < 64; i++) {
            fdsSample[i] = 32;
        }
        if (samp.data.length != 0) {

            if (samp.data.bits16) {
                for (int i = 0; i < 64; i++) {
                    if (i >= samp.data.length)
                        fdsSample[i] = 0;
                    int a = samp.data.data16[i];
                    a += 32768;
                    a = (a + (1 << 9)) >> 10;
                    if (a > 63) a = 63;
                    fdsSample[i] = a;
                }
            } else {
                for (int i = 0; i < 64; i++) {
                    if (i >= samp.data.length)
                        fdsSample[i] = 0;
                    int a = samp.data.data8[i];
                    a += 128;
                    a = (a + 2) >> 2;
                    if (a > 63) a = 63;
                    fdsSample[i] = a;
                }
            }
        }
    }
/*
void getDefaultWavetable( u8 *dest ) {
	for( int i = 0; i < 128; i++ ) {
		dest[i] = n106default[i];
	}
}*/

/***********************************************************************************************
 *
 * Bank
 *
 ***********************************************************************************************/

    Bank::Bank(const ToadLoader::Bank &bank, const ConversionInput::OperationData &od) {

        strTitle = od.strTitle;
        strArtist = od.strArtist;
        strCopyright = od.strCopyright;

        for (int i = 0, n = bank.modules.size(); i < n; i++) {
            addModule(*(bank.modules[i]));
        }

    }

    void Bank::addModule(const ToadLoader::Module &mod) {

        if (!QUIET) {
            printf("-------------------\n");
            printf("Adding module, title=%s\n", mod.title);
        }
        modules.push_back(new Module(this, mod));
        if (!strTitle) {
            strTitle = mod.title;
        } else {
            if (strTitle[0] == 0) {
                strTitle = mod.title;
            }
        }
    }

    int Bank::addDPCM(const ToadLoader::Sample &samp) {
        // todo create dpcm
        // if exists, return index
        // otherwise add new index

        DPCMSample *s = new DPCMSample(samp.data);
        for (u32 i = 0; i < dpcm_samples.size(); i++) {
            if (s == dpcm_samples[i]) {
                delete s;
                return i;
            }
        }

        dpcm_samples.push_back(s);
        return dpcm_samples.size() - 1;
    }

    Bank::~Bank() {
        for (u32 i = 0; i < modules.size(); i++) {
            delete modules[i];
        }
        for (u32 i = 0; i < dpcm_samples.size(); i++) {
            delete dpcm_samples[i];
        }
    }

    int Bank::getDPCMbank(int index) {
        return dpcm_samples[index]->pointer.bank;
    }

    int Bank::getDPCMoffset(int index) {
        return (dpcm_samples[index]->pointer.address >> 6) & 0xFF;
    }

    int Bank::getDPCMlength(int index) {
        return dpcm_samples[index]->getDMClength();
    }

    int Bank::getDPCMloop(int index) {
        return dpcm_samples[index]->loop ? 1 : 0;
    }

/***********************************************************************************************
 *
 * Module
 *
 ***********************************************************************************************/

    Module::Module(Bank *fparent, const ToadLoader::Module &mod) {

        parent = fparent;

        initialVolume = mod.globalVolume;
        initialTempo = mod.initialTempo;
        initialSpeed = mod.initialSpeed;

        length = mod.length > 255 ? 255 : mod.length;

        for (int i = 0; i < length; i++) {
            sequence[i] = mod.orders[i];
            if (sequence[i] == 255) {
                length = i + 1;
                break;
            }
        }

        MML::Data mml(mod);
        expansionChips = mml.expansionChips;
        numChannels = mml.channelCount;

        for (u32 i = 0; i < 128; i++) {
            n106wavetable[i] = mml.n106table[i];
        }

        for (u32 i = 0; i < mod.patterns.size(); i++) {
            if (mod.patterns[i]) {
                patterns.push_back(new Pattern(*(mod.patterns[i]), numChannels));
            } else {
                patterns.push_back(new Pattern());
            }
        }


        for (int i = 0; i < numChannels; i++) {
            channelMap[i] = mml.channelMap[i];
        }

        for (u32 i = 0; i < mml.instruments.size(); i++) {
            instruments.push_back(new Instrument(this, mod, *(mml.instruments[i])));
        }
    }

    Module::~Module() {
        for (u32 i = 0; i < patterns.size(); i++)
            delete patterns[i];

        for (u32 i = 0; i < instruments.size(); i++)
            delete instruments[i];
    }

    int Module::getExportSize() {
        int size = 0x60; // base size
        size += length; // sequence
        size += patterns.size() * 3; // pattern table
        size += instruments.size() * 3; // instrument table
        return size;
    }

/***********************************************************************************************
 *
 * Pattern
 *
 ***********************************************************************************************/

    Pattern::Pattern() {

    }

    Pattern::Pattern(ToadLoader::Pattern &source, int chcount) {
        rows = (u8) (source.rows);

        int row;
        u8 *read = source.data;

        int nchannels = 0;

        if (source.dataLength != 0) {

            u8 p_maskvar[64];

            std::vector <u8> rowbuffer[2];
            u8 databits[2] = {0};

            for (row = 0; row < source.rows;) {
                u8 chvar = *read++;

                if (chvar == 0) {
                    data.push_back(databits[0]);
                    databits[0] = 0;
                    data.insert(data.end(), rowbuffer[0].begin(), rowbuffer[0].end());
                    rowbuffer[0].clear();

                    if (chcount >= 8) {
                        data.push_back(databits[1]);
                        databits[1] = 0;
                        data.insert(data.end(), rowbuffer[1].begin(), rowbuffer[1].end());
                        rowbuffer[1].clear();
                    } else {
                        databits[1] = 0;
                        rowbuffer[1].clear();
                    }

                    row++;
                    continue;
                }

                int channel = (chvar - 1) & 63;
                int pindex;
                if (channel < 8) pindex = 0;
                else if (channel < 16) pindex = 1;
                else pindex = -1;

                if (channel >= nchannels)
                    nchannels = channel + 1;

                if (channel < 8)
                    databits[0] |= 1 << channel;
                else
                    databits[1] |= 1 << (channel - 8);

                u8 maskvar;
                if (chvar & 128) {
                    maskvar = *read++;
                    maskvar |= maskvar << 4;
                    p_maskvar[channel] = maskvar;
                } else {
                    maskvar = p_maskvar[channel];
                }

#define pushbuffer(xxx) { int val = (xxx); if( pindex != -1 ) rowbuffer[pindex].push_back( (u8)val ); }

                pushbuffer(maskvar)

                if (maskvar & 1) {        // note
                    pushbuffer(*read++)
                }
                if (maskvar & 2) {        // instr
                    pushbuffer(*read++ - 1) // ADJUST FOR -1 INSTRUMENT...
                }
                if (maskvar & 4) {        // vcmd
                    pushbuffer(*read++)
                }

                if (maskvar & 8) {        // cmd+param

                    pushbuffer(*read++)
                    pushbuffer(*read++)
                }

            }
        } else {
            // empty pattern
            for (int i = 0; i < source.rows; i++) {
                data.push_back(0);

                if (chcount >= 8)
                    data.push_back(0);
            }
        }

        if (nchannels > chcount) {
            if (!QUIET) {
                printf("a pattern uses more channels than it's supposed to...\n");
            }
            nchannels = chcount;
        }

        numChannels = nchannels;
    }

    u32 Pattern::getCRC() {
        u32 crc = 0;
        for (u32 i = 0; i < data.size(); i++) {
            crc += data[i];
        }
        return crc;
    }

/***********************************************************************************************
 *
 * Instrument
 *
 ***********************************************************************************************/

    Instrument::Instrument(Module *fparent, const ToadLoader::Module &mod, const MML::Instrument &source) {
        parent = fparent;
        defaultVolume = source.defaultVolume;

        //double a = (double)source.data.centerFreq;
        //pitchBase = (int)floor(log(a / 8363.0) / log(2.0) * 768.0 + 0.5);
        // todo: tuning
        pitchBase = source.getPitchBase();

        type = source.type;
        defaultVolume = source.defaultVolume;
        modDelay = source.mdelay;
        modSweep = source.msweep;
        modDepth = source.mdepth;
        modRate = source.mrate;
        shortnoise = source.shortnoise;

        envelopemask = 0;
        if (convertEnvelope(envVolume, source.envVol))
            envelopemask |= 1;
        if (convertEnvelope(envPitch, source.envPitch))
            envelopemask |= 2;
        if (convertEnvelope(envDuty, source.envDuty))
            envelopemask |= 4;

        if (type == 11) { // fds sound
            ToadLoader::Sample &samp = *mod.samples[source.sample];
            getFDSsound(fdsSample, samp);


        } else if (type == 3) { // dpcm
            ToadLoader::Sample &samp = *mod.samples[source.sample];
            dpcmIndex = parent->getParent()->addDPCM(samp);
        } else if (type == 8) { // vrc7c
            for (int i = 0; i < 8; i++) {
                vrc7custom[i] = source.fm[i];
            }
        }
    }

    bool Instrument::convertEnvelope(Envelope &dest, const MML::Envelope &src) {
        dest.nodes.clear();
        if (!src.valid) {
            return false;
        }

        int susStart, susEnd, loopStart;
        for (u32 i = 0; i < src.nodes.size(); i++) {
            if (i == src.susStart) {
                susStart = dest.nodes.size();
            } else if (i == src.susEnd) {
                susEnd = dest.nodes.size();
                dest.nodes.push_back(254);
                dest.nodes.push_back(susStart + 1);
            } else if (i == src.loop) {
                loopStart = dest.nodes.size();
            }
            dest.nodes.push_back(src.nodes[i]);
        }
        dest.nodes.push_back(255);
        dest.nodes.push_back(loopStart + 1);

        return true;
    }

    int Instrument::getExportSize() {
        int size = 25; // base size
        if (envelopemask & 1)
            size += 1 + envVolume.nodes.size();

        if (envelopemask & 2)
            size += 1 + envPitch.nodes.size();

        if (envelopemask & 4)
            size += 1 + envDuty.nodes.size();

        if (type == 0xB) { // fds sample
            size += 64;
        }
        return size;
    }


/***********************************************************************************************
 *
 * DPCMSample
 *
 ***********************************************************************************************/

    DPCMSample::DPCMSample() {
        reset();
    }

    void DPCMSample::reset() {
        loop = 0;
        dataLength = 0;
        pointer.address = 0;
        pointer.bank = 0;
        dpcmLen = 0;
        data.clear();
    }

    DPCMSample::DPCMSample(const ToadLoader::SampleData &source) {
        createFromSample(source);
    }

    void DPCMSample::createFromSample(const ToadLoader::SampleData &source) {

        reset();

        loop = source.loop;

        s8 pos = 32; // default dpcm pos

        int length = source.length;
        length += ((16 - (length % 16)) % 16); // pad to 16 bytes
        length += 16; // more padding!
        length = length / 8;
        length++; // one more byte

        for (int i = 0; i < length; i++) {

            u8 buffer = 0;

            for (int bit = 0; bit < 8; bit++) {

                u8 sample;
                buffer >>= 1; // shift buffer

                if ((i * 8 + bit) > source.length) { // past end of sample
                    sample = 0;
                } else {
                    // read sample converted to 6 bits unsigned
                    if (source.bits16) {
                        int a = source.data16[i * 8 + bit] + 32768;
                        a = (a + 512) >> 10;
                        if (a > 63) a = 63;
                        sample = (u8) a;
                    } else {
                        int a = source.data8[i * 8 + bit] + 128;
                        a = (a + 2) >> 2;
                        if (a > 63) a = 63;
                        sample = (u8) a;
                    }
                }

                if (pos > sample || (pos == 0 && sample == 0)) {
                    pos--;
                    if (pos < 0) pos = 0;
                } else {
                    buffer |= 0x80;
                    pos++;
                    if (pos > 63)
                        pos = 63;
                }
            }

            data.push_back(buffer);
        }
    }

    bool DPCMSample::operator==(const DPCMSample &test) const {
        if (getDataLength() != test.getDataLength())
            return false;
        for (int i = 0; i < getDataLength(); i++) {
            if (data[i] != test.data[i]) {
                return false;
            }
        }
        return true;
    }

/***********************************************************************************************
 *
 * Export
 *
 ***********************************************************************************************/

    void Bank::exportNSF(const char *nsf) const {
        std::string nstr;

        if (!QUIET)
            printf("exporting NSF...\n");

        IO::File file(nsf, IO::MODE_WRITE);

        // denotes an NES sound formt file
        file.writeAscii("NESM");
        file.write8(0x1A);

        // version
        file.write8(0x01);

        // total song
        file.write8(modules.size());

        // starting song
        file.write8(1);

        // load address of data
        file.write16(0x8000);

        // init address of data
        file.write16(0x8000);

        // play address of data
        file.write16(0x8003);

        // write title
        if (strTitle) {
            file.writeAsciiF(strTitle, 31);
            file.write8(0);
        } else {
            file.zeroFill(32);
        }

        // write artist
        if (strArtist) {
            file.writeAsciiF(strArtist, 31);
            file.write8(0);
        } else {
            file.zeroFill(32);
        }

        // write copyright
        if (strCopyright) {
            file.writeAsciiF(strCopyright, 31);
            file.write8(0);
        } else {
            file.zeroFill(32);
        }

        // NTSC speed (60hz)
        file.write16(16667);

        // bankswitch init values
        file.write32(0x03020100);
        file.write32(0x07060504);

        // PAL speed (not supported, also 60hz)
        file.write16(16667);

        // PAL/NTSC bits (NTSC)
        file.write8(0);

        // expansion chips used
        {
            int chips = 0;
            for (u32 i = 0; i < modules.size(); i++) {
                chips |= modules[i]->expChipsUsed();
            }
            file.write8(chips);
        }

        // 4 extra expansion bytes
        file.write32(0);

        file.resetOrigin();

        char drv_version[13];
        drv_version[12] = 0;

        if (!QUIET)
            printf("driver version: ");

#ifndef DEBUG
        int program_size = nsf_program[0x80 + 6] + nsf_program[0x80 + 7] * 256;
        for (int i = 0; i < 12; i++)
            drv_version[i] = nsf_program[0x80 + 24 + i];
        file.writeBytes(nsf_program + 0x80, program_size);
#else

        // debug: load driver from newest build
        FILE *fdrv = fopen( DEBUG_PROGRAM, "rb" );
        fseek( fdrv, 0, SEEK_END );
        int nsf_size = ftell(fdrv);
        u8 *drvdata = new u8[nsf_size];
        fseek( fdrv, 0, SEEK_SET );
        fread( drvdata, 1, nsf_size, fdrv );
        fclose( fdrv );
        int program_size = drvdata[0x80+6] + drvdata[0x80+7]*256;
        file.writeBytes( drvdata + 0x80, program_size );
        for( int i = 0; i < 12; i++ )
            drv_version[i] = nsf_program[0x80+24+i];
        delete drvdata;
#endif

        if (!QUIET)
            printf("%s\n", drv_version);

        // write #modules
        file.write8(modules.size());

        // reserve space for module table
        int moduleTableStart = file.tell();
        file.zeroFill(modules.size() * 3);

        int v_dpcm = 0;

        if (!QUIET)
            printf("exporting DPCM...\n");
        // export dpcm samples
        for (u32 i = 0; i < dpcm_samples.size(); i++) {

            dpcm_samples[i]->exportData(file);
            v_dpcm += dpcm_samples[i]->getExportSize();
        }

        std::vector < Module * > moduleList;
        std::vector < Instrument * > instrumentList;
        std::vector < Pattern * > patternList;

        int v_modulesize = 0, v_instrumentsize = 0, v_patternsize = 0;

        // populate lists
        for (u32 i = 0; i < modules.size(); i++) {
            Module *mod = modules[i];
            moduleList.push_back(mod);
            v_modulesize += mod->getExportSize();
            for (u32 j = 0; j < mod->instruments.size(); j++) {
                instrumentList.push_back(mod->instruments[j]);
                v_instrumentsize += mod->instruments[j]->getExportSize();
            }
            for (u32 j = 0; j < mod->patterns.size(); j++) {
                patternList.push_back(mod->patterns[j]);
                v_patternsize += mod->patterns[j]->getExportSize();
            }
        }

        if (!QUIET) {
            printf("total data size (excluding alignments!): %i\n",
                   v_dpcm + v_modulesize + v_instrumentsize + v_patternsize + modules.size() * 3 + 1);
            printf("(%i bytes module table)\n", modules.size() * 3 + 1);
            printf("(%i bytes module info)\n", v_modulesize);
            printf("(%i bytes instrument data)\n", v_instrumentsize);
            printf("(%i bytes pattern data)\n", v_patternsize);
            printf("(%i bytes dpcm sample data)\n", v_dpcm);
            printf("nsf driver size: %i bytes\n", program_size);
        }

        if (!QUIET)
            printf("exporting module info/instruments/patterns\n");

        // export modules + instruments + maybe some patterns
        while ((!moduleList.empty()) || (!instrumentList.empty())) {

            bool found = false;
            int remaining = file.bankRemaining();

            // search for module to store
            for (u32 i = 0; i < moduleList.size(); i++) {
                if (moduleList[i]->getExportSize() < remaining) {
                    moduleList[i]->exportData(file);
                    moduleList.erase(moduleList.begin() + i);
                    found = true;
                    break;
                }
            }
            if (found)
                continue;

            // search for instrument to store
            for (u32 i = 0; i < instrumentList.size(); i++) {
                if (instrumentList[i]->getExportSize() < remaining) {

                    instrumentList[i]->exportData(file);

                    instrumentList.erase(instrumentList.begin() + i);
                    found = true;
                    break;
                }
            }
            if (found)
                continue;

            // store/pop a pattern if there is one
            if (!patternList.empty()) {
                patternList[0]->exportData(file);
                patternList.erase(patternList.begin());
            } else {
                // otherwise just zeropad until next bank
                file.zeroFill(file.bankRemaining());
            }
        }

        // export remaining patterns
        for (u32 i = 0; i < patternList.size(); i++) {
            patternList[i]->exportData(file);
        }

        if (!QUIET)
            printf("writing address tables...\n");


        // write addresses
        file.seek(moduleTableStart);
        for (u32 i = 0; i < modules.size(); i++)
            file.write8(modules[i]->getPointer()->address & 0xFF);
        for (u32 i = 0; i < modules.size(); i++)
            file.write8((modules[i]->getPointer()->address >> 8) + 0xC0);
        for (u32 i = 0; i < modules.size(); i++)
            file.write8(modules[i]->getPointer()->bank);

        for (u32 i = 0; i < modules.size(); i++) {
            modules[i]->exportAddressTable(file);
        }

        if (!QUIET)
            printf("embedding animal...\n");

        file.seek(6);
        const char *amn = getAnimal();
        file.writeAsciiF(amn, 10);

        file.close();
        if (!QUIET)
            printf("operation success!\n");

    }

    void DPCMSample::exportData(IO::File &file) {
        file.writeAlign(64);

        pointer.address = file.bankOffset();
        pointer.bank = file.bankIndex();

        for (u32 i = 0; i < data.size(); i++) {
            file.write8(data[i]);
        }

    }

    void Module::exportData(IO::File &file) {

        pointer.address = file.bankOffset();
        pointer.bank = file.bankIndex();

        file.write8(initialTempo);
        file.write8(initialVolume);
        file.write8(initialSpeed);

        file.write8(patterns.size());
        file.write8(instruments.size());
        file.write8(length);
        file.write8(numChannels);
        file.write8(expansionChips);

        file.zeroFill(0x10 - 0x8);

        // export wavetable
        for (int i = 0; i < 64; i++)
            file.write8(n106wavetable[i * 2] + (n106wavetable[(i * 2) + 1] << 4));

        for (int i = 0; i < 16; i++)
            file.write8(channelMap[i]);

        for (int i = 0; i < length; i++)
            file.write8(sequence[i]);

        // reserve space for address table
        file.zeroFill(instruments.size() * 3 + patterns.size() * 3);
    }

    void Module::exportAddressTable(IO::File &file) const {
        file.seek(pointer.bank * 4096 + pointer.address + 0x60 + length);

        for (u32 i = 0; i < patterns.size(); i++)
            file.write8(patterns[i]->getPointer()->address & 0xFF);
        for (u32 i = 0; i < patterns.size(); i++)
            file.write8((patterns[i]->getPointer()->address >> 8) + 0xC0);
        for (u32 i = 0; i < patterns.size(); i++)
            file.write8(patterns[i]->getPointer()->bank);

        for (u32 i = 0; i < instruments.size(); i++)
            file.write8(instruments[i]->getPointer()->address & 0xFF);
        for (u32 i = 0; i < instruments.size(); i++)
            file.write8((instruments[i]->getPointer()->address >> 8) + 0xD0);
        for (u32 i = 0; i < instruments.size(); i++)
            file.write8(instruments[i]->getPointer()->bank);
    }

    void Pattern::exportData(IO::File &file) {

        pointer.address = file.bankOffset();
        pointer.bank = file.bankIndex();

        file.write8(rows);
        for (u32 i = 0; i < data.size(); i++) {
            u8 a = data[i];
            file.write8(data[i]);
        }
    }

    void Instrument::exportData(IO::File &file) {

        pointer.address = file.bankOffset();
        pointer.bank = file.bankIndex();

        file.write8(type);
        file.write8(defaultVolume);
        file.write16(pitchBase);
        file.write8(modDelay);
        file.write8(modSweep);
        file.write8(modDepth);
        file.write8(modRate);

        file.write8(envelopemask);

        bool dpcmLoop = false;

        // type specific data
        switch (type) {
            case 2: // noise
                file.write8(shortnoise ? 128 : 0);
                file.zeroFill(16 - 1);
                break;
            case 3: // dpcm
                file.write8(parent->getParent()->getDPCMbank(dpcmIndex));
                file.write8(parent->getParent()->getDPCMoffset(dpcmIndex) + 128);
                file.write8(parent->getParent()->getDPCMlength(dpcmIndex));
                dpcmLoop = !!parent->getParent()->getDPCMloop(dpcmIndex);
                file.write8(dpcmLoop ? 64 : 0);
                file.zeroFill(16 - 4);
                break;
            case 8:
                for (int i = 0; i < 8; i++) {
                    file.write8(vrc7custom[i]);
                }
                file.zeroFill(16 - 8);
                break;
            default:
                file.zeroFill(16);
        }

        if (envelopemask & 1)
            envVolume.exportData(file, 64);
        if (envelopemask & 2)
            envPitch.exportData(file, -1);
        if (envelopemask & 4)
            envDuty.exportData(file, type | (dpcmLoop ? 0x100 : 0));

        if (type == 0xB) {
            // fds sample
            for (int i = 0; i < 64; i++) {
                file.write8(fdsSample[i]);
            }
        }
    }

    void Envelope::exportData(IO::File &file, int convert) {
        file.write8(nodes.size() + 1);
        bool special = false;
        for (u32 i = 0; i < nodes.size(); i++) {
            u8 a = nodes[i];
            if (a < 254) {
                if (!special && convert != -1) {
                    switch (convert & 0xFF) {
                        case 0: // 2a03 pulse, mmc5
                        case 9:
                            a = (a << 6) | (0x30);
                            break;
                        case 4: // vrc6 square
                        case 7: // vrc7 stock
                            a = a << 4;
                            break;
                        case 64: // volume envelope
                            if (a != 16) {
                                int b = a ^0xF;
                                a = 0;
                                if (b & 1) a |= 8;
                                if (b & 2) a |= 4;
                                if (b & 4) a |= 2;
                                if (b & 8) a |= 1;
                            } else {
                                a = 128;
                            }

                            break;
                    }

                    if (convert & 0x100) {
                        a |= 0x40;
                    }
                } else {
                    special = false;
                }
            } else {
                special = true;
            }
            file.write8(a);
        }
    }

    static const char *danimals[] = {
            "angler", "ant", "antelope", "arctic fox",
            "armadillo", "ass",
            "baboon", "bacteria", "badger", "bandicoot",
            "bear", "beaver", "bison", "boar",
            "bobcat", "buffalo", "bunny",
            "camel", "canary", "capybara", "catfish",
            "centipede", "cerberus", "cheetah", "chinchilla",
            "chimp", "chipmunk", "cobra", "cockatoo",
            "cockroach", "cougar", "coyote", "crab",
            "crocodile", "crow", "cucumber",
            "dingo", "dog", "dogfish", "dolphin",
            "donkey", "dragon", "DUCK.",//HIGHSCORE
            "elephant", "eel",
            "falcon", "ferret", "firefly", "fox",
            "frog",
            "'gator", "gecko", "gerbil", "giraffe",
            "goldfish", "goose", "gopher", "gorilla",
            "griffin", "ground hog", "guinea pig",
            "hamster", "hedgehog", "hippo", "horsey",
            "hydra", "hyena",
            "iguana",
            "jackal", "jaguar", "jelly fish", "jerboa",
            "kangaroo", "kinkajou", "kitten", "koala",
            "kraken",
            "leopard", "lion", "lionfish", "lizard",
            "llama", "lobster",
            "mandrill", "manta ray", "manticore", "mantis",
            "masked owl", "minotaur", "mole", "mongoose",
            "monkey", "mosquito", "mouse", "mule",
            "muskrat", "musk deer", "mustang",
            "narwhal", "nautilus", "newt",
            "ocelot", "octopus", "opossum", "ostridge",
            "otter",
            "panda", "panther", "parakeet", "parrot",
            "penguin", "pig", "piranha", "platypus",
            "pony", "porcupine", "puppy", "pygmy",
            "python",
            "quack", "quail",
            "raccoon", "raven", "rhino", "rodent",
            "rooster",
            "salamander", "scorpion", "seal", "serval",
            "shark", "sheep", "shrew", "shrimp",
            "skunk", "slug", "sphinx", "spider",
            "squid", "squirrel", "stallion", "stingray",
            "tiger", "toad", "tortoise", "turkey",
            "turtle", "t-rex",
            "unicorn",
            "vicuna", "vulture",
            "walrus", "warthog", "wasp", "waterbuck",
            "weasel", "whale", "wildcat", "wolf",
            "wolverine", "wombat", "woodchuck", "worm",
            "yak",
            "zebra",
            0
    };

    static int animalcount() {
        for (int i = 0;; i++)
            if (!danimals[i])
                return i;
    }

    const char *Bank::getAnimal() const {
        // get crc of pattern data
        u32 crc = 0;
        for (u32 m = 0; m < modules.size(); m++) {
            for (u32 i = 0; i < modules[m]->patterns.size(); i++) {
                crc += modules[m]->patterns[i]->getCRC();
            }
        }
        // choose animal
        return danimals[crc % animalcount()];

    }

} //namespace
