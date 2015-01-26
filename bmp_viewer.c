#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "bmp_header.h"
#include "framebuf.h"

int x_offs = 0;
int y_offs = 0;

static void print_bmp_header(PBITMAPFILEHEADER bf, PBITMAPINFOHEADER bi)
{
	fprintf(stdout, "bfType		= 0x%x\n", bf->bfType); 
	fprintf(stdout, "bfSize		= 0x%x\n", bf->bfSize); 
	fprintf(stdout, "bfOffBits	= 0x%x\n", bf->bfOffBits);

	fprintf(stdout, "biSize		= 0x%x\n", bi->biSize); 
	fprintf(stdout, "biWidth	= 0x%x\n", bi->biWidth);
	fprintf(stdout, "biHeight	= 0x%x\n", bi->biHeight);
	fprintf(stdout, "biBitCount	= 0x%x\n", bi->biBitCount);
	fprintf(stdout, "biSizeImage	= 0x%x\n", bi->biSizeImage);
	fprintf(stdout, "biClrUsed	= 0x%x\n", bi->biClrUsed);
}

static int check_bmp_header(PBITMAPFILEHEADER bf, PBITMAPINFOHEADER bi)
{
	if ( bf->bfType != 0x4D42 ) {	// ascii BM
		fprintf(stderr, "BMP: Invalid bfType 0x%x\n", bf->bfType);
		return -EINVAL;
	}

	if ( bi->biSize != sizeof(BITMAPINFOHEADER) ) {
		fprintf(stderr, "BMP: Invalid biSize %d\n", bi->biSize);
		return -EINVAL;
	}

	if ( bi->biPlanes != 1 ) {
		fprintf(stderr, "BMP: Invalid biPlanes %d\n", bi->biPlanes);
		return -EINVAL;
	}

	if ( bi->biCompression != 0 ) {
		fprintf(stderr, "BMP: Invalid biCompression %d\n", bi->biCompression);
		return -EINVAL;
	}


	switch ( bi->biBitCount ) {
	case 0x01 :		// 2-color bitmap
	case 0x04 :		// 16-color bitmap
	case 0x08 :		// 256-color bitmap
		if ( bi->biClrUsed != (1 << bi->biBitCount) ) {		// palette (color table)
			bi->biClrUsed = (1 << bi->biBitCount);
		}
		break;
	case 0x18 : 		// 24-bit true color
		if ( bi->biClrUsed != 0 ) {		// no palette
			bi->biClrUsed = 0;
		}
		break;
	default :
		fprintf(stderr, "Invalid biBitCount %d\n", bi->biBitCount);
		return -EINVAL;
	}

	fprintf(stdout, "BMP: Width:%d  Height:%d  BPP:%d\n", bi->biWidth, bi->biHeight, bi->biBitCount);
	return 0;
}
  
static int display_bmp(char *bmpfile, struct fb_info *fb_info)
{
	FILE *bmpfd;

	BITMAPFILEHEADER bfile;  
	BITMAPINFOHEADER binfo;

	struct rgb888_sample *rgb888_pixel= NULL;
	unsigned char *bmpdata = NULL;
	unsigned int datasize = 0;
	unsigned int rowsize = 0;
	RGBQUAD *palette = NULL;
	unsigned int palette_size = 0;

	unsigned char byte = 0;
	unsigned char mask = 0;

	unsigned int line, col;
	int i;
	int ret = 0;

	__u8 red;
	__u8 green;
	__u8 blue;

	/*
	 * Open BitMap
	 */
	bmpfd = fopen(bmpfile, "rb"); 
	if (bmpfd == NULL) {
		fprintf(stderr, "BMP: Fail to Open File %s\n", bmpfile);  
		return -ENOENT;
	}

	/*
	 * BitMap Header
	 */
	fread(&bfile,   sizeof(bfile),   1,  bmpfd);  
	fread(&binfo,   sizeof(binfo),   1,  bmpfd); 

	if (check_bmp_header(&bfile, &binfo)) {
		fprintf(stderr, "BMP: Invalid BMP Header\n");
		ret = -EINVAL;

		print_bmp_header(&bfile, &binfo);
		goto out;
	}

	if ( (binfo.biWidth > fb_info->xres) || (binfo.biHeight > fb_info->yres) ) {
		fprintf(stderr, "BMP: Too Large!!!\n");
		ret = -EINVAL;
		goto out;
	}

	/*
 	 * Center
	 */
	x_offs = (fb_info->xres - binfo.biWidth) / 2;
	y_offs = (fb_info->yres - binfo.biHeight) / 2;


	/*
	 * BitMap Palette
	 */
	palette_size = binfo.biClrUsed * sizeof(RGBQUAD);
	if ( palette_size != 0 ) {
		palette = (RGBQUAD *)malloc(palette_size);
		if (!palette) {
			fprintf(stderr, "BMP: Fail to malloc memory for BMP Palette!\n");
			ret = -ENOMEM;
			goto out;
		}

		fread(palette, palette_size, 1, bmpfd);
	}

	/*
	 * BitMap Data
	 *
	 * The bitmap pixels are packed in rows.
	 * The size of each row is rounded up to a multiple of 4 bytes (a 32-bit DWORD) by padding.
	 */
	rowsize = (binfo.biWidth * binfo.biBitCount + 31) / 32 * 4;	// line: 4 byte alignment
	datasize = binfo.biHeight * rowsize;				// should equal to (bfSize - bfOffBits) !!!

	bmpdata = (unsigned char *)malloc(datasize);
	if (!bmpdata) {
		fprintf(stderr, "BMP: Fail to malloc memory for BMP Pixel Array!\n");
		ret = -ENOMEM;
		goto out;
	}

	fread(bmpdata, datasize, 1, bmpfd);

	/*
	 * Display
	 */
	switch (binfo.biBitCount) {
	case 0x01 :		// 2-color bitmap
	case 0x04 :		// 16-color bitmap
	case 0x08 :		// 256-color bitmap
		mask = (1 << binfo.biBitCount) - 1;

		for (line = 0; line < binfo.biHeight; line++) {

			for (col = 0; col < binfo.biWidth; col++) {

				byte = bmpdata[(binfo.biHeight - line) * rowsize + col * binfo.biBitCount / 8];

				for (i = 0; i < (8 / binfo.biBitCount); i++) {

					red   = palette[byte & mask].rgbBlue;//Red;
					green = palette[byte & mask].rgbGreen;
					blue  = palette[byte & mask].rgbRed;//Blue;

					byte >>= binfo.biBitCount;

					fb_set_a_pixel(fb_info, line + y_offs, col + x_offs, red, green, blue, 0);	// R-G-B-Alpha
				}
			}
		}
		break;

	case 0x18 :		// 24-bit true color
		for (line = 0; line < binfo.biHeight; line++) {
	
			for (col = 0; col < binfo.biWidth; col++) {
	
				rgb888_pixel = (struct rgb888_sample *)(bmpdata + (binfo.biHeight - line) * rowsize + col * binfo.biBitCount / 8);
	
				red	= rgb888_pixel->b1;
				green	= rgb888_pixel->b2;
				blue	= rgb888_pixel->b3;
	
				fb_set_a_pixel(fb_info, line + y_offs, col + x_offs, red, green, blue, 0);	// R-G-B-Alpha
	
			}
		}
		break;
	}

out:
	/*
	 * Cleanup
	 */
	fclose(bmpfd);
	if (bmpdata) {
		free(bmpdata);
		bmpdata = NULL;
	}
	if (palette) {
		free(palette);
		palette = NULL;
	}
	return ret;
}

