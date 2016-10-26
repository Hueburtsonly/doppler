doppler: doppler.cpp usbtg.cpp usbtg.h
	g++ doppler.cpp usbtg.cpp -o doppler -Wl,-rpath,/usr/local/lib -lbb_api -lftdi -lcurses

run: doppler
	./doppler

rungui: doppler gui/gui
	./doppler | gui/gui

gui/gui: qui/Makefile .PHONY
	cd gui && make

gui/Makefile: gui/gui.pro
	cd gui && qmake

.PHONY:
	