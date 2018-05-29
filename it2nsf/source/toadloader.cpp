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
#include "io.h"

namespace ToadLoader {


    namespace {

        template<typename T>
        static void deletePtrVector(std::vector<T *> &vecs) {

            for (typename std::vector<T *>::iterator iter = vecs.begin(), ending = vecs.end();
                 iter != ending;
                 iter++) {

                if (*iter)
                    delete (*iter);
            }

            vecs.clear();
        }

        void copyFstr(char *dest, const char *source, int length) {
            dest[length - 1] = 0;
            length--;
            bool remaining = true;
            for (int i = 0; i < length; i++) {
                if (remaining) {
                    if (source[i]) {
                        dest[i] = source[i];
                    } else {
                        remaining = false;
                        dest[i] = 0;
                    }
                } else {
                    dest[i] = 0;
                }
            }
        }
    }

    Bank::Bank(const std::vector<const char *> &files) {
        for (u32 i = 0; i < files.size(); i++) {

            addModule(files[i]);
        }
    }

    Bank::~Bank() {
        deletePtrVector<Module>(modules);
    }

    void Bank::addModule(const char *filename) {

        Module *m = new Module();
        m->load(filename);
        modules.push_back(m);
    }

    Module::Module() {
        for (int i = 0; i < 26; i++) title[i] = 0;
        patternHighlight = 0; //?
        length = 0;
        cwtv = 0x0214;
        cmwt = 0x0214;
        flgStereo = true;
        instrumentMode = true;
        vol0MixOptimizations = false;
        linearSlides = true;
        oldEffects = false;
        gxxCompat = true;
        globalVolume = 128;
        mixingVolume = 80;
        initialSpeed = 6;
        initialTempo = 125;
        sep = 128; //?
        PWD = 0;
        messageLength = 0;
        message = 0;

        for (int i = 0; i < 64; i++) {
            channelPan[i] = 0;
            channelVolume[i] = 0;
        }

        for (int i = 0; i < 256; i++)
            orders[i] = 0;
    }

    Module::~Module() {
        deletePtrVector<Instrument>(instruments);
        deletePtrVector<Sample>(samples);
        deletePtrVector<Pattern>(patterns);

        if (message)
            delete[] message;
    }

    void Module::load(const char *pFilename) {
        IO::File file(pFilename, IO::MODE_READ);

        filename = pFilename;

        // skip "IMPM" string
        file.skip(4);

        // read title
        for (int i = 0; i < 26; i++)
            title[i] = file.read8();
        title[26] = 0;

        patternHighlight = file.read16();

        length = file.read16();
        int instrumentCount = file.read16();
        int sampleCount = file.read16();
        int patternCount = file.read16();
        cwtv = file.read16();
        cmwt = file.read16();

        { // read flags
            int flags = file.read16();

            flgStereo = !!(flags & 1);
            vol0MixOptimizations = !!(flags & 2);
            instrumentMode = !!(flags & 4);
            linearSlides = !!(flags & 8);
            oldEffects = !!(flags & 16);
            gxxCompat = !!(flags & 32);
        }
        int special = file.read16();
        globalVolume = file.read8();
        mixingVolume = file.read8();
        initialSpeed = file.read8();
        initialTempo = file.read8();

        sep = file.read8();
        PWD = file.read8();
        messageLength = file.read16();

        u32 messageOffset = file.read32();

        file.skip(4); // reserved

        for (int i = 0; i < 64; i++)
            channelPan[i] = file.read8();

        for (int i = 0; i < 64; i++)
            channelVolume[i] = file.read8();

        bool foundend = false;
        int actualLength = length;
        for (int i = 0; i < 256; i++) {
            orders[i] = i < length ? file.read8() : 255;
            if (orders[i] == 255 && !foundend) {
                foundend = true;
                actualLength = i + 1;
            }
        }

        length = actualLength;

        u32 *instrTable = new u32[instrumentCount + 1];
        u32 *sampleTable = new u32[sampleCount + 1];
        u32 *patternTable = new u32[patternCount + 1];

        for (int i = 0; i < instrumentCount; i++)
            instrTable[i] = file.read32();
        for (int i = 0; i < sampleCount; i++)
            sampleTable[i] = file.read32();
        for (int i = 0; i < patternCount; i++)
            patternTable[i] = file.read32();

        for (int i = 0; i < instrumentCount; i++) {
            if (instrTable[i]) {
                file.seek(instrTable[i]);
                Instrument *ins = new Instrument();
                ins->load(file);
                instruments.push_back(ins);
            } else {
                instruments.push_back(0);
            }
        }

        for (int i = 0; i < sampleCount; i++) {
            if (sampleTable[i]) {
                file.seek(sampleTable[i]);
                Sample *smp = new Sample();
                smp->load(file);
                samples.push_back(smp);
            } else {
                samples.push_back(0);
            }
        }

        for (int i = 0; i < patternCount; i++) {
            if (patternTable[i]) {
                file.seek(patternTable[i]);
                Pattern *p = new Pattern();
                p->load(file);
                patterns.push_back(p);
            } else {
                patterns.push_back(0);
            }
        }

        if (special & 1) {
            message = new char[messageLength + 2];
            file.seek(messageOffset);
            message[messageLength] = 0;

            for (int i = 0; i < messageLength; i++)
                message[i] = file.read8();
        } else {
            message = 0;
        }

        delete[] instrTable;
        delete[] sampleTable;
        delete[] patternTable;
    }

