#pragma once

#define HLS_NO_XIL_FPO_LIB
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_video_mem.h"

typedef unsigned short ushort;

struct cXY {
	unsigned short x;
	unsigned short y;

	cXY() { x=0; y=0; };

	void update(unsigned short width, bool valid=true) 
	{
		if(valid) {
			if (x==(width-1)) {
				x = 0;
				y++;
			} else {
				x++;
			}		
		}
	}
};


template<unsigned MAX_LINE_SIZE, unsigned KERNEL_H_SIZE, unsigned KERNEL_V_SIZE, typename T>
struct Window2D {
	Window2D(ushort width, ushort height) {
		mWidth  = width;	
		mHeight = height;
		mNumPix = width*height;
		mValid  = false;
		mCount  = 0;
		mDone   = false;
	};

	void next(hls::stream<T> &img_i) {
		#pragma HLS INLINE		
		T C[KERNEL_V_SIZE];
		T mPix = (mCount<mNumPix) ? img_i.read() : 0;
		mLineBuffer.get_col(C,mSrcXY.x); C[KERNEL_V_SIZE-1] = mPix;
		mLineBuffer.shift_pixels_up(mSrcXY.x);
		mLineBuffer.insert_bottom_row(mPix, mSrcXY.x);
		mWindowIn.shift_pixels_left();
		mWindowIn.insert_right_col(C);
		mSrcXY.update(mWidth);		
		mDstXY.update(mWidth, mValid);		

		// Clamp pixels to 0 when outside of image 
	    for(int row=0; row<KERNEL_V_SIZE; row++) {
	      for(int col=0; col<KERNEL_H_SIZE; col++) {
	        int xoffset = (mDstXY.x+col-(KERNEL_H_SIZE/2));
	        int yoffset = (mDstXY.y+row-(KERNEL_V_SIZE/2));
	        if ( (xoffset<0) || (xoffset>=mWidth) || (yoffset<0) || (yoffset>=mHeight) ) {
	          mWindow.insert_pixel(0, row, col);
	        } else {
	          mWindow.insert_pixel(mWindowIn.val[row][col], row, col);
	        } 
	      }
	    }

		mValid = (mCount>(mWidth*(KERNEL_V_SIZE/2)+(KERNEL_H_SIZE/2)-1));
		mDone  = (mDstXY.x==(mWidth-1)) && (mDstXY.y==(mHeight-1));
		mCount++;
	};

	T operator () (int row, int col) {
		#pragma HLS inline	
		return mWindow(row,col);
	}

	bool done() {
		return mDone;
	}

	bool valid() {
		return mValid;
	}

	hls::Window<KERNEL_V_SIZE, KERNEL_H_SIZE, T> mWindow;
	hls::Window<KERNEL_V_SIZE, KERNEL_H_SIZE, T> mWindowIn;
	hls::LineBuffer<KERNEL_V_SIZE-1, MAX_LINE_SIZE, T> mLineBuffer;	

private:
	cXY       mSrcXY;
	cXY       mDstXY;
	ushort    mWidth;
	ushort    mHeight;
	unsigned  mNumPix;
	unsigned  mCount;
	bool 	  mValid;
	bool      mDone;
};
