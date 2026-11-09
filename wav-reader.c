/* This program reads in the metadata for different
   types of .WAV files and parses out the metadata. 
   WAVE files are divided into chunks. The general order for a basic
   audio WAVE file is:
   * RIFF
   * fmt
   * data
   But IQ files could be:
   RIFF         RIFF        RIFF        RF64        RF64
   fmt          JUNK        fmt         ds64        ds64
   auxi         fmt         data        fmt         fmt
   data         data                    auxi        data
                                        data
   This program will read through the WAVE file, identify the present
   chunks, and provide the standard information for each.
   NOTE: As the possible permutations for WAVE files is quite high, this
   program only looks at those created for dealing with basic audio files
   and IQ files.
   
   Written by Gary Schafer, 2026
   
   No AI was used in the creation of this program. It was written using
   (and I'm not making this up) a simple text editor, an old copy of 
   "Programming in C", some web searches (primarily Stack Overflow posts),
   and a lot of trial-and-error-and-more-errors.
   
   Published under the Gnu General Public License, Version 3.
   
   You can copy, edit, mutilate, berate, say snarky things at, or otherwise
   go crazy with this code. However, give me SOME credit, will ya?*/
   
#include <stdio.h>
#include <string.h>

#define RIFF        1179011410  // This value is an unsigned 32-bit integer expression for "RIFF".
#define RF64        875972178   // This value is an unsigned 32-bit integer expression for "RF64".
#define MAX_SIZE    4294967295  // This is 0xffffffff used for RF64 for the initial file size.
#define WAVE        1163280727  // This value is an unsigned 32-bit integer expression for "WAVE".
#define FMT         544501094   // This value is an unsigned 32-bit integer expression for "FMT ".
#define JUNK        1263424842  // This value is an unsigned 32-bit integer expression for "JUNK".
#define DS64        875983716   // This value is an unsigned 32-bit integer expression for "ds64".
#define AUXI        1769502049  // This value is an unsigned 32-bit integer expression for "auxi".
#define DATA        1635017060  // This value is an unsigned 32-bit integer expression for "data".
#define BIG1        828860770   // This value is an unsigned 32-bit integer expression for "big1".
#define CUE         543520099   // This value is an unsigned 32-bit integer expression for "cue ".
#define LIST        1953720684  // This value is an unsigned 32-bit integer expression for "list".
#define LABL        1818386796  // This value is an unsigned 32-bit integer expression for "labl".
#define R64M        1832138354  // This value is an unsigned 32-bit integer expression for "r64m".
#define XML         4128828     // Expression for "<?xm", the beginning of an XML section in certain
                                // "auxi" chunks.

// Declare all of the functions
    
unsigned int name2num(char chunkName[4]);
void read_ds64(FILE *fcnp);
void read_junk(FILE *fcnp);
void read_auxi(FILE *fcnp);
void read_format(FILE *fcnp);
int find_string(char *fullString, char *searchString);
short string_present(char *fullString, char *searchString);
int array_length(char *myChar);
int display_field(char *fullString, int beginVal);
void parse_auxi_binary(unsigned int chunkSize, FILE *fcnp);
void parse_auxi_xml(unsigned int chunkSize, char *xmlChar, FILE *fcnp);

// Functions

unsigned int name2num(char chunkName[4])
{
    // This function takes the characters of the chunk name
    // and converts it into an unsigned integer number.
    // This is easier to use for matching chunk types.
    // Convert the chunk name into an unsigned 32-bit little-endian integer.
    // This is done by turning each character (8-bit) into a 32-bit integer,
    // then shifting the values for each character (in reverse order).
    // NOTE: It might be easier just to back up the file counter 4 bytes
    // and read in the number directly. Maybe later... this works for now.
    
    unsigned int result = 0;
    result=(int)(chunkName[3]<<24);
    result=result+(int)(chunkName[2]<<16);
    result=result+(int)(chunkName[1]<<8);
    result=result+(int)(chunkName[0]);
    
    return result;
}