static int init_framebuffer(char *fbdev, struct fb_info *fb_info)
{
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	// Open the file for reading and writing
	fb_info->fd = open(fbdev, O_RDWR);
	if (fb_info->fd < 0) {
		fprintf(stderr, "FB: Cannot open framebuffer device %s (%s)\n", fbdev, strerror(errno));
		return -ENODEV;
	}

	// Get fixed screen information
	if (ioctl(fb_info->fd, FBIOGET_FSCREENINFO, &finfo)) {
		fprintf(stderr, "FB: Cannot read fixed information (%s)\n", strerror(errno));
		return -EINVAL;
	}

	// Get variable screen information
	if (ioctl(fb_info->fd, FBIOGET_VSCREENINFO, &vinfo)) {
		fprintf(stderr, "FB: Cannot read variable information (%s)\n", strerror(errno));
		return -EINVAL;
	}

	// Put variable screen information
	vinfo.activate |= FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW;
	if (ioctl(fb_info->fd, FBIOPUT_VSCREENINFO, &vinfo)) {
		fprintf(stderr, "FB: Cannot write variable information (%s)\n", strerror(errno));
		return -EINVAL;
	}

	// Figure out the size of the screen in bytes
	fb_info->xres = vinfo.xres;
	fb_info->yres = vinfo.yres;
	fb_info->bpp = vinfo.bits_per_pixel;
	fb_info->smem_len = finfo.smem_len;

	// Map the device to memory
	fb_info->fb_mem = (__u8 *)mmap(0,
				       finfo.smem_len,
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED,
				       fb_info->fd,
				       0);
	if (!fb_info->fb_mem) {
		fprintf(stderr, "FB: Fail to mmap framebuffer device to memory (%s)\n", strerror(errno));
		return -EINVAL;
	}

	fprintf(stdout, "FB: Width:%d  Height:%d  BPP:%d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
	return 0;
}

void finish_framebuffer(struct fb_info *fb_info)
{
	if (fb_info->fb_mem)
		munmap(fb_info->fb_mem, fb_info->smem_len);
	fb_info->fb_mem = NULL;
	if (fb_info->fd > 0)
		close(fb_info->fd);
	fb_info->fd = -1;
}

int main(int argc ,char ** argv)
{
	struct fb_info fbinfo;

	char *bmpfile = NULL;
	char *fbdev = NULL;

 	int ret = 0;

	if (argc != 3) {
		puts("BMP Viewer");
		puts("Usage: ");
		puts("\tbmpviewer  <fb node>  <bmp file>\n"); 
		return  -1 ;
	}

	fbdev = argv[1];
	bmpfile = argv[2];

	printf("%s <%s> <%s>\n", argv[0], fbdev, bmpfile);

	ret = init_framebuffer(fbdev, &fbinfo);
	if (ret != 0) {
		fprintf(stderr, "Quit ...\n");
		goto error;
	}

	display_bmp(bmpfile, &fbinfo);

error:
	finish_framebuffer(&fbinfo);
	return ret;
}
