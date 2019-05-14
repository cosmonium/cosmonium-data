//
// Author: Fridger Schrempp
//
// fridger.schrempp@desy.de
//
// The program reads an elevation map in signed 16 bit integer
// raw format from STDIN and the BMNG watermask of same width in 8 bit
// unsigned raw integer format from the command line. In the heightmap, 
// oceans are found from 'elevation == 0'. The result is bitwise OR-ed with 
// (the complement of) the watermask to improve the precision of the ocean 
// boundaries of the latter. The resulting specmap is output at STDOUT as a 
// graymap in 8 bit unsigned integer raw format.       

// Specify byteswap = 1 if the byte ordering 
// of input file and computer differ


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


int main(int argc, char* argv[])
{
    int width   = 0;
    int height  = 0;
    float light = 0.0;
	string filename; 
	vector<string> pcOrder(2);
	
	pcOrder[0] = "Big-endian (MAC...)"; 
	pcOrder[1] = "Little-endian (Intel)";
	
	if (argc < 3 || argc > 5)     
    {
        cerr << "\nUsage: specmap <width> <watermask name> [<light> [<byteswap>]]\n\n";
        cerr << "Version "<< version << endl;
        cerr << "------------------------------------------------------------------------\n";
        cerr << "The program reads an elevation map in signed 16 bit integer raw format \n";
        cerr << "from STDIN and the BMNG watermask of same width in 8 bit unsigned \n";
        cerr << "integer raw format via the command line. In the heightmap, oceans are\n"; 
		cerr << "found from 'elevation == 0'. The result is bitwise OR-ed with (the\n"; 
        cerr << "complement of) the watermask to improve the precision of the ocean\n"; 
        cerr << "boundaries of the latter. The resulting specmap is output at STDOUT\n"; 
		cerr << "as a graymap in 8 bit unsigned integer raw format.\n";       
        cerr << "------------------------------------------------------------------------\n\n"; 
		cerr << "Assume : height = width / 2.\n\n";
        cerr << "Default: Byte ordering of input heightmap and computer are equal,\n";
        cerr << "         else specify <byteswap> = 1.\n";
        cerr << "         The light fraction of the land defaults to 0.0 (black),\n";
        cerr << "         and may be increased by specifying  <light> (< 1).\n";
		cerr << "         A recommended value is light = 0.12. \n\n"; 
        cerr <<"Computer and output to STDOUT have **"<<pcOrder[IsLittleEndian()]<<"** byte order!\n\n";  
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
    		if (SSCANF(argv[3], " %f", &light) != 1)
    		{
    			cerr << "Bad spec level.\n";
        		return 1;
    		}
    		    
            if (argc > 4)
    		{
    			if (SSCANF(argv[4], " %d", &byteSwap) != 1)
    			{
    				cerr << "Bad byteorder specs.\n";
        			return 1;
    			}
    		}
       	}
    }		
    cerr << "\n\nResidual light fraction of landmap: "<< light<<endl;
    cerr <<endl;
        
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
    
    ifstream file (filename.c_str(), ios::in|ios::binary);
    
    height = width / 2;    
    cerr << "\n[specbin]: Improving the watermap from height=0: " << setw(6) << width <<" x" << setw(6) << height << "\n";    
    short* h = new short [width];
    unsigned char* hspec  = new unsigned char [width];
    
    if (file.is_open())
    {
	    char* hwater = new char[width];    

		clock_t start = clock(), end;
    	      	
    	for (int j = 0; j < height; j++)
    	{
			readRowS16(stdin, h, width);
	     	file.read(hwater, width);
     		 
       		for (int i = 0; i < width; i++)
     		{  
				hspec[i] = 0;
     	    	if (h[i] == 0||(j > 0.83252f * height && h[i] <= 40))
         			hspec[i] = 255;
         		   	
       	    	// Perform bitwise OR with the complement of the watermap
            	// to strongly improve the watermask's ocean boundaries! 
                  	    
       	    	hspec[i] |= ~ (unsigned char) hwater[i];
       	    	
       	    	// Optional linear brightening of dark land: 
       	    	// black = 0.0  -> light < 1.0, leaving white water untouched
       	    	hspec[i] =  ~(unsigned char)((unsigned char)(~hspec[i]) * (1.0f - light));  
      	 	}        	
     		fwrite(hspec, 1, width,stdout);
			if ((j + 1) % 1024 == 0 ) 
			{
				end = clock();
				double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
				cerr << "spec["<< setw(6) << j + 1 << " rows of " << setw(5) << height <<" -> " << setw(6) << fixed << setprecision(2) << duration << " s] \n";
			}   
		}
    	file.close();
		return true;
    }
    else
        cerr << "Unable to open file \'" << filename.c_str()<<"\'"<< endl;

}