void read_format(FILE *fcnp)
{

    // This function will read the "format" chunk.
    // This assumes that the file pointer has already
    // read in the chunk id (fmt ), and the this function
    // will start by reading in the chunk size.

    int numBytes = 0;
    unsigned int chunkSize, sampRate, bytesPerSec = 0;
    unsigned short sampleFormat, numChan, bytePerBloc, bitsPerSample = 0;    
    
    // Standard size for the "format" chunk is 24 bytes (including ID and size).
    // The value for size should be 16 bytes for standard. If it's not 16 bytes,
    // then tag this as a non-standard format chunk.
    // The standard chunk is as follows:
    // chunkID - NOTE: Will already be read-in in main.
    // chunkSize: 4 bytes
    // sampleFormat: 2 bytes (1 = PCM, 3 = IEEE 754 floating point)
    // numChan: 2 bytes (typically 2 for IQ)
    // sampRate: 4 bytes
    // bytesPerSec: 4 bytes
    // bytePerBloc: 2 bytes
    // bitsPerSample: 2 bytes
    
    // Read in the chunksize
    numBytes = fread(&chunkSize,sizeof(int),1,fcnp);
    printf("Chunk size: %u bytes\n",(chunkSize+8));
    if(chunkSize!=16)
    {
        printf("This is a non-standard \"format\" chunk.\n");
        printf("The values provided may not be correct.\n");
    }
    
    // Read in the sample format (typically PCM or straight integer).
    numBytes = fread(&sampleFormat,sizeof(short),1,fcnp);
    chunkSize = chunkSize - 2;
    
    if(sampleFormat==1)
    {
        printf("The samples are in PCM format (Value = 1).\n");
    }
    if(sampleFormat==3)
    {
        printf("The samples are in IEEE 754 (floating point) format (Value = 3).\n");
    }
    
    // Number of channels. Should be 2 for a stereo or IQ file.
    numBytes = fread(&numChan,sizeof(short),1,fcnp);
    chunkSize = chunkSize - 2;
    
    if(numChan==2)
    {
        printf("This is a standard stereo or IQ file; it has 2 channels.\n");
    }
    else
    {
        printf("This is not a stereo or IQ file. It has %hu channel(s).\n",numChan);
    }
    
    // Read in the sample rate
    numBytes = fread(&sampRate,sizeof(int),1,fcnp);
    chunkSize = chunkSize - 4;
    
    printf("The sample rate is %u Sps.\n",sampRate);
    
    // Read in bytes / second.    
    numBytes = fread(&bytesPerSec,sizeof(int),1,fcnp);
    chunkSize = chunkSize - 4;
    
    printf("Bytes / second: %u.\n",bytesPerSec);
    
    // Read in bytes / sample.
    numBytes = fread(&bytePerBloc,sizeof(short),1,fcnp);
    chunkSize = chunkSize - 2;
    
    printf("Bytes / sample: %hu.\n",bytePerBloc);
    
     // Read in bits / sample.
    numBytes = fread(&bitsPerSample,sizeof(short),1,fcnp);
    chunkSize = chunkSize - 2;
    
    printf("Bits / sample: %hu.\n",bitsPerSample);
    
    if((bitsPerSample/8*numChan)!=bytePerBloc)
    {
        printf("ERROR: The bytes/sample, bits/sample and number of channels do not align.\n");
    }
    
    if(chunkSize>0)
    {
        printf("Not all data was read in.\n");
        fseek(fcnp,chunkSize,SEEK_CUR);
    }
}

void read_auxi(FILE *fcnp)
{
    // There are currently two types of auxiliary (auxi) data
    // chunks in WAVE files. The difference is that one type is
    // a binary file and is 172 bytes in size, while the other
    // is variable size, and is a XML text file.
    
    // The first thing to do is to test whether this is the XML
    // text file.
        
    unsigned int chunkSize;
        
    // Read in the chunksize
    int numBytes = fread(&chunkSize,sizeof(int),1,fcnp);
    printf("Chunk size: %u bytes\n",(chunkSize+8));
    printf("Chunk data size: %u bytes\n",chunkSize);
    
    unsigned int tempVal = 0, halfCount = (chunkSize/2)-1;
    int n = 0;
    char tempChar[chunkSize]={}, auxiChar[chunkSize] = {};
    
    // Get the current position in case it is not a XML.
    fpos_t fpos;
    fgetpos(fcnp, &fpos);
        
    // Read in the entire chunk as characters
    numBytes=fread(&auxiChar,sizeof(char),chunkSize,fcnp);
    
    // Check to see if every other character is a NULL character.
    // If so, remove the NULL characters.
    
    for(n = 1; n<chunkSize; n = n + 2)
    {
        tempVal = tempVal + auxiChar[n];
    }
    printf("Removing NULL characters, if any.\n");            
    if(tempVal == 0) // If the total of every other character is 0,
                     // create a new array with only the actual ASCII
                     // values. This removes the NULL characters.
    {
        
        for(n = 0; n<halfCount; n = n + 1)
        {
            tempChar[n] = auxiChar[n*2];
        }
        printf("Found and removed NULL characters.\n");
        
    }
    else
    {
        strcpy(tempChar,auxiChar);
    }
    
    // Check for the presence of the XML and SDR XML tags.
    short xml_check = string_present(tempChar,"<?xml version");
    short root_check = string_present(tempChar,"<SDR-XML-Root");
    
    if((xml_check==1)&(root_check==1))
    {
        // Found both the XML and SDR XML Root tags, so this is XML text.
        printf("Found XML text. Parsing...\n");
        parse_auxi_xml(chunkSize,tempChar, fcnp);
    }
    else
    {
        // Didn't find XML, so assume it is binary.
        printf("Not XML. Assuming binary and parsing...\n");
        // Reset the file pointer to re-read the data since it is binary.
        fsetpos(fcnp, &fpos);
        parse_auxi_binary(chunkSize,fcnp);
    }
        
}

