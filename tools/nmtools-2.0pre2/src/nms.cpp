//
//  Authors:  Chris Laurel, Fridger Schrempp & Robert Skuridin
//
//  claurel@gmail.com
//  fridger.schrempp@desy.de
//  skuridin1@mail.ru
//
// The program reads an elevation map in signed 16-bit raw format 
// from STDIN. It outputs to STDOUT a normalmap (PNG format), 
// corrected for spherical geometry. 


#include <iostream>
#include <cstdio>
#include <math.h>
#include <string>
#include <vector>
#include <ctime>
#include <png.h>
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

using namespace std;

const string version = "2.0, November 2008, authors: C. Laurel, F. Schrempp and R. Skuridin\n";

int byteSwap = 0;
int kernelSize = 2;
int halfKernelSize = 1;

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

short* readRowS16(FILE* in, int width)
{
    short* row = new short[width];
    for (int i = 0; i < width; i++)
        row[i] = readS16(in);

    return row;
}

int main(int argc, char* argv[])
{
    const double pi = 3.1415926535897932385;
    int width  = 0;
    int height = 0;
    double radius        = 0.0;
    double exag 	     = 1.0;
    double heightmapunit = 1.0;
 	vector<string> pcOrder(2);
	
	pcOrder[0] = "Big-endian (PPC-MAC,...)"; 
	pcOrder[1] = "Little-endian (Intel-PC, Intel-MAC,...)";
	           
    if (argc < 3 || argc > 6)
    {
        cerr << "\nUsage: nms  <bodyradius> <width> [<exag> [<byteswap> [ <heightmapunit>]]]\n\n";
        cerr << "Version "<< version << endl;
        cerr << "-------------------------------------------------------\n";
        cerr << "The program reads an elevation map in signed 16-bit integer \n"; 
        cerr << "raw format from STDIN. It outputs to STDOUT a normalmap (PNG),\n"; 
        cerr << "corrected for spherical geometry\n\n";
        cerr << "Units    : bodyradius[km], width[pixel]\n";
        cerr << "Assume   : aspect ratio = width : height = 2 : 1\n\n";
        cerr << "Defaults : exag = 1.0 (exaggeration factor, recommended: exag ~ 2.5)\n";
        cerr << "           byteswap = 0 <=> byte ordering of input file and computer are equal\n";
        
        cerr << "           heightmapunit = 1.0 (length of one heightmap unit in meters)\n\n";
        cerr << "-------------------------------------------------------\n\n"; 
        cerr << "You computer uses +++ "<<pcOrder[IsLittleEndian()]<<" +++ byte order!\n";
        cerr << "If different from byte order of input file, use byteswap = 1\n\n";  
        cerr << "Reference: bodyradius = 6378.140 (Earth)\n";
        cerr << "                      = 3396.0   (Mars)\n\n";
        return 1;
    }
	else
	{
    	if (SSCANF(argv[1], "%lf", &radius)  != 1)
    	{
    		cerr<<"Read error: body_radius\n";
    		return 1;
    	}
    			
    	if (SSCANF(argv[2], "%d", &width)  != 1)
    	{
    		cerr << "Bad image dimensions.\n";
        	return 1;
    	}
    	height = width / 2;
    	
    	if (argc > 3)
    	{    	
    		if (SSCANF(argv[3], "%lf", &exag)  != 1)    		
    		{
    			cerr<<"Read error: exag\n";
    			return 1;
    		}
    	
    		if (argc > 4)
    		{
    			if (SSCANF(argv[4], "%d", &byteSwap) != 1)
    			{
    				cerr << "Bad byteorder specs.\n";
        			return 1;
    			};

	    		if (argc == 6)
    			{    		
    				if (SSCANF(argv[5], "%lf", &heightmapunit) != 1)    		    	
    				{
    					cerr<<"Read error: heightmapunit\n";
    					return 1;
    				}
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

	//	
	// Output to STDOUT in PNG format
	//		  	
	png_structp png;
  	png_infop info;

	if ((png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL,NULL)) == NULL)
	{
    	fprintf(stderr, "Failed creating PNG write structure (out of memory?)");
    	exit(99);
	}
  	// Allocate image info
	if ((info = png_create_info_struct (png)) == NULL)
	{
    	fprintf(stderr, "Failed creating PNG info structure (out of memory?)");
    	exit(99);
  	}
    if (setjmp (png_jmpbuf (png)))
	{
    	fprintf(stderr, "Failed setting the PNG jump value (errors)");
    	exit(99);
	}
				
	png_init_io(png, stdout);	
	png_set_IHDR (png, info, width, height, 8, PNG_COLOR_TYPE_RGB,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png, 4);
	cerr<<"[nms    ]: Input file is a  16 bit elevation map:"<< setw(6)<<width<<" x"<< setw(6)<<height<<"\n\n";
	cerr<<"           Generating a normalmap for spherical geometry in PNG format"<<"\n\n";	
	clock_t start = clock(), end;  
	//write header 			
	png_write_info (png, info);
					
    short** h = new short* [height+1];
    unsigned char* rgb = new unsigned char[width * 3];

    for (int i = 0; i < kernelSize - 1; i++)
        h[i] = readRowS16(stdin, width);
    double bumpheight = heightmapunit * exag * width / (2000 * pi * radius); 
    double hp = pi / (double) height;

    for (int y = 0; y < height; y++)
    {

        // Out with the old . . .
        if (y > halfKernelSize)
            delete[] h[y - halfKernelSize - 1];

        // . . . and in with the new.
        if (y < height - halfKernelSize)
            h[y + halfKernelSize] = readRowS16(stdin, width);
       
        double spherecorr = 1.0 / sin(hp * (y + 0.5));
        for (int x = 0; x < width; x++)
        {
            double dx, dy;

            // use forward differences throughout!
			dx = (double)(h[y][x] - h[y][(x + 1) % width]);
            // the pixel x in the row nearest to South pole uses the
            // pixel across the pole at position width/2 + x for the dy gradient
            if (y == height - 1)
            	dy = (double)(h[y][x] - h[y][((width >> 1) + x) % width]);
            else			
                dy = (double)(h[y][x] - h[y + 1][x]);
                	
	    	dx *= bumpheight * spherecorr;
	    	dy *= bumpheight;
	    	double mag = sqrt(dx * dx + dy * dy + 1.0);
	    	double rmag = 127.0 / mag;

		    rgb[x * 3 + 0] = (unsigned char) (128.5 + dx * rmag);
		    rgb[x * 3 + 1] = (unsigned char) (128.5 + dy * rmag);
	    	rgb[x * 3 + 2] = (unsigned char) (128.5 + rmag);
		}
		png_bytep row_pointer = rgb;
		// Write a row of the image    	
		png_write_row(png, row_pointer);
     	if ((y + 1) % 1024 == 0 ) 
        {
        	end = clock();
			double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
			cerr << "nms ["<< setw(6) << y + 1 << " rows of " << setw(5) << height << " -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";
        }   			
	}			
	png_write_end (png, info);
  	png_destroy_write_struct (&png, &info);	
    return 0;
}


