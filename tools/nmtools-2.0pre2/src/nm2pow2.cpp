//
//  Authors: Fridger Schrempp & Robert Skuridin
//
//  fridger.schrempp@desy.de
//  skuridin1@mail.ru
//
// The program reads samples with signed 16-bit integers from STDIN
// and outputs to STDOUT a signed 16-bit integer sample 
// of width reduced to the nearest power of 2.

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

void readRowS16(FILE* in, short* row, int width)
{
    fread(row, 2, width, in);

    if (byteSwap == 1)
        for (int i = 0; i < width; i++)
            row[i] = ((row[i] & 0xff00) >> 8) | ((row[i] & 0x00ff) << 8);
}

inline double min(double a, double b)
{
    return a < b ? a : b;
}

inline double max(double a, double b)
{
    return a > b ? a: b;
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
        cerr << "\nUsage: nm2pow2 <inputwidth> [<byteswap>]\n\n";
        cerr << "Version "<< version << endl;
        cerr << "Assume : inputwidth : inputheight = 2 : 1\n";
        cerr << "Default: byte ordering of input file and computer are equal \n";
        cerr << "Else   : specify <byteswap> = 1\n\n";
        cerr << "-------------------------------------------------------\n";
        cerr << "The program reads samples from STDIN in signed 16-bit integer\n";
        cerr << "raw format. It outputs to STDOUT a signed 16-bit sample of \n"; 
        cerr << "width reduced to the nearest power of 2. The main application \n";
        cerr << "refers to binary 16-bit elevation maps.\n";
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

    // find nearest power-of-two width < width0
    int width = 1;
    while ( width < width0)
        width <<= 1;
    width >>= 1; 
    int height = width / 2;
    cerr << "\n[nm2pow2]: Reducing to nearest power-of-two size:" << setw(6)<< width <<" x"<< setw(6)<< height << "\n";
	    
	clock_t start = clock(), end;  
    double *vertOut = new double[width0];
    short *out = new short[width];
    short *h[3];
    h[0] = new short[width0];
    h[1] = new short[width0];
    h[2] = new short[width0];

    double mr = (double) width / (double) width0;
    int i, j, k;
    int j0, j00 = 0;
    int idx_j = 1; //index of last line read.

    readRowS16(stdin, h[0], width0);
    readRowS16(stdin, h[1], width0);

    // Precalculating i0, x00 x01 sx for each column
    // sx contains offsets between real coords and discrete pixels
    int *i0 = new int[width];
    double *x00 = new double[width];
    double *x01 = new double[width];
    double *sx = new double[width];

    for (i = 0; i < width; ++i)
    {
        i0[i] = nint(width0 * (i + 0.5)/(double)width - 0.5);
        x00[i] = mr * i0[i];
        x01[i] = mr * (i0[i] + 1);
        double x0  = (double)i;
        double x1  = (double)(i + 1);
        sx[i] = min(x01[i],x1) - max(x00[i],x0);
    }

    for (j = 0; j < height; j++)
    {
		j0 = nint(height0 * (j + 0.5) / (double)height - 0.5); 
		// y-coord in src image
        for (k = j00 + 1; k <= j0; ++k)
        {
            if (k < height0 - 1)
                readRowS16(stdin, h[(++idx_j)%3], width0);
        }
        j00 = j0;

        double y00 = mr * j0;
        double y01 = mr * (j0 + 1);
        double y0  = (double)j;
        double y1  = (double)(j + 1);

        int idx_j0 = (idx_j%3 + (j0 - idx_j) + 3)%3;  // middle row index for h
        int idx_j0m1 = (idx_j0 + 2)%3;                // top row
        int idx_j0p1 = (idx_j0 + 1)%3;                // bottom row

        double s2 = (y00 - y0);                       // top middle
        double s5 = (min(y1,y01) - max(y00,y0));      // middle
        double s8 = (y1 - y01);                       // bottom middle
        bool isS2GreaterThan0 = (s2 > 0);
        bool shouldAddS8      = (s8 > 0) && (j0 < height0 - 1);
        double result;

        // Vertical pass (just enough rows to cover filter kernel)
        for (i = 0; i < width0; ++i)
        {
            result = s5 * h[idx_j0][i];
            if (isS2GreaterThan0)
                result += s2 * h[idx_j0m1][i];
            if (shouldAddS8)
                result += s8 * h[idx_j0p1][i];

            vertOut[i] = result;
        }

        // Horizontal pass (1 row, scale width0 to width)
        for (i = 0; i < width; ++i)
        {
            double s4 = x00[i] - (double) i;      // middle left
            double s6 = (double)(i + 1) - x01[i]; // middle right

            result = sx[i] * vertOut[i0[i]];
            if (s4 > 0)
                result += s4 * vertOut[i0[i] - 1];
            if ((s6 > 0) && (i0[i] < width0 - 1))
                result += s6 * vertOut[i0[i] + 1];

            out[i] = (short)nint(result);
        }

        fwrite(out, 2, width, stdout);
	
     	 if ((j + 1) % 1024 == 0 ) 
         {
         	end = clock();
			double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
			cerr << "pow2[" << setw(6) << j + 1 << " rows of " << setw(5) << height << " -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";
         }   
    }
    return 0;
}