void parse_auxi_binary(unsigned int chunkSize, FILE *fcnp)
{    
    
        unsigned int centerFreq, sampleRate, ifFreq, bandwidth, iqOffset, dBoffset, maxVal, numBytes;
        unsigned short startYear, startMonth, startDow, startDay, startHour, startMin, startSec,startMsec;
        unsigned short stopYear, stopMonth, stopDow, stopDay, stopHour, stopMin, stopSec, stopMsec;
        char tempShort[4] = { 0 };
    
        // This function will read through the "auxi" chunk
        // created for IQ data by SDRplay for its SDRuno program.
        // This function assumes that the chunk ID ("auxi") has
        // already been read in. This function will start with
        // the chunk size.
        // The format is as follows (including the chunk ID):
        // chunk ID: 4 bytes (char, but read in as a uint32)
        // chunk size: 4 bytes
        // start time year: 2 bytes
        // start time month: 2 bytes
        // start time day of week: 2 bytes
        // start time day: 2 bytes
        // start time hour: 2 bytes
        // start time minute: 2 bytes
        // start time seconds: 2 bytes
        // start time milliseconds: 2 bytes
        // stop time year: 2 bytes
        // stop time month: 2 bytes
        // stop time day of week: 2 bytes
        // stop time day: 2 bytes
        // stop time hour: 2 bytes
        // stop time minute: 2 bytes
        // stop time seconds: 2 bytes
        // stop time milliseconds: 2 bytes
        // center frequency: 4 bytes
        // sample rate: 4 bytes
        // IF frequency: 4 bytes
        // bandwidth: 4 bytes
        // IQ offset: 4 bytes
        // dB offset: 4 bytes
        // maximum value: 4 bytes
        
        // Read in start date and time
        numBytes = fread(&startYear,sizeof(short),1,fcnp);
        numBytes = fread(&startMonth,sizeof(short),1,fcnp);
        numBytes = fread(&startDow,sizeof(short),1,fcnp);
        numBytes = fread(&startDay,sizeof(short),1,fcnp);
        numBytes = fread(&startHour,sizeof(short),1,fcnp);
        numBytes = fread(&startMin,sizeof(short),1,fcnp);
        numBytes = fread(&startSec,sizeof(short),1,fcnp);
        numBytes = fread(&startMsec,sizeof(short),1,fcnp);
        printf("Start date (YYYY/MM/DD): %.4hu/%.2hu/%.2hu\n",startYear,startMonth,startDay);
        printf("Start time (HH-MM-SS.MSEC): %.2hu-%.2hu-%.2hu.%.4hu\n",startHour,startMin,startSec,startMsec);
        chunkSize = chunkSize - 16;
    
        // Read in stop date and time
        numBytes = fread(&stopYear,sizeof(short),1,fcnp);
        numBytes = fread(&stopMonth,sizeof(short),1,fcnp);
        numBytes = fread(&stopDow,sizeof(short),1,fcnp);
        numBytes = fread(&stopDay,sizeof(short),1,fcnp);
        numBytes = fread(&stopHour,sizeof(short),1,fcnp);
        numBytes = fread(&stopMin,sizeof(short),1,fcnp);
        numBytes = fread(&stopSec,sizeof(short),1,fcnp);
        numBytes = fread(&stopMsec,sizeof(short),1,fcnp);
        printf("Stop date (YYYY/MM/DD): %.4hu/%.2hu/%.2hu\n",stopYear,stopMonth,stopDay);
        printf("Stop time (HH-MM-SS.MSEC): %.2hu-%.2hu-%.2hu.%.4hu\n",stopHour,stopMin,stopSec,stopMsec);
        chunkSize = chunkSize - 16;
   
        // Read in center frequency & sample rate
        numBytes = fread(&centerFreq,sizeof(int),1,fcnp);
        numBytes = fread(&sampleRate,sizeof(int),1,fcnp);
        printf("Center frequency: %f MHz\n",(float)centerFreq/1e6);
        chunkSize = chunkSize - 8;
    
        // Report on ADC frequency
        if(sampleRate==0)
        {
            printf("No reported ADC frequency. (Value = 0)\n");
        }
        else
        {
            printf("ADC Frequency: %f kHz\n",(float)sampleRate/1000);
        }
    
        numBytes = fread(&ifFreq,sizeof(int),1,fcnp);
        numBytes = fread(&bandwidth,sizeof(int),1,fcnp);
        chunkSize = chunkSize - 8;
    
        // Report on IF frequency and bandwidth
        if(ifFreq == 0)
        {
            printf("No reported IF frequency. (Value = 0)\n");
        }
        else
        {
            printf("IF Frequency: %f MHz\n",(float)ifFreq/1e6);
        }
    
        if(bandwidth == 0)
        {
            printf("No reported bandwidth. (Value = 0)\n");
        }
        else
        {
            printf("Bandwidth: %f kHz\n",(float)bandwidth/1000);
        }
    
        numBytes = fread(&iqOffset,sizeof(int),1,fcnp);
        numBytes = fread(&tempShort,sizeof(char),4,fcnp);
        numBytes = fread(&dBoffset,sizeof(int),1,fcnp);
        numBytes = fread(&maxVal,sizeof(int),1,fcnp);
        chunkSize = chunkSize - 16;
    
        // Report IQ offset
        if(iqOffset==0)
        {
            printf("No reported IQ offset. (Value = 0)\n");
        }
        else
        {
            printf("IQ offset: %f\n",(float)iqOffset/100.0);
        }
    
        printf("Temporary numbers are: %i, %i, %i & %i\n",tempShort[0], tempShort[1], tempShort[2], tempShort[3]);
    
        if(dBoffset==0)
        {
            printf("No reported dB offset. (Value = 0)\n");
        }
        else
        {
            printf("dB offset: %f\n",(float)dBoffset/100.0);
        }
           
        // Report the maximum value
        printf("Maximum value: %i\n",maxVal);

    //Advance the file pointer to account for the unused bytes.
    fseek(fcnp,chunkSize,SEEK_CUR);
}
    
