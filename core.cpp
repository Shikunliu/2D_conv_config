#include "core.h"

void doImgproc(hls::stream<uint_8_side_channel> &inStream, hls::stream<uint_8_side_channel> &inStream2, hls::stream<int_8_side_channel> &outStream, char kernel[KERNEL_DIM*KERNEL_DIM],int operation, int mode, int addition, int stride, int pooling)
{
	#pragma HLS INTERFACE axis port=inStream
	#pragma HLS INTERFACE axis port=inStream2
	#pragma HLS INTERFACE axis port=outStream
	#pragma HLS INTERFACE s_axilite port=kernel bundle=KERNEL_BUS
	#pragma HLS INTERFACE s_axilite port=operation bundle=CRTL_BUS
	#pragma HLS INTERFACE s_axilite port=mode bundle=CRTL_BUS
	#pragma HLS INTERFACE s_axilite port=addition bundle=CRTL_BUS
	#pragma HLS INTERFACE s_axilite port=stride bundle=CRTL_BUS
	#pragma HLS INTERFACE s_axilite port=pooling bundle=CRTL_BUS
	#pragma HLS INTERFACE s_axilite port=return bundle=CRTL_BUS

	//Define the line buffer
	hls::LineBuffer<KERNEL_DIM, IMG_WIDTH_OR_COLS, unsigned char> lineBuff;  //Define the size of line buffer £¨3  of linebuffer which size of 240£©
	hls::Window<KERNEL_DIM,KERNEL_DIM,short> window;  //Define the size of the window

	hls::LineBuffer<KERNEL_DIM, IMG_WIDTH_OR_COLS, unsigned char> lineBuff2;  //Define the size of line buffer £¨3  of linebuffer which size of 240£©
	hls::Window<KERNEL_DIM,KERNEL_DIM,short> window2;  //Define the size of the window

	//Index used to keep track of row, col
	int idxCol = 0;
	int idxRow = 0;
	int pixConvolved = 0;
	int flag = 0; //flag to indicate that it arrived the last pixel that should output, in case pixConvolued has be cleared to 0
	int count_kernel = 0;
	int output_flag = 0;
	/*
	//For measure latency
	operation = 2;
	mode = 4; //When addition is 1, mode should be 11
	stride = 4;
	addition = 0;
	pooling = 1;
	*/
	//mode = 3;
	//stride = 1;

	//Calculate delay to fix line-buffer offset
	int waitTicks = (IMG_WIDTH_OR_COLS*(mode-1)+mode)-1;
	if (pooling != 0)
	{
		mode = stride;
		operation = 2;
	}

	if (mode == 2)//Special case for pooling = 2, mode = 2
		waitTicks = (IMG_WIDTH_OR_COLS*2+3)/2;

	int countWait = 0;
	int sentPixels = 0;


	int_8_side_channel dataOutSideChannel;
	uint_8_side_channel currPixelSideChannel;

	int_8_side_channel dataOutSideChannel2;
	uint_8_side_channel currPixelSideChannel2;

		for(int idxPixel = 0; idxPixel < (IMG_WIDTH_OR_COLS*IMG_HEIGHT_OR_ROWS); idxPixel++) // Sweep all the pixels
			{
				#pragma HLS PIPELINE II=1
				//Read and cache
				currPixelSideChannel = inStream.read();
				//Get the pixel data
				unsigned char pixelIn = currPixelSideChannel.data;


				currPixelSideChannel2 = inStream2.read();
				//Get the pixel data
				unsigned char pixelIn2 = currPixelSideChannel2.data;

				//Put data on the Linebuffer
				lineBuff.shift_up(idxCol);
				lineBuff.insert_top(pixelIn,idxCol);

				lineBuff2.shift_up(idxCol);
				lineBuff2.insert_top(pixelIn2,idxCol);

				if (addition == 1)
				{
					dataOutSideChannel.data = pixelIn+pixelIn2;
					dataOutSideChannel.keep = currPixelSideChannel.keep;
					dataOutSideChannel.strb = currPixelSideChannel.strb;
					dataOutSideChannel.user = currPixelSideChannel.user;
					dataOutSideChannel.last = 0;
					dataOutSideChannel.id = currPixelSideChannel.id;
					dataOutSideChannel.dest = currPixelSideChannel.dest;

					//Send to the stream (Block if the FIFO receiver is full)
					outStream.write(dataOutSideChannel);
					//sentPixels++;
				}
				else if (addition != 1)
				{
					//int idx_stride = idxPixel+stride-mode+1;
					int idx_stride = idxPixel - (IMG_WIDTH_OR_COLS*(mode-1)+mode-1);
					int modcol = 0;
					int modrow = 0;



					modcol = idx_stride%stride;
					modrow = idx_stride%(IMG_WIDTH_OR_COLS*stride);


					//Put data on the window and multiply with kernel, in this case, only 9 times multiplication, finish one time of window x kernel
					doImgproc_label1:for(int idxWinRow = 0; idxWinRow < 11; idxWinRow++)
					{
						if (idxWinRow < mode)//ug902 P359
						{
							doImgproc_label3:for (int idxWinCol = 0; idxWinCol < 11; idxWinCol++)
							{
								if (idxWinCol<mode)
								{
									//idxWinCol + pixConvolved, will slide the window ...
									short val = (short)lineBuff.getval(idxWinRow+(KERNEL_DIM-1)-(mode-1), idxWinCol+pixConvolved); //Get value at specific position

									if (pooling == 0)
									{
										//Multiply kernel by the sampling window
										val = (short)kernel[count_kernel]*val;  //Kernel x value in
										count_kernel++;
									}
									window.insert(val, idxWinRow, idxWinCol); //Save back to the corresponding position, now the window is the product of kernel and the same size (3 x 3 window)

									if (mode != 11 && pooling == 0)
									{
										short val = (short)lineBuff2.getval(idxWinRow+(KERNEL_DIM-1)-(mode-1), idxWinCol+pixConvolved);
										val = (short)kernel[count_kernel+mode*mode]*val;  //Kernel x value in
										//count_kernel++;
										window2.insert(val, idxWinRow, idxWinCol);
									}

								}
							}
						}
					}

					count_kernel = 0;

/*

						doImgproc_label2:for(int idxWinRow = 0; idxWinRow < KERNEL_DIM; idxWinRow++)
						{
							if (idxWinRow<mode)
							{
								doImgproc_label4:for (int idxWinCol = 0; idxWinCol < KERNEL_DIM; idxWinCol++)
								{
									if (idxWinCol<mode)
									{
										//idxWinCol + pixConvolved, will slide the window ...
										short val = (short)lineBuff2.getval(idxWinRow+(KERNEL_DIM-1)-(mode-1), idxWinCol+pixConvolved); //Get value at specific position

										//Multiply kernel by the sampling window
										val = (short)kernel[count_kernel]*val;  //Kernel x value in
										count_kernel++;
										window2.insert(val, idxWinRow, idxWinCol); //Save back to the corresponding position, now the window is the product of kernel and the same size (3 x 3 window)
									}
								}
							}
						}
*/



					//Avoid calculate out of the image boundaries
					short valOutput = 0;
					short valOutput2 = 0;
					if ((idxRow >= mode-1) && (idxCol >= mode-1))
					{
						output_flag = 1;
						switch (operation)
						{
							case 0:
							{
								valOutput = sumWindow(&window, mode); //Sum of 9 results
								//valOutput = valOutput /8;
								//Avoid negative values
								if (valOutput <0)
									valOutput = 0;
								//break;

								valOutput2 = sumWindow(&window2, mode);
								//valOutput2 = valOutput2 /8;
								if (valOutput2 <0)
									valOutput2 = 0;
								break;
							}
							case 1:
							{
								//Erode
								valOutput = minWindow(&window, mode);
								break;
							}
							case 2:
							{
								//Dilate
								valOutput = maxWindow(&window, mode);
								break;
							}
						}
						pixConvolved++;

					}
					else
						output_flag = 0;

					//Calculate row and col index
					if (idxCol < IMG_WIDTH_OR_COLS-1)
					{
						idxCol++;// Switch to next pixel in the same line
					}
					else
					{
						//New line
						idxCol = 0;
						idxRow++;
						pixConvolved = 0;
					}


				    if (mode != 11)
				    {
				    	if(pooling == 0)
				    		valOutput = valOutput + valOutput2;
				    }


					else if (mode == 11)
					{
						if ((countWait > (waitTicks*2)-1)&&(pixConvolved !=0))
						{
							valOutput = valOutput + lineBuff2.getval(KERNEL_DIM-(mode-1)*0.5, pixConvolved+(mode-1)*0.5);

							if (pixConvolved == IMG_WIDTH_OR_COLS - mode)
								flag = 1;
						}
						else if (flag == 1)
						{
							valOutput = valOutput + lineBuff2.getval(KERNEL_DIM-5, IMG_WIDTH_OR_COLS - (mode -1)+5);
							flag = 0;
						}
					}

					countWait++;

					if(countWait > waitTicks && output_flag == 1)
					{
						if (modcol ==0 && (modrow<IMG_WIDTH_OR_COLS))
						{
							dataOutSideChannel.data = valOutput;
							dataOutSideChannel.keep = currPixelSideChannel.keep;
							dataOutSideChannel.strb = currPixelSideChannel.strb;
							dataOutSideChannel.user = currPixelSideChannel.user;
							dataOutSideChannel.last = 0;
							dataOutSideChannel.id = currPixelSideChannel.id;
							dataOutSideChannel.dest = currPixelSideChannel.dest;

							//Send to the stream (Block if the FIFO receiver is full)
							outStream.write(dataOutSideChannel);
							sentPixels++;

						}//module; stride
					}

				} //Addition else

		} //overall for loop

		//Now send the remaining zeros (Number of delayed ticks)

		/*
					if (addition != 1)
					{
						 for(countWait = 0; countWait < waitTicks; countWait++)
						 {
							 dataOutSideChannel.data = 0;
							 dataOutSideChannel.keep = currPixelSideChannel.keep;
							 dataOutSideChannel.strb = currPixelSideChannel.strb;
				  			 dataOutSideChannel.user = currPixelSideChannel.user;

				  			 if(countWait < waitTicks - 1)
				  				 dataOutSideChannel.last = 0;
				  			 else
				  				 dataOutSideChannel.last = 1; //Mark as finished
				  			 dataOutSideChannel.id = currPixelSideChannel.id;
				 			 dataOutSideChannel.dest = currPixelSideChannel.dest;
				 			 outStream.write(dataOutSideChannel);
						 }
					}


		*/
}// Top function

