CXX = g++
CXXFLAGS = -O3 -fno-strict-aliasing -Wall -D_LINUX 
VPATH = ../Linux_PC.bin
INSTALL = /usr/bin/install
LIBPATH = ../lib

# tools will be installed into
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
libdir = $(exec_prefix)/lib

programs = tx2pow2 tx2half specmap tx2rgba  # txnopinch

all:	$(programs) bin2png png2bin txtiles txtilesDXT # txtiles16DXT

$(programs):	%: %.cpp 
	$(CXX) $(CXXFLAGS) $< -o $(VPATH)/$@
		
bin2png: bin2png.cpp
	$(CXX) $(CXXFLAGS) bin2png.cpp -o $(VPATH)/bin2png -lpng

png2bin: png2bin.cpp	
	$(CXX) $(CXXFLAGS) png2bin.cpp -o $(VPATH)/png2bin -lpng	

txtiles: txtiles.cpp
	$(CXX) $(CXXFLAGS) txtiles.cpp -o $(VPATH)/txtiles -lpng

txtilesDXT: txtilesDXT.cpp
	$(CXX) $(CXXFLAGS) -mfpmath=sse -march=pentium4 -msse2 txtilesDXT.cpp -o $(VPATH)/txtilesDXT -L $(LIBPATH) -lnvtt -lcudart

#txtiles16DXT: txtilesDXT.cpp
#	$(CXX) $(CXXFLAGS) -mfpmath=sse -march=pentium4 -msse2 txtiles16DXT.cpp -o $(VPATH)/txtiles16DXT -L $(LIBPATH) -lnvtt -lcudart

	
clean:	
	rm -f $(VPATH)/*  *.o *.*~ *~
install:
	$(INSTALL) $(VPATH)/tx2pow2 $(VPATH)/tx2half $(VPATH)/specmap $(VPATH)/tx2rgba $(VPATH)/bin2png $(VPATH)/png2bin $(VPATH)/txtiles $(VPATH)/txtilesDXT $(bindir)
         
        # $(VPATH)/txtiles16DXT $(bindir) $(VPATH)/txnopinch
	$(INSTALL) $(LIBPATH)/libnvtt.so $(libdir)
	rm -f $(libdir)/libcudart.so*	
	$(INSTALL) $(LIBPATH)/libcudart.so.2.0  $(libdir) 
	ln -s 	$(libdir)/libcudart.so.2.0 $(libdir)/libcudart.so
	ln -s 	$(libdir)/libcudart.so.2.0 $(libdir)/libcudart.so.2
	/sbin/ldconfig
	
uninstall:
	rm -f $(bindir)/tx2pow2  $(bindir)/tx2half  $(bindir)/specmap $(bindir)/tx2rgba $(bindir)/bin2png $(bindir)/png2bin $(bindir)/txtiles $(bindir)/txtilesDXT $(libdir)/libnvtt.so $(libdir)/libcudart.so*
         #$(bindir)/txtiles16DXT $(bindir)/txnopinch
      