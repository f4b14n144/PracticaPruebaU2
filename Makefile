

all:
	g++ -std=c++17 Principal.cpp -I/usr/include/opencv5/ -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -o vision.bin


run:
	./vision.bin

clean:
	rm -f vision.bin