void parse_auxi_xml(unsigned int chunkSize, char *xmlChar, FILE *fcnp)   
{    
    // An auxiliary chunk with XML text is known to be
    // based on the SDRconsole (by SDR-radio.com). If so,
    // it will contain an XML file consisting of ASCII
    // character / NULL character pairs (one ASCII character /
    // one NULL character). To check for this, read in the
    // entire chunk, then check if every other character
    // is the NULL character. If so, it's most likely an
    // XML file.
    
    // This will simply print out the XML text. A later version
    // of this program will actually break down the XML tags.
    
    unsigned int i = 0;
    int quoteFlag = 0; // Use this to toggle carriage returns for XML quotes.
    for(i = 0; i<chunkSize; i = i + 1)
    {
        printf("%c",xmlChar[i]);
        if(xmlChar[i]==34)
        {
            quoteFlag=quoteFlag + 1;
        }
        if(((xmlChar[i]==32)&((quoteFlag%2)==0))+(xmlChar[i]==62))
        {
            printf("\n");
        }
    }
   
}

// This function simply determines if a 
// certain string is present within another
// larger string.

short string_present(char *fullString, char *searchString)
{

    // Find the second string in the first
    int steps, i = 0, n = 0, length_1, length_2, match_val;
    length_1 = array_length(fullString);
    length_2 = array_length(searchString);
    
    if(length_2 > length_1)
    {
        return 0;
    }
    
    // Step through each of the characters to match
    // the first character of the "to find" string
    // with the other string.
    
    steps = length_1 - length_2;
    
    while(i <= steps)
    {
        
        // Check if the first character of the search string matches
        // the current character.
        
        if(fullString[i]==searchString[0])
        {
            
            match_val = 0;
            
            // If the first character matches, step through all of the
            // characters of the search string to see if they match.
            // To do this, add up matches. If it equals the length of
            // the search string, the search string has been found.
            for(n = 0; n<length_2; n = n + 1)
            {
                match_val = match_val + (fullString[i+n]==searchString[n]);
            }
            
            if(match_val == length_2)
            {
                // Found the string
                return 1;
            }
            
        }
        
        i = i + 1;
    }
    
    // Did not find the string.
    return 0;

}

