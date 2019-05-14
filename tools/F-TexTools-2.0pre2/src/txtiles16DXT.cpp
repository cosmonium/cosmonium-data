//
//  Author: Dr. Fridger Schrempp
//
//  fridger.schrempp@desy.de
//
// The program reads unsigned bpp x 8 bit integer raw textures from STDIN.
// It outputs VT tiles in compressed DXT format, including many optimizations

// It is assumed that the input raw file is of power-of-two size



#include <iostream>
#include <ctime>
#include <cstdio>
#include <math.h>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <nvtt/nvtt.h>

#ifdef _WIN32
	#include <io.h>
	#include <fcntl.h>
#endif 

#if _MSC_VER >= 1400  // MSVC 2005/8
	#define SSCANF sscanf_s
#else  // _MSC_VER >= 1400
	#define SSCANF sscanf
#endif  // _MSC_VER >= 1400

using namespace nvtt;
using namespace std;

const string Version = "1.0, November 2008, author: F. Schrempp\n";

int byteSwap = 0;

int IsLittleEndian()
{
    short word = 0x4321;
    if((*(char *)& word) != 0x21 )
        return 0;
    else 
        return 1;
}

short* readRowTx(FILE* in, int width)
{
    short* row = new short[width];
    fread(row, 2, width, in);

    if (byteSwap == 1)
        for (int i = 0; i < width; i++)
            row[i] = ((row[i] & 0xff00) >> 8) | ((row[i] & 0x00ff) << 8);

    return row;
}

int RGBAtoBGRA(int col, int Bpp)
{
	// transform RGBA format to the supported BGRA format    
    if (Bpp == 1)
	   return col;
	else
	{
        switch(col)
        {
            case 0: return 2;
            case 1: return 1;
            case 2: return 0;
            case 3: return 3;
		   default: return col; 
		}
	}		  
}
string int2String(const int& number)
{
   ostringstream oss;
   oss << number;
   return oss.str();
}

