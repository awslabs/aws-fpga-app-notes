#include "filter2d.h"

void readcoeffs(const ap_uint<AXIMM_DATA_WIDTH> *srcCoeffs, short coeffs[15][15])
{
    #pragma HLS INLINE 

    const unsigned NUM_512BIT_WORDS = ((FILTER_KERNEL_V_SIZE * FILTER_KERNEL_H_SIZE)*sizeof(short)+63)/64;

    ap_uint<AXIMM_DATA_WIDTH> tmp[NUM_512BIT_WORDS];    
    #pragma HLS ARRAY_PARTITION variable=tmp complete dim=0

    // Burst the coefficient into a temp array of 512-bit words
    rd_coeffs: for (int i=0; i<NUM_512BIT_WORDS; i++) 
    {
        #pragma HLS PIPELINE II=1
        tmp[i] = srcCoeffs[i];
    }

    // Remap the temp array of 512-bit words to an array of short[V][H]
    int idx = 0;
    int adr = 0;
    for(int i=0; i<FILTER_KERNEL_V_SIZE; i++) 
    {
        #pragma HLS UNROLL
        for(int j=0; j<FILTER_KERNEL_H_SIZE; j++) 
        {
            #pragma HLS UNROLL
            ap_uint<AXIMM_DATA_WIDTH> word = tmp[adr];
            coeffs[i][j] = word((16*(idx+1))-1, 16*idx);

            if (idx==31) {
                idx=0;
                adr++;
            } else {
                idx++;
            }
        }
    }
}