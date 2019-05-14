//
//  Author:  Fridger Schrempp
//
//  fridger.schrempp@desy.de
//
// The program reads unsigned bpp x 8 bit raw textures from STDIN.    
// The RGB(A) storage mode is assumed to be interleaved, ie. RGB(A)RGB(A)RGB(A)RGB(A) ...
// The program converts the texture to PNG format at STDOUT.

#include <iostream>
#include <cstdio>
#include <math.h>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>

#include <png.h>

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

int main(int argc, char* argv[])
{
    int width  = 0;
    int height = 0;
    int headlines = 0;    
    int bpp = 3;
	int bit_depth = 8;
	int color_type;
     
    if (argc < 3||argc > 5)
    { 
        cerr << "\nUsage: bin2png <channels> <width> [<height> [<#headerlines>]]\n\n";
        cerr << "Version "<< version << endl;
        cerr << "----------------------------------------------------------------------\n";
        cerr << "The program reads textures in unsigned bpp x 8 bit integer raw format \n";
        cerr << "from STDIN and converts them to PNG format at STDOUT.\n";
        cerr << "----------------------------------------------------------------------\n\n";     
		cerr << "Assume :  Interleaved RGB(A) ordering (RGB(A)RGB(A)RGB(A)...).\n";
		cerr << "       :  PNG_compression = 6; (0..9(slow!))\n\n";
        cerr << "Defaults: Inputwidth : inputheight = 2 : 1.\n";
        cerr << "          No header (#headerlines = 0). \n\n";
        cerr << "For 4 x 8 bit RGB + alpha textures enter channels = 4.\n";
        cerr << "For 3 x 8 bit RGB colored textures enter channels = 3.\n";      
        cerr << "For 1 x 8 bit  grayscale  textures enter channels = 1.\n";
        cerr << "For different aspect ratios enter <height> in pixels.\n";           
        cerr << "For PDS .img, .edr,...format  enter #headerlines  = 2,....\n\n";            
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
        
        if (argc > 3)
        {
            if (SSCANF(argv[3], " %d", &height) != 1)
        	{
            	cerr << "Bad image dimensions.\n";
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
	
	if (height == 0)
		height = width / 2; 
   	
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
	cerr << "[bin2png]: Converting binary to PNG format output"<<endl;
	cerr << "           bpp: "<<bpp<<"  size: "<< setw(6) << width <<" x" << setw(6) << height<<endl;
	
   	int rgbWidth = bpp * width;
    unsigned char* h = new unsigned char [rgbWidth];
        
    if (headlines)
    {
    	cerr << "\nSkipping "<< headlines <<" header records"<<endl;
  	 
    	for (int ih = 1; ih <= headlines; ih++)
        {
			if (fread(h, 1, rgbWidth, stdin) != (size_t) rgbWidth)
			{
				cerr<<"Read error in RGB file at STDIN!";
				return false;
			}
		}	
    }
	switch (bpp)
	{
		case 1:
			color_type = PNG_COLOR_TYPE_GRAY;      // 0
			break;
		case 3:
			color_type = PNG_COLOR_TYPE_RGB;       // 2
			break;
		case 4:
			color_type = PNG_COLOR_TYPE_RGB_ALPHA; // 6
			break; 
		default:
			color_type = PNG_COLOR_TYPE_RGB;       // 2
	}
	clock_t start = clock(), end; 

	//
	// PNG output
	// ==========
    //
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
	png_init_io(png_ptr, stdout);
	png_set_compression_level(png_ptr,6);
	
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		cerr<<"Error during writing header"<<endl;
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}
	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
               	PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);	
    png_write_info(png_ptr, info_ptr);	
    
    
	for (int j = 0; j < height; j++)
    {
		if (fread(h, 1, rgbWidth, stdin) != (size_t) rgbWidth)
		{
			cerr<<"Read error in RGB file at STDIN!";
			return false;
	    }
		png_write_row(png_ptr, h);
     	if ((j + 1) % 1024 == 0 ) 
        {
			end = clock();
			double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
			cerr << "png ["<< setw(6) << j + 1 << " rows of " << setw(5) << height <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";
        }   		
    }	
    png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
    return true;
}


