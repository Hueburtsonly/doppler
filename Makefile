doppler: doppler.cpp usbtg.cpp usbtg.h
	g++ doppler.cpp usbtg.cpp -o doppler -Wl,-rpath,/usr/local/lib -lbb_api -lftdi -lcurses

run: doppler
	./doppler

rungui: doppler gui/gui.class
	./doppler | java -cp gui Gui

gui/gui.class: gui/Gui.java
	cd gui && javac Gui.java
