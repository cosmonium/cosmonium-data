//
//  Author: Fridger Schrempp
//
//  fridger.schrempp@desy.de
//
// The program reads unsigned 8 bit integer raw textures of even height  
// from STDIN. For colored textures, the storage mode is assumed to be  
// interleaved, ie. RGB(A)RGB(A)RGB(A) for RGB (+alpha) textures.
// The program outputs to STDOUT a texture of size reduced by 
// a factor of two in the same raw format.

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
#else  // _MSC_VER >= 1400
	#define SSCANF sscanf
#endif  // _MSC_VER >= 1400

#ifdef _LINUX
    #include <fpu_control.h>
#endif

#define USE_FASTFLOOR

#ifdef USE_FASTFLOOR
// fastfloor() using magic numbers based on Sree Kotay's xs_FloorToInt()
// http://www.stereopsis.com/sree/fpu2006.html
const double _xs_doublemagic         = 6755399441055744.0;
    //2^52 * 1.5, uses limited precision to floor
const double _xs_doublemagicdelta    = 1.5e-8;
    //almost .5f = .5f + 1e^(number of exp bit)
const double _xs_doublemagicroundeps = .5f-_xs_doublemagicdelta;
    //almost .5f = .5f - 1e^(number of exp bit)
#ifdef __BIG_ENDIAN__
   #define _xs_iman_                   1
#else
   #define _xs_iman_                   0   //intel is little endian
#endif
typedef union
{
    double r64;
    int  i32[2];
} double_or_int;

inline int fastfloor( double x )
{
    double_or_int *u = (double_or_int *)&x;
    u->r64 = x - _xs_doublemagicroundeps + _xs_doublemagic;
    return u->i32[_xs_iman_];
}

#define FLOOR(x)    fastfloor(x)
#else
#define FLOOR(x)    floor(x)
#endif

using namespace std;
static int bpp = 3;
static const double  TWOPI = 6.283185308, PI = 3.1415927;

const string version = "1.0, March 2010, author: F. Schrempp\n";

unsigned char* readRowTx(FILE* in, int width)
{
    unsigned char* row = new unsigned char[width];
    fread(row, 1, width, in);
    return row;
}

unsigned char* pinchCorr(FILE* in, int width, double theta)
{    
	unsigned char*  row = readRowTx(in, width);
	unsigned char* newRow = new unsigned char[width];
	double w = width/bpp;
	for(int i = 0; i < w; i++)	
    {
		for (int col = 0; col < bpp; col++)
        {
			newRow[bpp * i + col] = row[bpp * (int)(cos(theta) * (double)(i - w/2) + w / 2) + col];
		}
	}
	delete row;	        
    return (newRow);
}

int main(int argc, char* argv[])
{
    int width   = 0;
    int height  = 0;
    
    if (argc < 3 || argc > 4)     
    {
        cerr << "\nUsage: txnopinch <channels> <width> [<height>]\n\n";
        cerr << "Version "<< version << endl;
        cerr << "----------------------------------------------------------------\n";
        cerr << "The program reads textures of even size in unsigned 8 bit\n";
        cerr << "integer raw format from STDIN. It outputs to STDOUT a texture\n"; 
        cerr << "corrected for polar pinch effects in the same 8 bit raw format.\n"; 
        cerr << "----------------------------------------------------------------\n\n"; 
        cerr << "Assume  : Interleaved RGB(A) storage mode ie. RGB(A)RGB(A)RGB(A)..\n";
        cerr << "          for RGB (+ alpha) textures\n";
        cerr << "Defaults: Inputwidth : inputheight = 2 : 1.\n\n";
	    cerr << "For 4 x 8 bit RGB + alpha textures enter channels = 4.\n";
        cerr << "For 3 x 8 bit RGB colored textures enter channels = 3.\n";       
        cerr << "For 1 x 8 bit  grayscale  textures enter channels = 1.\n";
        cerr << "For different aspect ratio, enter <height> in pixels.\n\n";                  
        return 1;
    }
    else
    {
        if (SSCANF(argv[1], " %d", &bpp) != 1)
        {
            cerr << "Bad image type.\n";
            return 1;
        }
        if (SSCANF(argv[2], " %d", &width) != 1)
        {
            cerr << "Bad image dimensions.\n";
            return 1;
        }       
        height =  width / 2;
        if (argc > 3)
        {
            if (SSCANF(argv[3], " %d", &height) != 1)
            {
                cerr << "Bad image dimensions.\n";
                return 1;
            }
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

	cerr << "[pinch]: Texture size:" << setw(6) << width <<" x"<< setw(6) << height<<"\n";
	    
	width = bpp * width;
	  
    for (int j = 0; j < height; j++)
    {
		double theta = PI * ((double) j - 0.5 * (double)(height - 1)) / (double)(height - 1);
        fwrite(pinchCorr(stdin, width, theta), 1, width, stdout);
		
		if ((j + 1) % 1024 == 0 ) 
        {
         	end = clock();
			double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
			cerr << "pinch["<< setw(6) << j + 1 << " rows of " << setw(5) << height <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";
        }  			
               
    }
}
