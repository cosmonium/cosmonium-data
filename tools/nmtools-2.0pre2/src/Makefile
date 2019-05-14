CXX = g++
CXXFLAGS = -O3 -Wall -D_LINUX
VPATH = ../Linux_PC.bin
INSTALL = /usr/bin/install
LIBPATH = ../lib

# tools will be installed into
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
libdir = $(exec_prefix)/lib

programs = nm2pow2 nm2half 

all:	$(programs) libnvtt.so nms nmtiles nmtilesDXT 

$(programs):	%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $(VPATH)/$@
		
nms nmtiles:	%: %.cpp 
	$(CXX) $(CXXFLAGS) $< -o $(VPATH)/$@ -lpng

nmtilesDXT:		%: %.cpp 
	$(CXX) $(CXXFLAGS) -mfpmath=sse -march=pentium4 -msse2 $< -o $(VPATH)/$@ -L $(VPATH) -lnvtt

libnvtt.so: $(LIBPATH)/libnvtt.so
	/bin/cp -p $< $(VPATH)

clean:	
	rm -f   $(VPATH)/* *.o *.*~ *~ 

install:
	$(INSTALL) $(VPATH)/nm2pow2 $(VPATH)/nm2half $(VPATH)/nms $(VPATH)/nmtiles $(VPATH)/nmtilesDXT $(bindir)
	$(INSTALL) $(VPATH)/libnvtt.so $(libdir)
	/sbin/ldconfig
	
uninstall:
	rm -f $(bindir)/nm2pow2 $(bindir)/nm2half $(bindir)/nms $(bindir)/nmtiles	$(bindir)/nmtilesDXT $(libdir)/libnvtt.so