#include "filter2d.h"
#include "window2d.h"


void Filter2D(
		const ap_uint<AXIMM_DATA_WIDTH>   *srcCoeffs, 
		STREAM_PIXELS& srcImg,
		U16            width,
		U16            height,
		STREAM_PIXELS& dstImg)
{
    // Filtering 2D window
    Window2D<MAX_WIDTH, FILTER_KERNEL_V_SIZE, FILTER_KERNEL_H_SIZE, U8> pixelWindow(width, height);
    #pragma HLS DEPENDENCE variable=pixelWindow.mLineBuffer inter false
    #pragma HLS DEPENDENCE variable=pixelWindow.mLineBuffer intra false

    // Filtering coefficients
    short coeffs[FILTER_KERNEL_V_SIZE][FILTER_KERNEL_H_SIZE];
    #pragma HLS ARRAY_PARTITION variable=coeffs complete dim=0

    // Burst copy the coefficients from global memory to local memory
    readcoeffs(srcCoeffs, coeffs);

    // Iterate until all pixels have been processed
    filter: while (! pixelWindow.done() ) {
        #pragma HLS PIPELINE II=1

        // Add a new pixel to the linebuffer, generate a new pixel window
        pixelWindow.next(srcImg);

        // Apply 2D filter to the pixel window
        int sum = 0;
        for(int row=0; row<FILTER_KERNEL_V_SIZE; row++) 
        {
            for(int col=0; col<FILTER_KERNEL_H_SIZE; col++) 
            {
                sum += pixelWindow(row,col)*coeffs[row][col];
            }
        }

        // Normalize result
        U8 outpix = sum/(FILTER_KERNEL_V_SIZE*FILTER_KERNEL_H_SIZE);

        // Take care of run-in effect, write output only when the window is valid
        // i.e. if kernel is VxH need at least V/2 rows and H/2 pixels before generating output
        if (pixelWindow.valid()) {
            dstImg << outpix;
        }
    }
}


extern "C" {

void Filter2DKernel(
        const ap_uint<AXIMM_DATA_WIDTH>* coeffs,
		const ap_uint<AXIMM_DATA_WIDTH>* src,
		unsigned int width,
		unsigned int height,
		unsigned int stride,
		ap_uint<AXIMM_DATA_WIDTH>* dst)
  {
    #pragma HLS INTERFACE m_axi     port=src    offset=slave bundle=port0    max_read_burst_length=256 max_write_burst_length=256 
    #pragma HLS INTERFACE s_axilite port=src                 bundle=control
    #pragma HLS INTERFACE s_axilite port=width               bundle=control
    #pragma HLS INTERFACE s_axilite port=height              bundle=control
    #pragma HLS INTERFACE s_axilite port=stride              bundle=control
    #pragma HLS INTERFACE m_axi     port=coeffs offset=slave bundle=port1    max_read_burst_length=256 max_write_burst_length=256
    #pragma HLS INTERFACE s_axilite port=coeffs              bundle=control
    #pragma HLS INTERFACE m_axi     port=dst    offset=slave bundle=port1    max_read_burst_length=256 max_write_burst_length=256
    #pragma HLS INTERFACE s_axilite port=dst                 bundle=control
    #pragma HLS INTERFACE s_axilite port=return              bundle=control

#ifndef __SYNTHESIS__
	assert(width  <= 1920);
	assert(height <= 1080);
    assert(stride%64 == 0);
#endif
            
	#pragma HLS DATAFLOW

	// Stream of pixels from kernel input to filter, and from filter to output
	hls::stream<U8> src_pixels;
	hls::stream<U8> dst_pixels;
	#pragma HLS stream variable=src_pixels depth=64
	#pragma HLS stream variable=dst_pixels depth=64

	// Read image data from global memory over AXI4 MM, and stream pixels out
	AXIBursts2PixelStream((AXIMM)src, width, height, stride, src_pixels);

	// Process incoming stream of pixels, and stream pixels out
	Filter2D(coeffs, src_pixels, width, height, dst_pixels);

	// Write incoming stream of pixels and write them to global memory over AXI4 MM
	PixelStream2AXIBursts(dst_pixels, width, height, stride, (AXIMM)dst);

  }

}
