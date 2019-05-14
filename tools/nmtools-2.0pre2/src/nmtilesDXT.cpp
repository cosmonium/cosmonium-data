//
//  Authors: Fridger Schrempp & Robert Skuridin
//
//  fridger.schrempp@desy.de
//  skuridin1@mail.ru
//
// The program reads an elevation map in signed 16-bit raw format  
// from STDIN. It outputs normalmap tiles in high-quality DXT5nm format, 
// corrected for spherical geometry and including many optimizations.

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
#include <nvtt/nvtt.h>

#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
#endif 

#if _MSC_VER >= 1400  // MSVC 2005/8
	#define SSCANF sscanf_s
#else 
	#define SSCANF sscanf
#endif

using namespace nvtt;
using namespace std;

const string Version = "1.0, November 2008, authors: F. Schrempp and R. Skuridin\n";

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

string int2String(const int& number)
{
   ostringstream oss;
   oss << number;
   return oss.str();
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
	int    fast_mode      = 0;
	
    vector<string> pcOrder(2);

    pcOrder[0] = "Big-endian (PPC-MAC...)"; 
    pcOrder[1] = "Little-endian (Intel-PC, Intel-MAC)";

    if (argc < 4 || argc > 7)
    {
        cerr << "\nUsage: nmtilesDXT <level> <bodyradius> <width> [<exag> [<byteswap> [<fast>]]]\n\n";
        cerr << "Version "<< Version << endl;
        cerr << "-------------------------------------------------------\n";
        cerr << "The program reads an elevation map in signed 16-bit integer raw\n";
        cerr << "format from STDIN. It outputs normalmap tiles in\n"; 
		cerr << "high-quality DXT5nm format for bodies of ~ spherical geometry,\n";
        cerr << "including various crucial optimizations!\n\n";
        cerr << "Units    : tilesize[pixel] = width/2^(level+1)\n";
        cerr << "           bodyradius[km], width[pixel]\n\n";
        cerr << "Assume   : input file has width : height = 2 : 1, power-of-two size\n\n";
        cerr << "Defaults : exaggeration factor: exag = 1.0,\n"; 
        cerr << "           (recommended  value: exag ~ 2.5)\n";
        cerr << "           byteswap = 0 <=> byte ordering of input file and computer are equal\n";
        cerr << "           fast = 0 (high quality DXT5nm compressor),\n"; 
		cerr << "           select fast, decent quality compressor for fast = 1.\n\n";
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
                    if (SSCANF(argv[6], "%d", &fast_mode) != 1)
                    {
                        cerr<<"Read error: fast\n";
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

  
	
    cerr<<"[nmtilesDXT]: Input file is a  16 bit elevation map:"<< setw(6)<<width<<" x"<< setw(6)<<height<<"\n\n";
    cerr<<"              Generating  "<< width * height/(hj * hj)<<" optimized normalmap tiles for level "<<level<< "\n";

    int reductionFak = (1 << (int)(log(1.0/sin(hp * hj))/log(2.0)));
    reductionFak = level > 0? reductionFak: 1;

    cerr<<"              in DXT5nm format,"; 
	if (reductionFak == 1)
		cerr<<" of square size "<<hj<<" x "<<hj<<"\n\n";
    else
		cerr<<" of size from "<<hj/reductionFak<<" x "<<hj<<" to "<<hj<<" x "<<hj<<"\n\n";	

	clock_t start = clock(), end;     
		    
    InputOptions inputOptions;
    inputOptions.reset();
    
    /*! Reset implies:
        
        WrapMode_Mirror, TextureType_2D, InputFormat_BGRA_8UB, AlphaMode_Transparency, input/output Gamma = 2.2f, ColorTransform_None, linearTransform = Matrix(identity), generateMipmaps = true, maxLevel = -1, MipmapFilter_Box, isNormalMap = false, normalizeMipmaps = true, convertToNormalMap = false, RoundMode_None
    */    

	// We only want mipmaps for the lowest level, obviously.
    if (level == 0)
	   inputOptions.setMipmapGeneration(true);
	else
       inputOptions.setMipmapGeneration(false);
          
    // Do normal map conversion/compressions here:
	
    inputOptions.setNormalMap(true);			
    
    // standard in/out Gamma for normalmaps!	
    inputOptions.setGamma(1.0f, 1.0f);
			
    CompressionOptions compressionOptions;				
    // request high-quality normalmap compression format DXT5nm , as defined 
    // by x-> A, y-> G, R=0xFF, B=0 in NVTT
      	
    compressionOptions.setFormat(Format_DXT5n);
	
	if (fast_mode == 1)			
		compressionOptions.setQuality(Quality_Fastest);
	else
		compressionOptions.setQuality(Quality_Normal);
    
    // partition into hlevel cells of tile height hj (hj * hlevel = height)	
    
    short** h = new short* [height + 1];
    h[0] = readRowS16(stdin, width);
    
    unsigned char* bgra0 = new unsigned char [4 * 4 * 4];
				
	
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

        unsigned char* bgra  = new unsigned char [4 * hj/k * hj];
        
		
        // partition cells horizontally into 2 * hlevel tiles
        for (int il = 0; il < 2 * hlevel; il++)
        {
            bool flag  = true;
            int  itile = il * hj;

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
                        
                        /* the pixel ii in the row nearest to South pole uses
                          the pixel across the pole at position width/2 + ii
                          for the dy gradient
                        */ 
                        if (jj == height - 1)
                            dy += (double)(h[jj][ii] - h[jj][((width >> 1) + ii) % width]);
                        else
                            dy += (double)(h[jj][ii] - h[jj + 1][ii]);
                    } 
                    dx *= bumpheight * spherecorr / (double)(k * k);
                    dy *= bumpheight / (double) k;
                    double mag =  sqrt(dx * dx + dy * dy + 1.0);
                    double rmag = 127.0 / mag;
                    int ij = 4 * (j * hj / k + i); 
                    
                    // normalmap in BGRA_8UB format as required
                    bgra[ij + 0] = (unsigned char) (128.5 + rmag);      // b
                    bgra[ij + 1] = (unsigned char) (128.5 + dy * rmag); // g
                    bgra[ij + 2] = (unsigned char) (128.5 + dx * rmag); // r
                    bgra[ij + 3] = 0xFF;            // complement to 4-block
                     
                    // check whether normalmap is monochrome! => flag = true
                    flag = ((bgra[ij + 0] == bgra[0]) && (bgra[ij + 1] == bgra[1]) && (bgra[ij + 2] == bgra[2]) && flag);
				}	
			
			}
	        // Generate VT name syntax		
            string filename = "tx_" + int2String(il) + "_" + int2String(jl) + ".dxt5nm";	
			
			OutputOptions outputOptions;			
			outputOptions.setFileName(filename.c_str());			
			
            if (flag)			
            {
                // replace monochrome tiles by smallest block (4x4x4) accepted by DXT format
                               
                for (int i = 0; i < 16; i++)
                {
                    bgra0[i * 4 + 0] = bgra[0];
                    bgra0[i * 4 + 1] = bgra[1];
                    bgra0[i * 4 + 2] = bgra[2];
					bgra0[i * 4 + 3] = bgra[3];
				}
				inputOptions.setTextureLayout(TextureType_2D, 4, 4);	
				inputOptions.setMipmapData(bgra0, 4, 4);
			}
            else   
			{	
                // regular normalmap textures         
			    inputOptions.setTextureLayout(TextureType_2D, hj/k, hj);
    			inputOptions.setMipmapData(bgra, hj/k, hj);					
            }			
			// compress	entire VT tiles		
			Compressor compressor;				
			compressor.process(inputOptions, compressionOptions, outputOptions);
		}      
        end = clock();
		double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
		cerr << "tile["<< setw(8) << 2 * hlevel * (jl + 1) << " VT's of "<< setw(5) << 2 * hlevel * hlevel <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";  						
    }		
    return 0;
}
