//
//  Author: Fridger Schrempp
//
//  fridger.schrempp@desy.de
//
// The program reads unsigned 8 bit integer, raw textures from STDIN
// and outputs to STDOUT a sample in the same raw format 
// of width reduced to the nearest power of 2.

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

#define USE_FASTNINT
#ifdef USE_FASTNINT
// fastnint() using magic numbers based on Sree Kotay's xs_RoundToInt()
// http://www.stereopsis.com/sree/fpu2006.html
const double _xs_doublemagic      = 6755399441055744.0;
    //2^52 * 1.5, uses limited precision to floor
const double _xs_doublemagicdelta = 1.5e-8;
    //almost .5f = .5f + 1e^(number of exp bit)
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

inline int fastnint( double x )
{
    double_or_int *u = (double_or_int *)&x;
    u->r64 = x + _xs_doublemagicdelta + _xs_doublemagic;
    return u->i32[_xs_iman_];
}
#define __NINT(x) fastnint(x)
#else
#define __NINT(x) (int)((x) < 0.0 ? ceil((x)-0.5) : floor((x)+0.5));
#endif


using namespace std;


const string version = "1.0, August 2007, author: F. Schrempp\n";

void readRowTx(FILE* in, unsigned char* row, int width)
{
    fread(row, 1, width, in);
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
    int headlines = 0;
    int RGB = 3;

    if (argc < 3||argc > 5)   
    {
        cerr << "\nUsage: tx2pow2 <channels> <width> [<height>  [<#headerlines>]]\n\n";
        cerr << "Version "<< version << endl;
        cerr << "---------------------------------------------------------------\n";
        cerr << "The program reads textures in unsigned 8 bit integer raw format\n";
        cerr << "from STDIN. It outputs to STDOUT a texture of width reduced to \n";    
        cerr << "the nearest power of 2 in the same format.\n"; 
        cerr << "---------------------------------------------------------------\n\n"; 
        cerr << "Assume :  Interleaved RGB ordering (RGBRGBRGBRGB...)\n"; 
        cerr << "          for RGB color textures.\n";
        cerr << "Defaults: Inputwidth : inputheight = 2 : 1.\n";
        cerr << "          No header (#headerlines = 0). \n\n";
        cerr << "For 4 x 8 bit RGB + alpha textures enter channels = 4.\n";
        cerr << "For 3 x 8 bit RGB colored textures enter channels = 3.\n";      
        cerr << "For 1 x 8 bit  grayscale  textures enter channels = 1.\n";
        cerr << "For different aspect ratios enter <height> in pixels.\n";
        cerr << "For PDS .img, .edr,...format enter #headerlines = 2,....\n\n"; 

        return 1;
    }
    else
    {
        if (SSCANF(argv[1], " %d", &RGB) != 1)
            {
                cerr << "Bad image type.\n";
                return 1;
            }    
        if (SSCANF(argv[2], " %d", &width0) != 1)
        {
            cerr << "Bad image dimensions.\n";
            return 1;
        }
        height0 = width0 / 2;        
        if (argc > 3)
        {
            if (SSCANF(argv[3], " %d", &height0) != 1)
            {
                cerr << "Bad image type.\n";
                return 1;
            }        
            if (argc > 4)
            {
                if (SSCANF(argv[4], " %d", &headlines) != 1)
                {
                    cerr << "Bad number of header lines.\n";
                    return 1;
                }   
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

    // find nearest power-of-two width < width0
    int width = 1;
    while ( width < width0)
        width <<= 1;
    width >>= 1;
	
	int height = width / (width0 / height0);
    cerr << "\n[tx2pow2]: Reducing to nearest power-of-two size:" << setw(6) << width <<" x" << setw(6) << height << "\n";

    int wRGB  = RGB * width;
    int w0RGB = RGB * width0;

    double *vertOut = new double[w0RGB];
    unsigned char *out = new unsigned char[wRGB];
    unsigned char *h[3];
    h[0] = new unsigned char[w0RGB];
    h[1] = new unsigned char[w0RGB];
    h[2] = new unsigned char[w0RGB];

    if (headlines)
    {
        cerr << "\nSkipping "<< headlines <<" header records"<<endl;
         
        for (int ih = 1; ih <= headlines; ih++)
            readRowTx(stdin, h[0], w0RGB);
    }   

    double mr = (double) width / (double) width0;
    int i, j, k;
    int j0, j00 = 0;
    int idx_j = 1; //index of last line read.

    readRowTx(stdin, h[0], w0RGB);
    readRowTx(stdin, h[1], w0RGB);

    // Precalculating i0, x00 x01 sx for each column
    // sx contains offsets between real coords and discrete pixels
    int *i0 = new int[width];
    double *x00 = new double[width];
    double *x01 = new double[width];
    double *sx  = new double[width];

    for (i = 0; i < width; ++i)
    {
        i0[i] = nint(width0 * (i + 0.5)/(double)width - 0.5);
        x00[i] = mr * i0[i];
        x01[i] = mr * (i0[i] + 1);
        double x0  = (double)i;
        double x1  = (double)(i + 1);
        sx[i] = min(x01[i],x1) - max(x00[i],x0);
    }

    for (j = 0; j < height; ++j)
    {
        j0 = nint(height0 * (j + 0.5) / (double)height - 0.5); // y-coord in src image
        for (k = j00 + 1; k <= j0; ++k)
        {
            if (k < height0 - 1)
                readRowTx(stdin, h[(++idx_j)%3], w0RGB);
        }
        j00 = j0;

        double y00 = mr *  j0;
        double y01 = mr * (j0 + 1);
        double y0  = (double) j;
        double y1  = (double)(j + 1);

        int idx_j0 = (idx_j%3 + (j0 - idx_j) + 3)%3;  // middle row index for h
        int idx_j0m1 = (idx_j0 + 2)%3;                // top row
        int idx_j0p1 = (idx_j0 + 1)%3;                // bottom row

        double s2 = (y00 - y0);                       // top middle
        double s5 = (min(y1,y01) - max(y00,y0));      // middle
        double s8 = (y1 - y01);                       // bottom middle
        bool isS2GreaterThan0 = (s2 > 0);
        bool shouldAddS8      = (s8 > 0) && (j0 < height0 - 1);
        int rgb;
        double result;

        // Do the reduction for each color
        for (rgb = 0; rgb < RGB; ++rgb)
        {
            // Vertical pass (just enough rows to cover filter kernel)
            for (i = 0; i < width0; ++i)
            {
                int ik  = RGB * i + rgb;

                result = s5 * h[idx_j0][ik];
                if (isS2GreaterThan0)
                    result += s2 * h[idx_j0m1][ik];
                if (shouldAddS8)
                    result += s8 * h[idx_j0p1][ik];

                vertOut[ik] = result;
            }

            // Horizontal pass (1 row, scale width0 to width)
            for (i = 0; i < width; ++i)
            {
                int ik0 = RGB * i0[i] + rgb;

                double s4 = x00[i] - (double) i;      // middle left
                double s6 = (double)(i + 1) - x01[i]; // middle right

                result = sx[i] * vertOut[ik0];
                if (s4 > 0)
                    result += s4 * vertOut[ik0 - RGB];
                if ((s6 > 0) && (ik0 < w0RGB - RGB))
                    result += s6 * vertOut[ik0 + RGB];

                out[RGB * i + rgb] = (unsigned char)nint(result);
            }
        }

        fwrite(out, 1, wRGB, stdout);
     	if ((j + 1) % 1024 == 0 ) 
        {
			end = clock();
			double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
			cerr << "pow2["<< setw(6) << j + 1 << " rows of " << setw(5) << height <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";
        }   
	}

    return 0;
}