short minWindow(hls::Window<KERNEL_DIM, KERNEL_DIM, short>*window, int mode)
{
	unsigned char minVal = 255;
	//look for the smallest value in the array
	for (int idxRow = 0; idxRow < mode; idxRow++)
	{
		for (int idxCol = 0; idxCol < mode; idxCol++)
		{
			unsigned char valInWindow;
			valInWindow = (unsigned char)window->getval(idxRow, idxCol);
			if (valInWindow < minVal)
				minVal = valInWindow;
		}
	}
	return minVal;
}

//Dilate will get the maximum value on the window, which in our case will always be rectangular and filled with one
short maxWindow(hls::Window<KERNEL_DIM, KERNEL_DIM, short>*window, int mode)
{
	unsigned char maxVal = 0;
	//Look for the max value in the array
	for (int idxRow = 0; idxRow < mode; idxRow++)
	{
		for (int idxCol = 0; idxCol < mode; idxCol++)
		{
			unsigned char valInWindow;
			valInWindow = (unsigned char)window->getval(idxRow, idxCol);
			if(valInWindow > maxVal)
				maxVal= valInWindow;
		}
	}
	return maxVal;
}

//Sum all values inside window (Already multiplied by the kernel)
short sumWindow(hls::Window<KERNEL_DIM, KERNEL_DIM,short>*window, int mode)
{
	short accumulator = 0;

	//Iterate on the window multiplying and accumulating the kernel and sampling window
	for (int idxRow = 0; idxRow < mode; idxRow++)
	{
		for (int idxCol = 0; idxCol < mode; idxCol++)
		{
			accumulator = accumulator + (short)window->getval(idxRow, idxCol);
		}
	}
	return accumulator;
}
