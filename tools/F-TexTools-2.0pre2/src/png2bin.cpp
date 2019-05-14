//
//  Author:  Fridger Schrempp
//
//  fridger.schrempp@desy.de
//
// The program reads PNG textures from STDIN and converts them 
// to unsigned bpp x 8 bit integer raw format at STDOUT.
// The RGB(A) storage mode is interleaved, ie. RGB(A)RGB(A)RGB(A)RGB(A) ...

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
using namespace std;

const string version = "1.0, October 2007, author: F. Schrempp\n";

int main(int argc, char* argv[])
{
    png_uint_32 width  = 0;
    png_uint_32 height = 0;
    int bpp;
	int bit_depth = 8;
	int color_type = 0;
    if (argc > 1)
    { 
        cerr << "\nUsage: png2bin\n\n";
        cerr << "Version "<< version << endl;
        cerr << "----------------------------------------------------------------------\n";
        cerr << "The program reads PNG textures from STDIN and \n";
        cerr << "converts them to unsigned bpp x 8 bit integer raw format at STDOUT.\n";
        cerr << "----------------------------------------------------------------------\n\n";     
		cerr << "Interleaved RGB(A) ordering: (RGB(A)RGB(A)RGB(A)...).\n";
        return 1;
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
	// PNG input
	// ==========
    //
	clock_t start = clock(), end;
	png_structp png_ptr;
	png_infop info_ptr, end_info;
	unsigned char sig[8];
	
	// Check that it really is a PNG file
  	fread(sig, 1, 8, stdin);
	if(!png_check_sig(sig, 8))
	{
    	cerr<< "This file is not a valid PNG file"<<endl;
    	return false;
    }
  	if((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL)
	{
    	cerr << "Could not create a PNG read structure (out of memory?)"<<endl;
    	return false;
	}
	if((info_ptr = png_create_info_struct(png_ptr)) == NULL)
	{
		cerr<<"png_create_info_struct failed"<<endl;
		png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		return false;			
	}
	if((end_info = png_create_info_struct(png_ptr)) == NULL)
	{
		cerr<<"png_create_info_struct failed"<<endl;
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
		return false;			
	}
		
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		cerr<<"Could not set PNG jump value"<<endl;
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return false;
	}
	png_init_io(png_ptr, stdin);
	png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
	    
	bpp = png_get_channels(png_ptr,info_ptr);		
	cerr << "[png2bin]: Converting PNG format to binary output"<<endl;
	cerr << "           bpp: "<<bpp<<"  size: "<< setw(6) << width <<" x" << setw(6) << height<<endl;
		
	int rgbWidth = png_get_rowbytes (png_ptr, info_ptr);		
    png_bytep row_pointer = new png_byte[rgbWidth];				

	for (png_uint_32 j = 0; j < height; j++)
    {
	    png_read_row(png_ptr,row_pointer,NULL);
	 	fwrite(row_pointer, 1, rgbWidth, stdout);
     	if ((j + 1) % 1024 == 0 ) 
        {
			end = clock();
			double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
			cerr << "bin ["<< setw(6) << j + 1 << " rows of " << setw(5) << height <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";
        }   		
	}
	png_read_end(png_ptr, end_info);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return true;	
}
	
