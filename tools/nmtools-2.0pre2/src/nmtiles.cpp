//
//  Authors: Fridger Schrempp & Robert Skuridin
//
//  fridger.schrempp@desy.de
//  skuridin1@mail.ru
//
// The program reads an elevation map in signed 16-bit raw format  
// from STDIN. It outputs normalmap tiles (PNG format), corrected for
// spherical geometry and including many optimizations

// It is assumed that the input raw file is of power-of-two size

#include <iostream>
#include <cstdio>
#include <math.h>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <png.h>
#include <iomanip>

#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
	#define snprintf  _snprintf_s 
#endif 

#if _MSC_VER >= 1400  // MSVC 2005/8
	#define SSCANF sscanf_s

#else 	
	#define SSCANF sscanf
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

short* readRowS16(FILE* in, int width)
{
    short* row = new short[width];
    fread(row, 2, width, in);

    if (byteSwap == 1)
        for (int i = 0; i < width; i++)
            row[i] = ((row[i] & 0xff00) >> 8) | ((row[i] & 0x00ff) << 8);

    return row;
}

int main(int argc, char* argv[])
{
    const double pi = 3.1415926535897932385;
    int height = 0;
    int width  = 0;
    int level  = 0;
    double radius         = 0.0;
    double exag           = 1.0;
    double heightmapunit  = 1.0;
    vector<string> pcOrder(2);

    pcOrder[0] = "Big-endian (PPC-MAC...)"; 
    pcOrder[1] = "Little-endian (Intel-PC, Intel-MAC)";

    if (argc < 4 || argc > 7)
    {
        cerr << "\nUsage: nmtiles  <level> <bodyradius> <width> [<exag> [<byteswap> [<heightmapunit>]]]\n\n";
        cerr << "Version "<< version << endl;
        cerr << "-------------------------------------------------------\n";
        cerr << "The program reads an elevation map in signed 16-bit integer raw\n";
        cerr << "format from STDIN. It outputs normalmap tiles (PNG format),\n";
        cerr << "for bodies of ~spherical geometry, with various crucial optimizations!\n\n";
        cerr << "Units    : tilesize[pixel] = width/2^(level+1)\n";
        cerr << "           bodyradius[km], width[pixel]\n\n";
        cerr << "Assume   : input file has width : height = 2 : 1, power-of-two size\n\n";
        cerr << "Defaults : exag = 1.0 (exaggeration factor, recommended: exag ~ 2.5)\n";
        cerr << "           byteswap = 0 <=> byte ordering of input file and computer are equal\n";
        cerr << "           heightmapunit = 1.0 (length of one heightmap unit in meters)\n\n";
        cerr << "-------------------------------------------------------\n\n";
        cerr << "You computer uses +++ "<<pcOrder[IsLittleEndian()]<<" +++ byte order!\n";
        cerr << "If different from byte order of input file, use byteswap = 1\n\n";
        cerr<<"Reference: bodyradius = 6378.140 (Earth)\n";
        cerr<<"                      = 3396.0   (Mars)\n\n";

        return 1;
    }
    else
    {
        if (SSCANF(argv[1], " %d", &level)  != 1)
        {
            cerr << "Bad level input.\n";
            return 1;
        }
        if (SSCANF(argv[2], "%lf", &radius)  != 1)
        {
            cerr<<"Read error: body_radius\n";
            return 1;
        }

        if (SSCANF(argv[3], "%d", &width)  != 1)
        {
            cerr << "Bad image dimensions.\n";
            return 1;
        }
        height = width / 2;

        if (argc > 4)
        {
            if (SSCANF(argv[4], "%lf", &exag)  != 1)
            {
                cerr<<"Read error: exag\n";
                return 1;
            }

            if (argc > 5)
            {
                if (SSCANF(argv[5], "%d", &byteSwap) != 1)
                {
                    cerr << "Bad byteorder specs.\n";
                    return 1;
                };

                if (argc == 7)
                {
                    if (SSCANF(argv[6], "%lf", &heightmapunit) != 1)
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
    #endif

    double bumpheight = heightmapunit * exag * width / (2000 * pi * radius); 
    double hp = pi / (double) height;
    int    hlevel = 1 << level;
    int    hj = height/hlevel;


    short** h = new short* [height + 1];
    unsigned char* rgb0 = new unsigned char [4 * 4 * 3];

    FILE* fp = NULL;
	
    cerr<<"[nmtiles]: Input file is a  16 bit elevation map:"<< setw(6)<<width<<" x"<< setw(6)<<height<<"\n\n";
    cerr<<"           Generating  "<< width * height/(hj * hj)<<" optimized normalmap tiles for level "<<level<< "\n";

    int reductionFak = (1 << (int)(log(1.0/sin(hp * hj))/log(2.0)));
    reductionFak = level > 0? reductionFak: 1;

    cerr<<"           in PNG format, with sizes from "<<hj/reductionFak<<" x "<<hj<<" to "<<hj<<" x "<<hj<<"\n\n";

    // partition into hlevel cells of tile height hj (hj * hlevel = height)
	clock_t start = clock(), end;     
	h[0] = readRowS16(stdin, width);
    for (int jl = 0; jl < hlevel; jl++)
    {
        int jtile = jl * hj;
        int jma = hj + 1;
        if (jl == hlevel - 1)
            jma = hj;
        for (int j = 1; j < jma; j++)
        {
            // delete previous cell
            if (jl > 0)
                 delete[] h[jtile - hj + j - 1];
            // read in next cell    
            h[jtile + j] = readRowS16(stdin, width);
        }
        // calculate reduction of horizontal resolution depending on latitude
        int k = 1;
        if (level > 0)
        {
            if (jl < hlevel / 2)
                k = 1 << (int)(log(1.0/sin(hp * (jtile + hj)))/log(2.0));
            else
                k = 1 << (int)(log(1.0/sin(hp * jtile))/log(2.0));
         }

        unsigned char* rgb  = new unsigned char [3 * hj/k * hj];
         

        // partition cells horizontally into 2 * hlevel tiles
        for (int il = 0; il < 2 * hlevel; il++)
        {
            bool flag  = true;
            int  itile = il * hj;
            char filename[50];

            // calculate normalmap for tile (il, jl -1)
            for (int j = 0; j < hj; j++)
            {
                int jj = jtile + j;
                double spherecorr = 1.0 / sin(hp * (jj + 0.5));

                for (int i = 0; i < hj/k; i++)
                {
                    double dx = 0.0;
                    double dy = 0.0;
                    for (int ii = itile + i * k; ii < itile + (i + 1)  * k; ii++)
                    {
                        dx += (double)(h[jj][ii] - h[jj][(ii + k) % width]);
                        // the pixel ii in the row nearest to South pole uses the
                        // pixel across the pole at position width/2 +ii for the dy gradient
                        if (jj == height - 1)
                            dy += (double)(h[jj][ii] - h[jj][((width >> 1) + ii) % width]);
                        else
                            dy += (double)(h[jj][ii] - h[jj + 1][ii]);
                    } 
                    dx *= bumpheight * spherecorr / (double)(k * k);
                    dy *= bumpheight / (double) k;
                    double mag =  sqrt(dx * dx + dy * dy + 1.0);
                    double rmag = 127.0 / mag;
                    int ij = 3 * (j * hj / k + i); 
                    rgb[ij + 0] = (unsigned char) (128.5 + dx * rmag);
                    rgb[ij + 1] = (unsigned char) (128.5 + dy * rmag);
                    rgb[ij + 2] = (unsigned char) (128.5 + rmag);

                    // check whether normalmap is monochrome! => flag = true
                    flag = ((rgb[ij + 0] == rgb[0]) && (rgb[ij + 1] == rgb[1]) && (rgb[ij + 2] == rgb[2]) && flag);
				}	
			}
			//	
			// Output tiles in PNG format
			//		  	
  		  	png_structp png;
  			png_infop info;

            snprintf(filename, sizeof(filename), "tx_%d_%d.png", il, jl);
						
			if((fp = fopen(filename, "wb")) == NULL)
			{
    		  	fprintf(stderr, "Could not open output image\n");
    			exit(99);
			}
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
				
			png_init_io(png, fp);
			png_bytepp row_pointers = NULL;								
						
			if (flag)			
            {
				row_pointers = new png_bytep [4 * sizeof (png_bytep)];
			
                // replace monochrome tiles by smallest one (4x4) accepted by DXT format
                
                for (int i = 0; i < 16; i++)
                {
                    rgb0[i * 3 + 0] = rgb[0];
                    rgb0[i * 3 + 1] = rgb[1];
                    rgb0[i * 3 + 2] = rgb[2];
				}
			  	for (int jj = 0; jj < 4; ++jj)
    				row_pointers[jj] = rgb0 + (jj * 12);
  				// Define image header etc.
  				png_set_IHDR (png, info, 4, 4, 8, PNG_COLOR_TYPE_RGB,PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
            }
            else   
			{   
				row_pointers = new png_bytep [hj* sizeof (png_bytep)];
			  	for (int jj = 0; jj < hj; ++jj)
    				row_pointers[jj] = rgb + (jj * 3 * hj / k);
  				// Define image header etc.
  				png_set_IHDR (png, info, hj/k, hj, 8, PNG_COLOR_TYPE_RGB,PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
							
			}
			png_set_compression_level(png, 3);	
			//write header 			
			png_write_info (png, info);
			// Write the image 
			png_write_image (png, row_pointers);
			png_write_end (png, info);
  			png_destroy_write_struct (&png, &info);	
            fclose(fp);
		}      
        end = clock();
		double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
		cerr << "tile["<< setw(6) << 2 * hlevel * (jl + 1) << " VT's of "<< setw(5) << 2 * hlevel * hlevel <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";  						
    }		
    return 0;
}
