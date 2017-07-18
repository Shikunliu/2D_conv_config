#include "hls_video.h"

//Defination of size of image

#define IMG_WIDTH_OR_COLS 240
#define IMG_HEIGHT_OR_ROWS 320

// 3x3 kernel
#define KERNEL_DIM 11

//Image file path
#define INPUT_IMAGE_CORE   "C:\\Users\\baohaochun\\2D_conv_config\\ab.bmp"
#define INPUT_IMAGE_2   "C:\\Users\\baohaochun\\2D_conv_config\\a.bmp"
#define INPUT_IMAGE_3   "C:\\Users\\baohaochun\\2D_conv_config\\c.bmp"
#define OUTPUT_IMAGE_CORE   "C:\\Users\\baohaochun\\2D_conv_config\\filter_result.bmp"
#define OUTPUT_IMAGE_CORE2   "C:\\Users\\baohaochun\\2D_conv_config\\filter_resultCORE2.bmp" //Reference output generated by the reference functions


//Use the axi stream side-channel
#include <ap_axi_sdata.h>
typedef ap_axiu<8,2,5,6> uint_8_side_channel;
typedef ap_axis<8,2,5,6> int_8_side_channel;

//Reference functions
short convolve2d(hls::Window<KERNEL_DIM,KERNEL_DIM,char>*window, char kernel[KERNEL_DIM*KERNEL_DIM]);


//IP
void doImgproc(hls::stream<uint_8_side_channel> &inStream, hls::stream<uint_8_side_channel> &inStream2, hls::stream<int_8_side_channel> &outStream, char kernel[KERNEL_DIM*KERNEL_DIM],int operation, int mode, int addition, int stride, int pooling);
//Parameters: 1. Input stream 2. Output stream 3. Kernel 4. Operation
short sumWindow(hls::Window<KERNEL_DIM,KERNEL_DIM,short>*window, int mode);
short minWindow(hls::Window<KERNEL_DIM,KERNEL_DIM,short>*window, int mode);
short maxWindow(hls::Window<KERNEL_DIM,KERNEL_DIM,short>*window, int mode);
