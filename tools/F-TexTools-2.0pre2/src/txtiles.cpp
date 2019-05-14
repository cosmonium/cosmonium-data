//
//  Author: Fridger Schrempp
//
//  fridger.schrempp@desy.de
//
// The program reads unsigned bpp x 8 bit integer raw textures from STDIN.
// It outputs VT tiles in PNG format, including many optimizations

// It is assumed that the input raw file is of power-of-two size



#include <iostream>
#include <cstdio>
#include <math.h>
#include <sstream>
#include <string>
#include <vector>
#include <png.h>
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
using namespace std;

const string version = "1.0, August 2007, author: F. Schrempp\n";

unsigned char* readRowTx(FILE* in, int width)
{
    unsigned char* row = new unsigned char[width];
    fread(row, 1, width, in);
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
    int height = 0;
    int width  = 0;
    int level  = 0;
    int bpp    = 3;
	int compression = Z_BEST_SPEED;
	        
    if (argc < 4||argc > 5)
    {
        cerr << "\nUsage: txtiles <channels> <width> <level> [<PNG_compression>]\n\n";
        cerr << "Version "<< version << endl;
		cerr << "--------------------------------------------------------------------\n";
        cerr << "The program reads textures in unsigned bpp x 8 bit integer raw format\n";        
		cerr << "from STDIN. It outputs VT tiles with many optimizations in PNG format.\n";  
        cerr << "--------------------------------------------------------------------\n\n"; 
        cerr << "Units    : tilesize[pixel] = width/2^(level+1).\n\n";
		cerr << "Input    : Interleaved RGB(A) storage mode ie. RGB(A)RGB(A)RGB(A)...\n";
		cerr << "           for RGB (+ alpha) textures\n";
        cerr << "           Inputwidth : inputheight = 2 : 1, power-of-two size.\n";
        cerr << "           No header.\n";
		cerr << "Default  : PNG_compression = Z_BEST_SPEED = 1\n";
		cerr << "         : best choice for subsequent DXT compression!\n\n";
		cerr << "For VT tiles in PNG format, enter PNG_compression = 6..9 (slow!)\n\n";   
		cerr << "For 4 x 8 bit RGB + alpha textures enter channels = 4.\n";
        cerr << "For 3 x 8 bit  colored    textures enter channels = 3.\n";      
        cerr << "For 1 x 8 bit grayscale   textures enter channels = 1.\n\n";     
        
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
			if (SSCANF(argv[4], " %d", &compression)  != 1)
    		{
    			cerr << "Bad PNG compression value.\n";
        		return false;
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
	int   bit_depth = 8;
	int  color_type;
	
    unsigned char** h = new unsigned char* [height + 1];
	//unsigned char* out0 = new unsigned char [4 * 4 * bpp];
	
	FILE* fp = NULL;
    string filetype, format = ".png";
	switch (bpp)
    {
		case 1:
			filetype = "1x8 bit grayscale map:";
			color_type = PNG_COLOR_TYPE_GRAY;      // 0
			break;
		case 3:
			filetype = "3x8 bit RGB color map:";
			color_type = PNG_COLOR_TYPE_RGB;       // 2
			break;
		case 4:
			filetype = " 4x8 bit RGBA texture:";
			color_type = PNG_COLOR_TYPE_RGB_ALPHA; // 6
			break;
       default:
			filetype = "3x8 bit RGB color map:";
			color_type = PNG_COLOR_TYPE_RGB;       // 2
	}

    cerr<<"[txtiles]: Input file is a "<< filetype << setw(6)<<width<<" x"<< setw(6)<<height<<"\n\n";
    cerr<<"           Generating  "<< width * height/(hj * hj)<<" optimized VT tiles for level "<<level<< "\n";
    
    int reductionFak = (1 << (int)(log(1.0/sin(hp * hj))/log(2.0)));
    reductionFak = level > 0? reductionFak: 1;
    cerr<<"           in PNG format,";
    if (reductionFak == 1)
		cerr<<"of square size "<<hj<<" x "<<hj<<"\n\n";
    else
		cerr<<"of size from "<<hj/reductionFak<<" x "<<hj<<" to "<<hj<<" x "<<hj<<"\n\n";
	cerr<<"           PNG_compression level = "<<compression<<" (range: 0..9)"<<endl;
	cerr<<endl;

	// partition into hlevel cells of tile height hj (hj * hlevel = height) 
	clock_t start = clock(), end;     
	h[0] = readRowTx(stdin, rgbWidth);
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
        // calculate reduction of horizontal resolution depending on latitude 	
        int k = 1;
        if (level > 0)
        {
        	if (jl < hlevel / 2)
        		k = 1 << (int)(log(1.0/sin(hp * (jtile + hj)))/log(2.0));
            else
        		k = 1 << (int)(log(1.0/sin(hp * jtile))/log(2.0));
     	}
     	
        unsigned char* out  = new unsigned char [bpp * hj/k * hj];
        
        // partition cells horizontally into 2 * hlevel tiles
        for (int il = 0; il < 2 * hlevel; il++)
        {
            int  itile = il * hj;
			// VT tile name generation 
    		string filename = "tx_" + int2String(il) + "_" + int2String(jl) + format;
            
			//fprintf(stderr,"%s\t ", filename.c_str());
			
			if((fp = fopen(filename.c_str(), "wb")) == NULL)
			{
    		  	cerr<<"Could not open output image"<<endl;
    			return false;
			}

       		// set up tile (il, jl -1)
        	for (int j = 0; j < hj; j++)
        	{
                int jj = jtile + j;	
                for (int i = 0; i < hj/k; i++)
                {
                    int ii = bpp * (itile + i * k);	
		        	int ij = bpp * (j * hj / k + i); 
		        	for (int col = 0; col < bpp; col++)
		        		out[ij + col] =  h[jj][ii + col];
                }
           	}
			// PNG tile output
            //===========
			
			// Headers first
			png_structp png_ptr;
			png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			if (png_ptr == NULL)
			{
				cerr<<"png_create_write_struct failed"<<endl;
				return false;
			}
			png_infop info_ptr  = png_create_info_struct(png_ptr);
			if (info_ptr == NULL)
			{
				cerr<<"png_create_info_struct failed"<<endl;
				png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
				return false;
			}
			if (setjmp(png_jmpbuf(png_ptr)))
			{
				cerr<<"Error during init_io"<<endl;
				png_destroy_write_struct(&png_ptr, &info_ptr);
				return false;
			}
			png_init_io(png_ptr, fp);
			png_set_compression_level(png_ptr,compression);
			if (setjmp(png_jmpbuf(png_ptr)))
			{
				cerr<<"Error during writing header"<<endl;
				png_destroy_write_struct(&png_ptr, &info_ptr);
				return false;
			}
			png_set_IHDR(png_ptr, info_ptr, hj / k, hj, bit_depth, color_type,
			             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
						 PNG_FILTER_TYPE_DEFAULT);	
			png_write_info(png_ptr, info_ptr);
            // calculate row_pointers
			png_bytepp row_pointers = new png_bytep [hj* sizeof (png_bytep)];	
			for (int jj = 0; jj < hj; ++jj)
    			row_pointers[jj] = out + (jj * bpp * hj / k);

            // Write out the whole tile			
            png_write_image(png_ptr, row_pointers);
			png_write_end(png_ptr, info_ptr);
			png_destroy_write_struct(&png_ptr, &info_ptr);           	
        	fclose(fp);            
    	}
        end = clock();
		double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
		cerr << "tile["<< setw(6) << 2 * hlevel * (jl + 1) << " VT's of "<< setw(5) << 2 * hlevel * hlevel <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n"; 		
  	}
    return true;
}



