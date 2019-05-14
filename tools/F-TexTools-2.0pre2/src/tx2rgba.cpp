//
// Author: Fridger Schrempp
//
// fridger.schrempp@desy.de
//
// The program reads a RGB texture map in 3 x 8 bit integer raw format from STDIN and 
// a specular graymap of same dimensions in 1 x 8 bit. The specmap is mounted as the alpha channel of
// the resulting RGBA map. The latter is output at STDOUT in 4 x 8 bit unsigned integer raw format. 
//

#include <iostream>
#include <fstream>
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

using namespace std;

const string version = "1.0, August 2007, author: F. Schrempp\n";

int main(int argc, char* argv[])
{
    int width   = 0;
    int height  = 0;
	int bpp     = 3;
	string filename; 
	
	if (argc < 3 || argc > 4)     
    {
        cerr << "\nUsage: tx2rgba <width> <specmap name> [<height>]\n\n";
        cerr << "Version "<< version << endl;
        cerr << "------------------------------------------------------------------\n";
        cerr << "The program reads a RGB map in 3 x 8 bit integer raw format from STDIN \n";
        cerr << "and a specular graymap of same dimensions in 1 x 8 bit raw format.\n";
        cerr << "The specmap is mounted as the alpha channel of the resulting RGBA map\n"; 
        cerr << "The latter is output at STDOUT in 4 x 8 bit unsigned integer raw format.\n\n"; 
        cerr << "------------------------------------------------------------------\n\n";
		cerr << "Default : height = width / 2.\n\n";
		return 1;
    }
    else
    {
    	if (SSCANF(argv[1], " %d", &width) != 1)
    	{
    		cerr << "Bad image dimensions.\n";
        	return 1;
		}
    	if( filename.empty())
			filename.assign(argv[2]);
    	else
    	{
    		cerr << "Bad filename.\n";
        	return 1;
    	}
    	if (argc > 3)
    	{
    		if (SSCANF(argv[3], " %d", &height) != 1)
    		{
    			cerr << "Bad height specification.\n";
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
        
    if (height == 0)
		height = width / 2;    

	int rgbWidth  = bpp * width; 
	int	rgbaWidth = rgbWidth + width;
    
	cerr << "\n[tx2rgba]: Mounting a 1x8 bit specular graymap"<<endl; 
	cerr << "           above  the 3x8 bit RGB base texture"<<endl; 
	cerr << "           => 4x8 bit RGBA image" << endl;
	
    unsigned char* rgb = new unsigned char  [rgbWidth];
	unsigned char* rgba = new unsigned char [rgbaWidth];
	
    ifstream file (filename.c_str(), ios::in|ios::binary);
    if (file.is_open())
    {
	    char* spectex = new char[width];    
    	

		// Conversion RGB -> RGBA, bpp: 3 -> 4;
		
		clock_t start = clock(), end;

    	for (int j = 0; j < height; j++)
    	{
            if (fread(rgb, 1, rgbWidth, stdin) != (size_t) rgbWidth)
			{
				cerr<<"Read error in RGB file at STDIN!";
				return false;
	     	}
			file.read(spectex, width);
     		 
       		for (int i = 0; i < width; i++)
			{
				int irgba = i * (bpp + 1); 
				for (int col = 0; col < bpp; col++)
					*(rgba + irgba + col) = *(rgb + i * bpp + col);
				*(rgba + irgba + bpp) = *(spectex + i);
				
			}        	
     		fwrite(rgba, 1, rgbaWidth, stdout);
			if ((j + 1) % 1024 == 0 ) 
			{
				end = clock();
				double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
				cerr << "rgba["<< setw(6) << j + 1 << " rows of "<< setw(5) << height <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";
			}   
		}
    	file.close();
		return true;
    }
    else
	{
        cerr << "Unable to open file \'" << filename.c_str()<<"\'"<< endl;
		return false;
	}	
}

