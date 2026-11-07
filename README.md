This program is a command line tool designed to read and display the metadata for .WAV files used in the collection of IQ data, such as is captured by SDR (software defined radios). As I'm not a real programmer, this is a VERY basic program. It can be compiled using GCC as follows:

gcc wav-reader.c -o wav-reader

Once compiled, it can be used with ./wav-reader <WAV file name>

On a Linux system, once compiled, it can be added to /usr/local/bin (typically part of the PATH) so that it can be called from any directory. 

NOTE: This program was written on and for Linux. I don't know if it will work on Windows unaltered.
