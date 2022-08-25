#if defined(USECV)	// whole this file

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#if defined(CLRESIZE)
# include <opencv2/core/ocl.hpp>
#endif

void	cvdrawfancy(cv::Mat im);

Extern cv::Mat	img;
//#if defined(CVRESIZE) | defined(CLRESIZE) | defined(CUDA)
//Extern cv::Mat  img2;
//#endif
#if defined(CVRESIZE)
# if defined(OVERSCAN)
Extern cv::Mat	imgs;
# endif
#elif defined(CLRESIZE)
Extern cv::UMat uimg, uimg2;
# if defined(OVERSCAN)
Extern cv::UMat	uimgs;
# endif
//#elif defined(CUDA)	// now CUDA is not used for resize
//Extern cv::cuda::GpuMat d_img, d_img2;
//# if defined(OVERSCAN)
//Extern cv::cuda::GpuMat	d_imgs;
//# endif
#endif

#endif	// USECV
