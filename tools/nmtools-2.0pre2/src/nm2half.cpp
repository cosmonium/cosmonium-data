//
//  Authors: Fridger Schrempp & Robert Skuridin
//
//  fridger.schrempp@desy.de
//  skuridin1@mail.ru
//
// The program reads textures of even height in signed 16-bit integer raw 
// format from STDIN. It outputs to STDOUT a texture of size reduced by 
// a factor of two in the same 16 bit raw format.

// Specify byteswap = 1 if the byte ordering 
// of input file and computer differ

// The main application refers to raw binary 16bit elevation maps

#include <iostream>
#include <cstdio>
#include <math.h>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
	#include <io.h>
	#include <fcntl.h>
#endif 

#if _MSC_VER >= 1400  // MSVC 2005/8
	#define SSCANF sscanf_s
#else 
	#define SSCANF sscanf
#endif 

#ifdef _LINUX
    #include <fpu_control.h>
#endif

#define USE_FASTNINT

#ifdef  USE_FASTNINT
// fastnint() using magic numbers based on Sree Kotay's method
// http://www.stereopsis.com/sree/fpu2006.html

const double _xs_doublemagic      = 6755399441055744.0;
//2^52 * 1.5, uses limited precision to floor
const double _xs_doublemagicdelta = 1.5e-8;
//almost .5f = .5f + 1e^(number of exp bit)
const double _xs_doublemagicroundeps = .5f-_xs_doublemagicdelta;
//almost .5f = .5f - 1e^(number of exp bit)

#ifdef __BIG_ENDIAN__
   #define _xs_iman_                1
#else
   #define _xs_iman_                0   //intel is little endian
#endif

typedef union
{
    double r64;
    int  i32[2];
} double_or_int;

inline int fastcnint( double x )
{
    double_or_int *u = (double_or_int *)&x;
    u->r64 = x + _xs_doublemagic;
    return u->i32[_xs_iman_];
}

inline int fastceil( double x )
{
    return fastcnint(x+_xs_doublemagicroundeps);
}

inline int fastfloor( double x )
{
    return fastcnint(x-_xs_doublemagicroundeps);
}

inline int fastnint( double x )
{
    // There is a faster method that doesn't use a conditional
    // but negative floats of the form -x.5 (eg -18.5) rounded incorrectly
    // so use this workaround instead
    return (int)(x < 0.0 ? fastceil(x-0.5) : fastfloor(x+0.5));
}

#define __NINT(x) fastnint(x)
#else
#define __NINT(x) (int)((x) < 0.0 ? ceil((x)-0.5) : floor((x)+0.5));
#endif

using namespace std;

const string version = "2.0, November 2008, authors: F. Schrempp and R. Skuridin\n";

int byteSwap = 0;

int IsLittleEndian()
{
   short word = 0x4321;
   if((*(char *)& word) != 0x21 )
       return 0;
   else 
       return 1;
}

short readS16(FILE *in)
{
    short b2;
    
    fread(&b2, 2, 1, in);

    if (byteSwap == 1)
    	b2 = ((b2 & 0xff00) >> 8) | ((b2 & 0x00ff) << 8); 
    
    return (short) b2; 
}

int* rebinRowCol(FILE* in, int width)
{
    int* newRowCol = new int[width / 2];
    for (int i = 0; i < width / 2; i++)
        newRowCol[i] = readS16(in) + readS16(in);
    for (int i = 0; i < width / 2; i++)
        newRowCol[i] += readS16(in) + readS16(in);
    return newRowCol;
}

inline int nint(double x)
{
    return __NINT(x);
} 


int main(int argc, char* argv[])
{
    int width0   = 0;
    int height0  = 0;
	vector<string> pcOrder(2);
	
	pcOrder[0] = "Big-endian (MAC...)"; 
	pcOrder[1] = "Little-endian (Intel)";
	
	if (argc < 2 || argc > 3)     
    {
        cerr << "\nUsage: nm2half <inputwidth> [<byteswap>]\n\n";
        cerr << "Version "<< version << endl;
        cerr << "Assume : inputwidth : inputheight = 2 : 1\n\n";
        cerr << "Default: byte ordering of input file and computer are equal \n";
        cerr << "Else   : specify <byteswap> = 1\n";
        cerr << "-------------------------------------------------------\n";
        cerr << "The program reads textures of even height in signed 16-bit int\n";
        cerr << "raw format from STDIN. It outputs to STDOUT a texture of size\n"; 
        cerr << "reduced by a factor of two in the same 16 bit raw format.\n"; 
        cerr << "-------------------------------------------------------\n\n"; 
        cerr <<"Computer and output to STDOUT have +++ "<<pcOrder[IsLittleEndian()]<<" +++ byte order!\n\n";  
        return 1;
    }
    else
    {
    	if (SSCANF(argv[1], " %d", &width0) != 1)
    	{
    		cerr << "Bad image dimensions.\n";
        	return 1;
    	}    
    	height0 = width0 / 2;
    	if (argc == 3)
    	{
    		if (SSCANF(argv[2], " %d", &byteSwap) != 1)
    		{
    			cerr << "Bad byteorder specs.\n";
        		return 1;
    		};
    	}
    }		
        
   	#ifdef _WIN32
		if (_setmode(_fileno(stdin), _O_BINARY) == -1 )
    	{
    		cerr<<"Binary read mode from STDIN failed\n";
    		return 1;
    	}

		if (_setmode(_fileno(stdout), _O_BINARY) == -1 )
    	{
    		cerr<<"Binary write mode via STDOUT failed\n";
    		return 1;
    	}
   	#endif
	   
	#ifdef _LINUX
        // Adjust FPU precision mode on Linux for correct results
        fpu_control_t cw;
        _FPU_GETCW(cw);
        cw &= ~_FPU_EXTENDED;
        cw |= _FPU_DOUBLE;
        _FPU_SETCW(cw);
    #endif
    
	clock_t start = clock(), end; 
    int** h = new int* [height0 / 2];
    cerr << "[nm2half]: Input size:" << setw(6)<< width0 <<" x"<< setw(6)<<height0<<" => ";
    cerr << "New size:" << setw(6)<< width0 / 2 <<" x"<< setw(6)<<height0 / 2<< "\n";
    for (int j = 0; j < height0 / 2; j++)
    {
        if (j > 0)
        	delete[] h[j-1];
     	h[j]     =  rebinRowCol(stdin, width0); 
     	for (int i = 0; i < width0 / 2; i++)
     	{
         	short hs = (short) nint(0.25 * h[j][i]);
      		fwrite(&hs, 2, 1, stdout);
		}
		if ((j + 1) % 1024 == 0 ) 
        {
         	end = clock();
			double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
			cerr << "half["<< setw(6) << j + 1 << " rows of "<< setw(5)<< height0 / 2 <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";
        }  			
    }
}
