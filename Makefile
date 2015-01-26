
bmpviewer: bmp_viewer.c 
	arm-none-linux-gnueabi-gcc -static bmp_viewer.c -o bmpviewer

clean:
	rm  bmpviewer