int main(int argc, char* argv[])
{
    int height = 0;
    int width  = 0;
    int level  = 0;
    int bpp    = 3;
	int fast_mode     = 0;
	int dxt5_format   = 0;
	
	vector<string> pcOrder(2);	        
    pcOrder[0] = "Big-endian (PPC-MAC...)"; 
    pcOrder[1] = "Little-endian (Intel-PC, Intel-MAC)";
	
    if (argc < 4||argc > 6)
    {
        cerr << "\nUsage: txtilesDXT <channels> <width> <level> [<fast_mode> [<dxt5_format>]]\n\n";
        cerr << "Version "<< Version << endl;
		cerr << "--------------------------------------------------------------------\n";
        cerr << "The program reads textures in bpp x 8 bit unsigned integer raw format\n";        
		cerr << "from STDIN. It outputs tiles with many optimizations in\n"; 
		cerr << "DXT1 and DXT3/DXT5 format, for bpp = 1,3 and 4, respectively.\n\n";  
		cerr << "Since for RGBA, the alpha channel (A) compression quality is better\n"; 
		cerr << "in DXT5 than in DXT3, DXT5 may be selected by entering dxt5_format = 1\n";
		cerr << "But note: DXT5 filesize = 2 * DXT3 filesize!\n";         
		cerr << "--------------------------------------------------------------------\n\n"; 
		cerr << "Default  : High-quality compression mode (fast_mode = 0)\n";
		cerr << "           enter fast_mode = 1 for fast, decent quality compression\n";
        cerr << "Units    : tilesize[pixel] = width[pixel]/2^(level+1).\n\n";
		cerr << "Input    : Interleaved RGB(A) storage mode ie. RGB(A)RGB(A)RGB(A)...\n";
		cerr << "           for RGB (+ alpha) textures\n";
        cerr << "           Inputwidth : inputheight = 2 : 1, power-of-two size.\n\n";   
		cerr << "For 4 x 8 bit RGB + alpha textures enter channels = 4.\n";
        cerr << "For 3 x 8 bit  colored    textures enter channels = 3.\n";      
        cerr << "For 1 x 8 bit grayscale   textures enter channels = 1.\n\n";     
        cerr<< "\n-------------------------------------------------------\n\n";
        cerr << "You computer uses +++ "<<pcOrder[IsLittleEndian()]<<" +++ byte order!\n";
        return true;
    }   
	else
	{
        if (SSCANF(argv[1], " %d", &bpp) != 1)
        {
            cerr << "Bad image type.\n";
            return false;
        }   	
    			
    	if (SSCANF(argv[2], "%d", &width)  != 1)
    	{
    		cerr << "Bad image dimensions.\n";
        	return false;
    	}
    	if (SSCANF(argv[3], " %d", &level)  != 1)
    	{
    		cerr << "Bad level input.\n";
        	return false;
    	}
		if (argc > 4)
		{
			if (SSCANF(argv[4], "%d", &fast_mode)  != 1)
    		{
    			cerr << "Bad Compression Quality value.\n";
        		return false;
			}
			if (argc > 5)
            {
                if (SSCANF(argv[5], "%d", &dxt5_format) != 1)
                {
                    cerr << "Bad Compression Format entry.\n";
                    return 1;
                }   
            }	
		}
    	
    	height = width / 2;
    	
	}
	    	
	#ifdef _WIN32
		if (_setmode(_fileno(stdin), _O_BINARY) == -1 )
    		{
    			cerr<<"Binary read mode from STDIN failed\n";
    			return false;
    		}
	#endif
    
    const double pi = 3.1415926535897932385;
    double       hp = pi / (double) height;
    int      hlevel = 1 << level;
    int          hj = height/hlevel;
	int    rgbWidth = bpp * width;
	

    string filetype, cformat;
	Format compressFormat;	
	
    switch (bpp)
    {
		case 1:
			filetype = "1x8 bit grayscale map:";
			cformat  = "DXT1 = BC1";
			compressFormat = Format_BC1;     
			break;
		case 3:
			filetype = "3x8 bit RGB color map:";
			cformat  = "DXT1 = BC1";
			compressFormat = Format_BC1;
			/*! DXT1 and DXT3 compression give identical error results
			    with nvimgdiff for RGB, but DXT1 format has only half 
			    the size of DXT3.
			*/		
			break;
		case 4:
			filetype = " 4x8 bit RGBA texture:";
			if (dxt5_format)
			{
			    cformat  = "DXT5 = BC3";
			    compressFormat = Format_BC3;
			}
			else
			{
			    cformat  = "DXT3 = BC2";
				compressFormat = Format_BC2;
			}	 			
			break;
       default:
			filetype = "3x8 bit RGB color map:";
			cformat  = "DXT1 = BC1";
			compressFormat = Format_BC1;       
	}			
	// Initial printout:
		
	cerr<<"\n[txtilesDXT]: Input file is a "<< filetype << setw(6)<<width<<" x"<< setw(6)<<height<<"\n\n";
    cerr<<"              Generating  "<< width * height/(hj * hj)<<" optimized VT tiles for level "<<level<< "\n";
    
    int reductionFak = (1 << (int)(log(1.0/sin(hp * hj))/log(2.0)));
    reductionFak = level > 0? reductionFak: 1;
    cerr<<"              in "<<cformat<<" format,";
    if (reductionFak == 1)
		cerr<<" of square size "<<hj<<" x "<<hj<<"\n\n";
    else
		cerr<<" of size from "<<hj/reductionFak<<" x "<<hj<<" to "<<hj<<" x "<<hj<<"\n\n";	
	if (fast_mode == 1)			    
	    cerr<<"              Fast, DXT compression mode of decent quality.\n"; 
	else
	{
	    cerr<<"              High-quality DXT compression,\n";
	    cerr<<"              about (3 - 6)x slower than fast-mode!!\n";
	}
	cerr<<endl;


	// partition into hlevel cells of tile height hj (hj * hlevel = height) 
		
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
		
	CompressionOptions compressionOptions;		
	compressionOptions.setFormat(compressFormat);
	if (fast_mode == 1)			
		compressionOptions.setQuality(Quality_Fastest);
	else
		compressionOptions.setQuality(Quality_Normal);
		
    short** h = new short* [height + 1];
    h[0] = readRowTx(stdin, rgbWidth);
        	
    // partition into hlevel cells of tile height hj (hj * hlevel = height)
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
			h[jtile + j] = readRowTx(stdin, rgbWidth);
        }
		        
		// calculate reduction of horizontal resolution depending 
		// on latitude 	
		        
		int k = 1;
        if (level > 0)
        {
        	if (jl < hlevel / 2)
        		k = 1 << (int)(log(1.0/sin(hp * (jtile + hj)))/log(2.0));
            else
        		k = 1 << (int)(log(1.0/sin(hp * jtile))/log(2.0));
		}
		unsigned char* rgba  = new unsigned char [4 * hj/k * hj];
		inputOptions.setTextureLayout(TextureType_2D, hj/k, hj);			
											
		//partition cells horizontally into 2 * hlevel tiles        
		for (int il = 0; il < 2 * hlevel; il++)
        {
            int  itile = il * hj;
            
       		// set up tile (il, jl -1)
        	for (int j = 0; j < hj; j++)
        	{
                int jj = jtile + j;	
                for (int i = 0; i < hj/k; i++)
                {
                    int ii = bpp * (itile + i * k);	
		        	int ij =   4 * (j * hj / k + i); 
					
                    // pixel data in BGRA_8UB format as required
                    for (int col = 0; col < bpp; col++)
					   rgba[ij + col] =  h[jj][ii + RGBAtoBGRA(col, bpp)];
					 
					// Complement to bpp = 4 size for bpp < 4 input formats
					if(bpp <= 3){rgba[ij + 3] = 0xFF;} // RGB input
					if(bpp == 1) // grayscale input
					{
					   rgba[ij + 1] = rgba[ij]; 
					   rgba[ij + 2] = rgba[ij];
					}					
				}
			} 		
			inputOptions.setMipmapData(rgba, hj/k, hj);			
			
			// Generate VT name syntax       		
			string filename = "tx_" + int2String(il) + "_" + int2String(jl) + ".dds";									       
			
			OutputOptions outputOptions;			
			outputOptions.setFileName(filename.c_str());			
			
			// compress	entire VT tiles		
			Compressor compressor;				
			compressor.process(inputOptions, compressionOptions, outputOptions);
		}					              
		end = clock();
		double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
		cerr << "tile["<< setw(8) << 2 * hlevel * (jl + 1) << " VT's of "<< setw(5) << 2 * hlevel * hlevel <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n"; 		
  	}
    return true;
}