// Calculate the length of an array.
// This function assumes that the end
// of the string array is "\0".

int array_length(char *myChar)
{

    int i = 0;
    
    while(myChar[i]>0)
    {
        i = i + 1;
    }
    
    return i;

}

int find_string(char *fullString, char *searchString)
{

    // Find the second string in the first
    int steps, i = 0, n = 0, length_1, length_2, match_val;
    length_1 = array_length(fullString);
    length_2 = array_length(searchString);
    
    if(length_2 > length_1)
    {
        return 0;
    }
    
    // Step through each of the characters to match
    // the first character of the "to find" string
    // with the other string.
    
    steps = length_1 - length_2;
    
    while(i <= steps)
    {
        
        // Check if the first character of the search string matches
        // the current character.
        
        if(fullString[i]==searchString[0])
        {
            
            match_val = 0;
            
            // If the first character matches, step through all of the
            // characters of the search string to see if they match.
            // To do this, add up matches. If it equals the length of
            // the search string, the search string has been found.
            for(n = 0; n<length_2; n = n + 1)
            {
                match_val = match_val + (fullString[i+n]==searchString[n]);
            }
            
            if(match_val == length_2)
            {
                return i;
            }
            
        }
        
        i = i + 1;
    }
    
    // Did not find the string
    return 0;
}

// Pull out the value for a field, typically a string
// within the larger string of a XML file for an "auxi"
// chunk. This works by reading in the full XML string
// plus the beginning value of the field, then locates
// the beginning and ending " character, and displays
// everything in between.

int display_field(char *fullString, int n)
{

    int charLength = array_length(fullString), i;
        
    while(fullString[n]>0)
    {
    
        i = 0;
    
        if(fullString[n] == 34) // Find the first double quotes
        {
            n = n + 1;
            while(fullString[n+i] != 34)
            {
                printf("%c",fullString[n+i]);
                i = i + 1;
            }
        
            printf("\n");
            return (n+i);
        
        }
        
        n = n + 1;
    
    }
        
}

void read_junk(FILE *fcnp)
{
    // This will deal with a "JUNK" chunk, which is used as a placeholder
    // for possible RF64 files if the file size goes above 4 GB.
    
    unsigned int numBytes, chunkSize;
    
    numBytes = fread(&chunkSize,sizeof(int),1,fcnp);
    printf("Chunk size: %u bytes\n",(chunkSize+8));
    
    fseek(fcnp,chunkSize,SEEK_CUR);
}

void read_ds64(FILE *fcnp)
{

    // Read in 64-bit values for files greater than 4 GB
    
    unsigned int numBytes, chunkSize, tableLength = 0;
    unsigned long int riffSize, dataSize, sampleCount = 0;
    
    numBytes = fread(&chunkSize,sizeof(int),1,fcnp);
    printf("Chunk size: %u bytes\n",(chunkSize+8));
    
    numBytes = fread(&riffSize,sizeof(long int),1,fcnp);
    numBytes = fread(&dataSize,sizeof(long int),1,fcnp);
    numBytes = fread(&sampleCount,sizeof(long int),1,fcnp);
    numBytes = fread(&tableLength,sizeof(int),1,fcnp);
    
    printf("RIFF size: %li bytes.\n",riffSize);
    printf("Data size: %li bytes.\n",dataSize);
    printf("Sample count: %li samples.\n",sampleCount);
    printf("Table length: %i.\n",tableLength);
    
}

/*******************************************************************/

