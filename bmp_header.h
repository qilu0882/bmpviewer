
#ifndef _WINGDI_
#define _WINGDI_

//#define OS_64

#ifdef OS_64
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned int	DWORD;
typedef unsigned int	LONG;
#else
typedef unsigned char	BYTE;		// 1-byte
typedef unsigned short	WORD;		// 2-byte
typedef unsigned long	DWORD;		// 4-byte
typedef unsigned long	LONG;		// 4-byte
#endif

#pragma pack(2)
typedef struct tagBITMAPFILEHEADER {		// 14 Bytes
	WORD    bfType;			// (offset 0x00) 0x424D (ASCII BM - Windows 3.1x, 95, NT, ... etc.)
	DWORD   bfSize;			// (offset 0x02) the size of the BMP file in bytes
	WORD    bfReserved1;		// (offset 0x06) reserved
	WORD    bfReserved2;		// (offset 0x08) reserved
	DWORD   bfOffBits;		// (offset 0x0A) the offset where the bitmap image data(pixel array) can be found
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {		// 40 Bytes (Windows V3 - Windows 3.0 or later)
	DWORD   biSize;			// (offset 0x0E) the size of this header (must be 40 bytes)
	LONG    biWidth;		// (offset 0x12) the bitmap width in pixels (signed integer)
	LONG    biHeight;		// (offset 0x16) the bitmap height in pixels (signed integer)
	WORD    biPlanes;		// (offset 0x1A) the number of color planes (must be 1)
	WORD    biBitCount;		// (offset 0x1C) the number of bits per pixel (the color depth of the image, typical value are 1,4,8,16,24 and 32)
	DWORD   biCompression;		// (offset 0x1E) the compression method, shoud be 0(none)
	DWORD   biSizeImage;		// (offset 0x22) the size of raw bitmap data
	LONG    biXPelsPerMeter;	// (offset 0x26) the horizontal resolution (pixel per meter, signed integer)
	LONG    biYPelsPerMeter;	// (offset 0x2A) the vertical resolution (pixel per meter, signed integer)
	DWORD   biClrUsed;		// (offset 0x2E) the number of colors in the color palette, or 0 to default to 2^n
	DWORD   biClrImportant;		// (offset 0x32) the number of important colors used, or 0 when every color is important; generally ignored
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {		// Palette
	BYTE	rgbBlue;
	BYTE	rgbGreen;
	BYTE	rgbRed;
	BYTE	rgbReserved;
} RGBQUAD;
#pragma pack()

struct bmp_info {
//	PBITMAPFILEHEADER bf;
//	PBITMAPINFOHEADER bi;

	unsigned int width;
	unsigned int height;
	unsigned int bpp;

	/*
	 * The color table (palette) occurs in the BMP image file directly after the BMP file header, the BMP info header. Therefore, 
	 * its offset is the size of the BITMAPFILEHEADER plus the size of the BITMAPINFOHEADER.
	 * The color table is normally not used when the pixels are in the 16-bit per pixel format (and higher); there are normally no 
	 * color table in those bitmap image files.
	 * The number of entries in the palette is 2^biBitCount. In most cases, each entry in the color table occupies 4 bytes, in the 
	 * order blue, green, red, 0x00. (The colors in the color table are usually specified in the 4-byte per entry RGBA32 format.)
	 * The color table is a block of bytes (a table) listing the colors used by the image. Each pixel in an bitmap image is described 
	 * by a number of bits (1, 4, or 8) which is an index of a single color described by this table.
	 */
	unsigned char *palette;
	unsigned int palette_size;

	/*
	 * The bits representing the bitmap pixels are packed in rows. The size of each row is rounded up to a multiple of 4 bytes (a 32-bit
	 * DWORD) by padding.
	 *
	 * RowSize = (BitsPerPixel * ImageWidth + 31) / 32 * 4; (ImageWidth is expressed in pixels)
	 * PixelArraySize = RowSize * |ImageHeight|; (The absolute value is necessary because ImageHeight can be negative ?)
	 */
	unsigned int rowsize;		// the number of bytes to store one row of pixels
	unsigned int pixelarraysize;	// the number of bytes to store an array of pixels

	/*
	 * The pixel array is a block of 32-bit DWORDs, that describes the image pixel by pixel. Normally pixels are stored "upside-down" with
	 * respect to normal image raster scan order, starting in the lower left corner, going from left to right, and then row by row from the
	 * bottom to the top of the image.
	 */
	unsigned char *bmpdata;		// bitmap data (Pixel Array)
};

#endif /* _WINGDI_ */