    void Module::save(const char *filename) {
        IO::File file(filename, IO::MODE_WRITE);

        file.writeAscii("IMPM");
        file.writeAsciiF(title, 25);
        file.write8(0);
        file.write16(patternHighlight);
        file.write16(length);
        file.write16(instruments.size());
        file.write16(samples.size());
        file.write16(patterns.size());
        file.write16(cwtv);
        file.write16(cmwt);
        file.write16(buildFlags());
        file.write16(message ? 1 : 0);
        file.write8(globalVolume);
        file.write8(mixingVolume);
        file.write8(initialSpeed);
        file.write8(initialTempo);
        file.write8(sep);
        file.write8(PWD);
        file.write16(messageLength);

        u32 pointerToMessageOffset = file.tell();

        file.write32(0); //message offset fill in later
        file.write32(0); // reserved
        for (int i = 0; i < 64; i++)
            file.write8(channelPan[i]);
        for (int i = 0; i < 64; i++)
            file.write8(channelVolume[i]);
        for (int i = 0; i < length; i++)
            file.write8(orders[i]);

        u32 pointerToInstrumentTable = file.tell();
        for (u32 i = 0; i < instruments.size(); i++)
            file.write32(0);

        u32 pointerToSampleTable = file.tell();
        for (u32 i = 0; i < samples.size(); i++)
            file.write32(0);

        u32 pointerToPatternTable = file.tell();
        for (u32 i = 0; i < patterns.size(); i++)
            file.write32(0);

        u32 messageOffset = file.tell();
        if (message) {
            file.writeBytes((const u8 *) message, messageLength);
        }

        // export instruments
        std::vector <u32> insPointers;
        for (u32 i = 0; i < instruments.size(); i++) {
            if (instruments[i]) {
                insPointers.push_back(file.tell());
                instruments[i]->save(file);
            } else {
                insPointers.push_back(0);
            }
        }

        std::vector <u32> smplPointers;
        for (u32 i = 0; i < samples.size(); i++) {
            if (samples[i]) {
                smplPointers.push_back(file.tell());
                samples[i]->save(file);
            } else {
                smplPointers.push_back(0);
            }
        }

        std::vector <u32> patPointers;
        for (u32 i = 0; i < patterns.size(); i++) {
            if (patterns[i]) {
                patPointers.push_back(file.tell());
                patterns[i]->save(file);
            } else {
                patPointers.push_back(0);
            }
        }

        std::vector <u32> smpDataPointers;
        for (u32 i = 0; i < samples.size(); i++) {
            if (samples[i]) {
                smpDataPointers.push_back(file.tell());
                samples[i]->data.save(file);
            } else {
                smpDataPointers.push_back(0);
            }
        }


        file.seek(pointerToMessageOffset);
        file.write32(messageOffset);
        file.seek(pointerToInstrumentTable);
        for (u32 i = 0; i < instruments.size(); i++)
            file.write32(insPointers[i]);
        file.seek(pointerToSampleTable);
        for (u32 i = 0; i < samples.size(); i++)
            file.write32(smplPointers[i]);
        file.seek(pointerToPatternTable);
        for (u32 i = 0; i < patterns.size(); i++)
            file.write32(patPointers[i]);

        for (u32 i = 0; i < samples.size(); i++) {
            if (smplPointers[i]) {
                file.seek(smplPointers[i] + 0x48);
                file.write32(smpDataPointers[i]);
            }
        }
    }

