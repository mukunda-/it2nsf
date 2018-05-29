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
#include <stdio.h>
#include "inputdata.h"
#include "toad.h"
#include "nsf.h"
#include "packify.h"

const char USAGE[] =
        "IT2NSF (C) 2009 mukunda, coda, madbrain, reduz\n"
        "Version 1.1h\n"
        "\n"
        "  IT->NSF Usage: \n"
        "    it2nsf [options] <modules>\n"
        "  Sample pack creation: \n"
        "    it2nsf -p [options] <module>\n"
        "  UN-CONVERSION: \n"
        "    it2nsf -u <index> [options] <nsf>\n"
        "\n"
        "Options\n"
        "-o [file]  Specify output file.\n"
        "--output\n   Default is formed from first input file.\n"
        "-q         Suppress normal output.\n"
        "--quiet\n"
        "--help     Show Help.\n"
        "-t         Specify title for NSF output.\n"
        "--title\n    (maximum 32 characters)\n"
        "-a         Specify author for NSF output.\n"
        "--author\n   (maximum 32 characters)\n"
        "-c         Specify copyright for NSF output.\n"
        "--copyright\n  (maximum 32 characters)\n"
        "-p         Enter sample-pack creation mode.\n"
        "--pack\n"
        "-u [index] Enter REVERSE-CONVERSION mode with song index.\n"
        "--unconv\n"
        "-m [max]   Generate no more than max multisamples per instrument (with -p).\n"
        "--multisamples\n"
        "\n"
        "Sample Pack Creation:\n"
        " source WILL LOSE ALL INSTRUMENTS AND 'GENERATED' SAMPLES,\n"
        " and will be filled with new instruments from the mml data.\n"
        "\n"
        "Typical usage:\n"
        "  it2nsf mygreatsong.it\n"
        "  ->\"mygreatsong.nsf\" will be created\n"
        "  it2nsf -p mygreatpack.it\n"
        "  ->\"mygreatpack.it\" will be filled with instruments!\n"
        "  it2nsf -p mygreatpack.it -o foo.it\n"
        "  ->you can change the output option too!\n"
        "  it2nsf -u 1 mygreatsongs.nsf\n"
        "  ->mygreatsongs.it will be made from the first song in mygreatsongs.nsf\n"
        "  it2nsf -u 4 mygreatsongs.nsf -o duck.it\n"
        "  ->duck.it will be created from song index 4 of mygreatsongs.nsf\n"
        "    of course you cannot do this with nsf's not made with IT2NSF.\n"
        "\n"
        "Multiple modules may be listed to create a multisong NSF.\n"
        "note: Full IT is not supported of course. Follow the rules.\n";

std::string PATH;
bool QUIET;

void normalConversion(ConversionInput::OperationData &od) {
    if (!QUIET)
        printf("Loading modules...\n");

    ToadLoader::Bank bank(od.files);

    if (!QUIET)
        printf("Starting conversion...\n");

    NSF::Bank result(bank, od);

    // export products
    result.exportNSF(od.output.c_str());

}

void packMaker(ConversionInput::OperationData &od) {
    Packify::run(od.files[0], od.output.c_str());
}

void reverseConversion(ConversionInput::OperationData &od) {
    printf(" oops this isnt implemented yet.\n");
}

int main(int argc, char *argv[]) {

//	ToadLoader::Module test;

//	test.load( "waow.it" );
//	test.save( "pood2.it" );

//	return 0;

    ConversionInput::OperationData od(argc, argv);

    QUIET = od.quiet;

    if (argc < 2) {
        od.help = true;
    }

    if (od.help) {
        printf(USAGE);
        return 0;
    }

    if (od.files.empty()) {
        printf("nothing to do.\n");
        return 0;
    }

    if (od.output.empty()) {
        printf("missing output file!?\n");
        return 0;
    }

    if ((!od.packmode) && (!od.reverse))
        normalConversion(od);
    else if (od.packmode)
        packMaker(od);
    else if (od.reverse)
        reverseConversion(od);

    return 0;
}
