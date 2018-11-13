
#include "filter2d.h"
#include "window2d.h"

void Filter2D(
        const    short coeffs[FILTER2D_KERNEL_V_SIZE][FILTER2D_KERNEL_H_SIZE],
		unsigned char *srcImg,
		unsigned int   width,
		unsigned int   height,
		unsigned int   stride,
		unsigned char *dstImg )
{

	Window2D<FILTER2D_KERNEL_H_SIZE, FILTER2D_KERNEL_V_SIZE, unsigned char> pixelWindow(width, height, stride);

    for(int y=0; y<height; ++y)
    {
        for(int x=0; x<width; ++x)
        {
        	// Add a new pixel to the linebuffer, generate a new pixel window
			pixelWindow.next(srcImg, x, y);

        	// Apply 2D filter to the pixel window
			int sum = 0;
			for(int row=0; row<FILTER2D_KERNEL_V_SIZE; row++)
			{
				for(int col=0; col<FILTER2D_KERNEL_H_SIZE; col++)
				{
					sum += pixelWindow(row,col)*coeffs[row][col];
				}
			}
			
        	// Normalize result
			unsigned char outpix = sum/(FILTER2D_KERNEL_V_SIZE*FILTER2D_KERNEL_H_SIZE);

			// Write output
           	dstImg[y*stride+x] = outpix;
        }
    }
}