int main(int argc, char **argv)
{

    // Setup the various variables... or variable various?
    // Sorry. Bad joke. Ignore it. But not the variables. Those are not a joke.
    
    unsigned int fileType, fileSize, waveType, chunkType, fmtBlocSize, sampRate, bytesPerSec = 0;
    unsigned int chunkSize = 0;
    unsigned short sampleFormat, nbrChan, bytePerBloc, bitsPerSample = 0;
    long int riffSizeLong, dataSizeLong, sampleCountLong = 0;
    char chunkName[4] = { 0 };
    fpos_t position;
    
    // Read in the filename
    
    if(argc==1) // Check that a filename has been provided
    {
        printf("ERROR: No filename provided.\n");
        return 4;
    }
    
    printf("WAVE Reader, version 1.0.1\n");
    printf("********************************\n");    
    printf("Filename: %s\n", argv[1]);
    
    FILE *fp=fopen(argv[1],"rb");
    if (fp==NULL)
    {
        printf("Failed to open file.\n");
        return 1;
    }   
    
    // fread() returns the number of items read or 0
    // The first variable within the fread function is a pointer. To ensure it is stored properly,
    // it should be sent to the address of the variable.
    int result = fread(&fileType,sizeof(int),1,fp);
    
    // Determine if this is a RIFF/RF64 file.
    
    if((fileType==RIFF)+(fileType==RF64))
    {
        printf("RIFF/RF64 file type: ");
        
        if(fileType==RIFF)
        {
            printf("RIFF.\n");
        }
        else
        {
            printf("RF64.\n");
        }
    }
    
    else // If this is not a RIFF/RF64 file, return with an error.
    
    {
        printf("This is not a RIFF file.\n");
        fclose(fp);
        return 2;
    }
    
    
    
    // Since it is a RIFF/RF64 file, read in the file size (RIFF) or 
    // 0xffffffff value (RF64).
    
    result = fread(&fileSize,sizeof(int),1,fp);
    
    if(fileType==RIFF)
    {
        printf("File size: %u bytes.\n",fileSize+8);
    }
    
    if((fileSize==MAX_SIZE)*(fileType==RF64))
    {
        printf("The initial file size conforms to RF64 standard.\n");
    }
    
    // Check that it is actually a WAVE file, and not another RIFF type such as
    // WEBP or AVI.
    
    result = fread(&waveType,sizeof(int),1,fp);
    
    if(waveType!=WAVE)
    {
        printf("This is not a WAVE file.\n");
        fclose(fp);
        return 3;
    }
    else
    {
        printf("This is tagged as a WAVE file.\n");
    }
    
    printf("*********************************\n");
    
    // The next value should be the beginning of the chunk following the master chunk.
    // This can be "ds64", "JUNK", "fmt " or "auxi".

    int keepGoing = 1; // This is a flag to tell the reading of the file to continue
                       // until this is set to 0.
    
    while(keepGoing)
    {                   
        result = fread(&chunkType,sizeof(int),1,fp);
        if(feof(fp))
        {
            keepGoing = 0;
            printf("Reached the end of the file. Did not find data chunk.\n");
            return 0;
        }
        if(chunkType==FMT)
        {
            printf("Chunk type: Format (fmt ).\n");
            read_format(fp);
            printf("********************************\n");       
        }
        
        if(chunkType==DATA)
        {
            keepGoing=0; // The data chunk should be the last chunk, so stop reading the file.
            printf("Chunk type: Data (data).\n");
            if(fileType==RIFF)
            {
                result=fread(&chunkSize,sizeof(int),1,fp);
                printf("Chunk size: %u bytes.\n",chunkSize);
            }
            else
            {
                printf("Chunk size shown in DS64 section.\n");
            }
        }
        
        if(chunkType==AUXI)
        {
            printf("Chunk type: Auxiliary (auxi).\n");
            read_auxi(fp);
            printf("********************************\n");
        }
        
        if(chunkType==JUNK)
        {
            printf("Chunk type: Junk (JUNK).\n");
            read_junk(fp);
            printf("********************************\n");
        }
        
        if(chunkType==DS64)
        {
            printf("Chunk type: DS64 (ds64).\n");
            read_ds64(fp);
            printf("********************************\n");
        }
        
        if(chunkType==BIG1)
        {
            printf("Chunk type: Big One (big1).\n");
            printf("********************************\n");
        }
        
        if(chunkType==LABL)
        {
            printf("Chunk type: Label (labl).\n");
            printf("********************************\n");
        }
        
        if(chunkType==CUE)
        {
            printf("Chunk type: Cue (cue ).\n");
            printf("********************************\n");
        }
        
        if(chunkType==LIST)
        {
            printf("Chunk type: List (list).\n");
            printf("********************************\n");
        }
        
        if(chunkType==R64M)
        {
            printf("Chunk type: R64M (r64m).\n");
            printf("********************************\n");
        }
        
    }
        
    fclose(fp);
    return 0;
}