    void Module::purgeInstruments() {
        deletePtrVector<Instrument>(instruments);
        instruments.clear();

//	deletePtrVector<Sample>( samples );
//	deletePtrVector<Pattern>( patterns );

    }

    void Module::purgeMarkedSamples(const char *marker) {
        for (u32 i = 0; i < samples.size(); i++) {
            if (samples[i]) {
                if (samples[i]->testNameMarker(marker)) {
                    delete samples[i];
                    samples[i] = 0;
                }
            }
        }
        trimSampleVector();
    }

    void Module::trimSampleVector() {
        if (samples.empty())
            return;
        for (int i = (int) samples.size() - 1;; i--) {
            if (i >= 0) {
                if (!samples[i]) {
                    samples.pop_back();
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    }

    u8 Module::buildFlags() {
        int flags = 0;
        if (flgStereo) flags |= 1;
        if (vol0MixOptimizations) flags |= 2;
        if (instrumentMode) flags |= 4;
        if (linearSlides) flags |= 8;
        if (oldEffects) flags |= 16;
        if (gxxCompat) flags |= 32;
        return flags;
    }

    void Module::setTitle(const char *text) {
        copyFstr(title, text, 27);
    }

    Instrument::Instrument() {
        for (int i = 0; i < 27; i++) name[i] = 0;
        for (int i = 0; i < 13; i++) DOSFilename[i] = 0;
        newNoteAction = 0;
        duplicateCheckType = 0;
        duplicateCheckAction = 0;
        fadeout = 0;
        PPS = 0;
        PPC = 60;
        globalVolume = 128;
        defaultPan = 32 | 128;
        randomVolume = 0;
        randomPanning = 0;
        trackerVersion = 0x0214;
        initialFilterCutoff = 0;
        initialFilterResonance = 0;

        midiChannel = 0;
        midiProgram = 0;
        midiBank = 0;

        for (int i = 0; i < 120; i++) {
            notemap[i].note = i;
            notemap[i].sample = 0;
        }
    }

    void Instrument::load(IO::File &file) {
        file.skip(4); // "IMPI"
        DOSFilename[12] = 0;
        for (int i = 0; i < 12; i++)
            DOSFilename[i] = file.read8();
        file.skip(1);    // 00h
        newNoteAction = file.read8();
        duplicateCheckType = file.read8();
        duplicateCheckAction = file.read8();
        fadeout = file.read16();
        PPS = file.read8();
        PPC = file.read8();
        globalVolume = file.read8();
        defaultPan = file.read8();
        randomVolume = file.read8();
        randomPanning = file.read8();
        trackerVersion = file.read16();
        int numberOfSamples = file.read8();
        file.read8(); // unused
        name[26] = 0;
        for (int i = 0; i < 26; i++)
            name[i] = file.read8();

        initialFilterCutoff = file.read8();
        initialFilterResonance = file.read8();

        midiChannel = file.read8();
        midiProgram = file.read8();
        midiBank = file.read16();

        for (int i = 0; i < 120; i++) {
            notemap[i].note = file.read8();
            notemap[i].sample = file.read8();
        }

        volumeEnvelope.load(file);
        panningEnvelope.load(file);
        pitchEnvelope.load(file);
    }

    void Instrument::save(IO::File &file) {
        file.writeAscii("IMPI");
        file.writeAsciiF(DOSFilename, 12);
        file.write8(0);
        file.write8(newNoteAction);
        file.write8(duplicateCheckType);
        file.write8(duplicateCheckAction);
        file.write16(fadeout);
        file.write8(PPS);
        file.write8(PPC);
        file.write8(globalVolume);
        file.write8(defaultPan);
        file.write8(randomVolume);
        file.write8(randomPanning);
        file.write16(trackerVersion);
        file.write8(1); // NoS (ignored right?)
        file.write8(0); // reserved
        file.writeAsciiF(name, 26);
        file.write8(initialFilterCutoff);
        file.write8(initialFilterResonance);
        file.write8(midiChannel);
        file.write8(midiProgram);
        file.write16(midiBank);

        for (int i = 0; i < 120; i++) {
            file.write8(notemap[i].note);
            file.write8(notemap[i].sample);
        }

        volumeEnvelope.save(file);
        panningEnvelope.save(file);
        pitchEnvelope.save(file);

        file.write32(0); // waste 7 bytes (?)
        file.write16(0);
        file.write8(0);
    }

    void Instrument::setName(const char *text) {
        copyFstr(name, text, 27);
    }

    Envelope::Envelope() {
        enabled = false;
        loop = false;
        sustain = false;
        isFilter = false;

        length = 2;
        loopStart = 0;
        loopEnd = 0;
        sustainStart = 0;
        sustainEnd = 0;

        nodes[0].x = 0;
        nodes[1].x = 4;
        nodes[0].y = 32;
        nodes[1].y = 32;

        for (int i = 2; i < 25; i++) {
            nodes[i].x = 0;
            nodes[i].y = 0;
        }
    }

    void Envelope::load(IO::File &file) {
        u8 FLG = file.read8();
        enabled = !!(FLG & 1);
        loop = !!(FLG & 2);
        sustain = !!(FLG & 4);
        isFilter = !!(FLG & 128);

        length = file.read8();

        loopStart = file.read8();
        loopEnd = file.read8();

        sustainStart = file.read8();
        sustainEnd = file.read8();

        for (int i = 0; i < 25; i++) {
            nodes[i].y = file.read8();
            nodes[i].x = file.read16();
        }

        file.read8(); // reserved
    }

    void Envelope::save(IO::File &file) {
        file.write8(buildFlags());
        file.write8(length);
        file.write8(loopStart);
        file.write8(loopEnd);
        file.write8(sustainStart);
        file.write8(sustainEnd);
        for (int i = 0; i < 25; i++) {
            file.write8(nodes[i].y);
            file.write16(nodes[i].x);
        }

        file.write8(0);
    }

    u8 Envelope::buildFlags() {
        int flags = 0;
        flags |= enabled ? 1 : 0;
        flags |= loop ? 2 : 0;
        flags |= sustain ? 4 : 0;
        flags |= isFilter ? 128 : 0;
        return flags;
    }

    Sample::Sample() {
        for (int i = 0; i < 27; i++) name[i] = 0;
        for (int i = 0; i < 13; i++) DOSFilename[i] = 0;

        globalVolume = 64;
        defaultVolume = 64;
        defaultPanning = 32;

        vibratoSpeed = 0;
        vibratoDepth = 0;
        vibratoForm = 0;
        vibratoRate = 0;
    }

    void Sample::load(IO::File &file) {
        file.skip(4);    // IMPS
        DOSFilename[12] = 0;
        for (int i = 0; i < 12; i++)
            DOSFilename[i] = file.read8();
        file.skip(1);    // 00h
        globalVolume = file.read8();
        u8 flags = file.read8();

        bool hasSample = !!(flags & 1);
        data.bits16 = !!(flags & 2);
        data.stereo = !!(flags & 4);
        data.compressed = !!(flags & 8);
        data.loop = !!(flags & 16);
        data.sustain = !!(flags & 32);
        data.bidiLoop = !!(flags & 64);
        data.bidiSustain = !!(flags & 128);

        defaultVolume = file.read8();

        name[26] = 0;
        for (int i = 0; i < 26; i++)
            name[i] = file.read8();

        int convert = file.read8();
        defaultPanning = file.read8();

        data.length = file.read32();
        data.loopStart = file.read32();
        data.loopEnd = file.read32();
        centerFreq = file.read32();
        data.sustainStart = file.read32();
        data.sustainEnd = file.read32();

        u32 samplePointer = file.read32();

        vibratoSpeed = file.read8();
        vibratoDepth = file.read8();
        vibratoRate = file.read8();
        vibratoForm = file.read8();

        file.seek(samplePointer);
        data.load(file, convert);
    }

    void Sample::save(IO::File &file) {
        file.writeAscii("IMPS");
        file.writeAsciiF(DOSFilename, 12);
        file.write8(0);
        file.write8(globalVolume);
        file.write8(buildFlags());
        file.write8(defaultVolume);
        file.writeAsciiF(name, 26);
        file.write8(1); // signed sample
        file.write8(defaultPanning);
        file.write32(data.length);
        file.write32(data.loopStart);
        file.write32(data.loopEnd);
        file.write32(centerFreq);
        file.write32(data.sustainStart);
        file.write32(data.sustainEnd);
        file.write32(0); // reserve sample pointer
        file.write8(vibratoSpeed);
        file.write8(vibratoDepth);
        file.write8(vibratoRate);
        file.write8(vibratoForm);
    }

    u8 Sample::buildFlags() {
        int flags = 0;
        if (data.length) flags |= 1;
        if (data.bits16) flags |= 2;
        if (data.loop) flags |= 16;
        if (data.sustain) flags |= 32;
        if (data.bidiLoop) flags |= 64;
        if (data.bidiSustain) flags |= 128;
        return flags;
    }

    bool Sample::testNameMarker(const char *str) {
        int matches = 0;
        for (int i = 0; i < 27; i++) {
            if (name[i] == str[matches]) {
                matches++;
                if (str[matches] == 0) return true;
            } else {
                matches = 0;
            }
        }
        return false;
    }

    void Sample::setName(const char *text) {
        copyFstr(name, text, 27);
    }

    SampleData::SampleData() {
        bits16 = false;
        stereo = false;
        length = 0;
        loopStart = 0;
        loopEnd = 0;
        sustainStart = 0;
        sustainEnd = 0;
        loop = false;
        sustain = false;
        bidiLoop = false;
        bidiSustain = false;

        data8 = 0;
    }

    SampleData::~SampleData() {
        if (data8) {
            if (bits16)
                delete[] data16;
            else
                delete[] data8;
        }
    }

    void SampleData::load(IO::File &file, int convert) {
        if (!compressed) {

            // subtract offset for unsigned samples
            int offset = (convert & 1) ? 0 : (bits16 ? -32768 : -128);

            // signed samples
            if (bits16) {
                data16 = new s16[length];
                for (int i = 0; i < length; i++) {
                    data16[i] = file.read16() + offset;
                }
            } else {
                data8 = new s8[length];
                for (int i = 0; i < length; i++) {
                    data8[i] = file.read8() + offset;
                }
            }
        } else {
            // TODO : accept compressed samples.
        }
    }

    void SampleData::save(IO::File &file) {
        for (int i = 0; i < length; i++) {
            if (bits16) {
                file.write16(data16[i]);
            } else {
                file.write8(data8[i]);
            }
        }
    }

    Pattern::Pattern() {
        rows = 64;
        data = new u8[rows];
        for (int i = 0; i < rows; i++) {
            data[i] = 0;
        }
        dataLength = 64;
    }

    Pattern::~Pattern() {
        deleteData();
    }

    void Pattern::deleteData() {
        rows = 0;
        if (data)
            delete[] data;
        dataLength = 0;
    }

    void Pattern::load(IO::File &file) {
        deleteData();

        dataLength = file.read16();
        rows = file.read16();
        file.skip(4); // reserved
        data = new u8[dataLength];
        for (int i = 0; i < dataLength; i++) {
            data[i] = file.read8();
        }
    }

    void Pattern::save(IO::File &file) {
        file.write16(dataLength);
        file.write16(rows);
        file.write32(0); // reserved
        file.writeBytes(data, dataLength);
    }

//-----------------------------------------------------------------------------
};
//-----------------------------------------------------------------------------
