doppler: doppler.cpp
	g++ doppler.cpp -o doppler -Wl,-rpath,/usr/local/lib -lbb_api
	
run: doppler
	./doppler