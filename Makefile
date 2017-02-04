doppler: doppler.cpp usbtg.cpp usbtg.h
	g++ doppler.cpp usbtg.cpp -o doppler -Wl,-rpath,/usr/local/lib -lbb_api -lftdi -lcurses

run: doppler
	./doppler

rungui: doppler gui/gui.class
	./doppler | java -cp gui Gui

gui/gui.class: gui/Gui.java
	cd gui && javac Gui.java

spectrum: spectrum.cpp spectrum_cmd.cpp spectrum_hw.cpp images/monospace.h
	g++ spectrum.cpp -o spectrum -Wl,-rpath,/usr/local/lib -lbb_api -lglut -lGL -lGLEW -lGLU -lpthread

runspectrum: spectrum
	./spectrum


images/process: images/process.cpp
	g++ images/process.cpp -o images/process -lpng

images/monospace.h: images/monospace.png images/process
	images/process images/monospace.png images/monospace.h

