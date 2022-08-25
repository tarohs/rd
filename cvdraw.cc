#include	"common.h"

#if defined(USECV)	// whole this file

#include	"cvdraw.h"

#if defined(CUDAFMM)
void cudafindminmax(void);
extern Float	minmaxres[];
#endif

#if defined(CUDARESIZE)
////# include <opencv2/core/opengl.hpp>
//# include <opencv2/core/cuda.hpp>
//# include <opencv2/imgproc.hpp>
#endif

int		cvcapture(cv::Mat *cap);


void
cvinitdisp(void)
{
	img = cv::Mat::zeros(cv::Size(szx, szy), CV_8UC3);
#if defined(CUDAMAP)
	if (! img.isContinuous()) {
		fprintf(stderr, "need continurous cv::Mat for GPU mapping.\n");
		exit(81);
	}
#endif
#if defined(CLRESIZE)
	if (cv::ocl::useOpenCL() == false) {
		fprintf(stderr, "not using OpenCL.  #undef CLRESIZE and remake.\n");
		exit(2);
	}
	cv::ocl::setUseOpenCL(true);
#elif defined(CVRESIZE)
#else
# if defined(OVERSCAN)
#  error use CVRESIZE or CLRESIZE for OVERSCAN simulation
# else
	img = cv::Mat::zeros(cv::Size(szx * mag, szy * mag), CV_8UC3);
# endif
#endif
	windowtitle = "Gray-Scott";
//	cv::namedWindow(windowtitle, CV_WINDOW_AUTOSIZE);
	cv::namedWindow(windowtitle,
		cv::WINDOW_AUTOSIZE | cv::WINDOW_GUI_NORMAL);
//CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO
	cvMoveWindow(windowtitle, 0, 0);
	cv::setMouseCallback(windowtitle, cvmousedot);

	return;
}


void
cvgetkey(void)
{
	int		key;

	key = cv::waitKey(waitms) & 0xff;
	reactkey(key);

	return;
}


#if defined(CVRESIZE) | defined(CLRESIZE)
/*
# define Map(y, x, pb, pg, pr)	\
    (img.ptr<cv::Vec3b>(y))[x] = cv::Vec3b(pb, pg, pr), \
	map[y][x][0] = pb, map[y][x][1] = pg, map[y][x][2] = pr
*/
# define Map(y, x, pb, pg, pr)	\
    (img.ptr<cv::Vec3b>(y))[x] = cv::Vec3b(pb, pg, pr)
#else
# if defined(LEDSIM)	// only valid if USECV && ! CVRESIZE
#  define	Mag		(mag / 2)
# else
#  define	Mag		mag
# endif
/*
# define Map(y, x, pb, pg, pr)	\
     for (Int yy = 0; yy < Mag; yy++) { \
       img.ptr<cv::Vec3b>yptr = (y * mag + yy); \
       for (Int xx = 0; xx < Mag; xx++) { \
         yptr[x * mag + xx] = cv::Vec3b(pb, pg, pr); \
     } } \
	map[y][x][0] = pb, map[y][x][1] = pg, map[y][x][2] = pr
*/
# define Map(y, x, pb, pg, pr)	\
     for (Int yy = 0; yy < Mag; yy++) { \
       cv::Vec3b *yptr = img.ptr<cv::Vec3b>(y * mag + yy); \
       for (Int xx = 0; xx < Mag; xx++) { \
         yptr[x * mag + xx] = cv::Vec3b(pb, pg, pr); \
     } }
#endif

/*
#else	// not USECV
# define Map(y, x, pb, pg, pr)	\
	map[y][x][0] = pb, map[y][x][1] = pg, map[y][x][2] = pr
#endif
*/


void
cvmapbgr(void)
{
	Float	umin, umax, vmin, vmax,
			udif, vdif;
	Uint8	pr, pg, pb;
//	static uchar	map[MAXSZ][MAXSZ][3];

#if defined(CUDAFMM)
	cudafindminmax(&umin, &umax, &vmin, &vmax);
#else
	findminmax(&umin, &umax, &vmin, &vmax);
#endif
	udif = umax - umin;
	vdif = vmax - vmin;
    if (umax - umin < Epsilon) {
		udif = Epsilon;
	}
	if (vmax - vmin < Epsilon) {
		vdif = Epsilon;
	}
// initialize condition here...
//    if (umax - umin < Epsilon || vmax - vmin < Epsilon)
//// || umin < Epsilon || vmin < Epsilon) { 
//       	inituv();
//	}

	Audioresetp;

	for (Int y = 0; y < szy; y++) {
		for (Int x = 0; x < szx; x++) {
#if defined(QUANTIZE)
			pr = (Uint8)(
				(Float)((int)((Unow(y, x) - umin) / udif * 8.)) * (255. / 8.));
			pb = (Uint8)(
				(Float)((int)((Vnow(y, x) - vmin) / vdif * 8.)) * (255. / 8.));
#else
			pr = (Uint8)((Unow(y, x) - umin) / udif * 255.);
			pb = (Uint8)((Vnow(y, x) - vmin) / vdif * 255.);
#endif
			pg = (Uint8)(255 - abs((int)pr - (int)pb));
			Map(y, x, pb, pg, pr);

			Audioset(pb);
		}
	}
	Audioqueue;

	return;
}


void
cvshow(void)
{
#if defined(CVRESIZE) | defined(CLRESIZE) | defined(CUDARESIZE)
	cv::Mat	img2; // = cv::Mat::zeros(cv::Size(szx * mag, szy * mag), CV_8UC3);
#endif
#if defined(CUDARESIZE)
//	cv::cuda::GpuMat	d_img(cv::Size(szx * mag, szy * mag), CV_8UC3),
//						d_img2(cv::Size(szx * mag, szy * mag), CV_8UC3);
#endif

#if defined(CVRESIZE)
# if defined(OVERSCAN)
	cv::resize(img, imgs, cv::Size(0, 0), 0.5, 0.5, cv::INTER_NEAREST);
	cv::resize(imgs, img2, cv::Size(0, 0), mag * 2, mag * 2, cv::INTER_NEAREST);
# else
	cv::resize(img, img2, cv::Size(0, 0), mag, mag, cv::INTER_NEAREST);
# endif
#elif defined(CLRESIZE)
// this causes error (u->refcount == 0) on Intel HD
//	uimg = img.getUMat(cv::ACCESS_READ, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
	img.copyTo(uimg);
# if defined(OVERSCAN)
	cv::resize(uimg, uimgs, cv::Size(0, 0), 0.5, 0.5, cv::INTER_NEAREST);
	cv::resize(uimgs, uimg2, cv::Size(0, 0), mag * 2, mag * 2,
		 cv::INTER_NEAREST);
# else
	cv::resize(uimg, uimg2, cv::Size(0, 0), mag, mag, cv::INTER_NEAREST);
# endif
//	img2 = uimg2.getMat(cv::ACCESS_READ);
	uimg2.copyTo(img2);	// download then imshow() is faster... why!?
					// note: ARM OpenCL is buggy: unresized image displayed
					// on direct imshow( , uimg2) after resize(uimg, umg2).
#elif defined(CUDARESIZE)
//	d_img.upload(img);
//	cv::cuda::resize(d_img, d_img2,
//		cv::Size(0, 0), mag, mag, cv::INTER_NEAREST);
//	d_img2.download(img2);
#endif

# if defined(CVRESIZE) | defined(CLRESIZE) | defined(CUDARESIZE)
	cvdrawfancy(img2);
    cv::imshow(windowtitle, img2);
# else
	cvdrawfancy(img);
	cv::imshow(windowtitle, img);
# endif
	return;
}


void
cvmousedot(int event, int rx, int ry, int flags, void *param)
{
	rx /= mag;
	ry /= mag;
	if (rx < 0 || szx <= rx ||
            ry < 0 || szy <= ry) {
                return;
        }
	if (event == CV_EVENT_LBUTTONDOWN ||
		(event == CV_EVENT_MOUSEMOVE &&
			 (flags & CV_EVENT_FLAG_LBUTTON) != 0)
       ) {
		drawdot(rx, ry, MOUSEDOT);
		isreact = 1;
	}

	return;
}


void
cvdrawfancy(cv::Mat im)
{
	if (dispparamname) {
		cv::putText(im, (cv::String)Currentparamset.name, CVTXTCRD,
			CVTXTFACE, CVTXTSCL, CVTXTCOL1, CVTXTTHK1, CV_AA, false);
		cv::putText(im, (cv::String)Currentparamset.name, CVTXTCRD,
			CVTXTFACE, CVTXTSCL, CVTXTCOL2, CVTXTTHK2, CV_AA, false);
	}
	if (edgetype == EDGE_NEUMANN) {
		cv::rectangle(im,
			cv::Point(0, 0),
			cv::Point(szx * mag - 1, szy * mag - 1),
			cv::Scalar(EDGENEU_COLOR),
			EDGENEU_BORDER);
	}
	return;
}


void
cvcapuv(void)
{
	cv::Mat		cap;
	uchar		*yptr, px;

	if (cvcapture(&cap) != 0) {
		return;
	}

	inituv();
	for (int y = 0; y < szy * mag; y++) {
		yptr = cap.ptr<uchar>(y);
		for (int x = 0; x < szx * mag; x++) {
			px = yptr[x];
			if (128 <= (int)px && rand() < 0.25 * RAND_MAX) {
				drawdot(x / mag, y / mag, 2);
			}
		}
	}
	isreact = 1;

	return;
}


void
cvcapconlife(void)
{
	cv::Mat		cap;
	uchar		*yptr, px;

	if (cvcapture(&cap) != 0) {
		return;
	}

	for (int y = 0; y < szy; y++) {
        for (int x = 0; x < szx; x++) {
            Bnow(x, y) = LIFES_DEAD;
        }
    }

	for (int y = 0; y < szy * mag; y++) {
		yptr = cap.ptr<uchar>(y);
		for (int x = 0; x < szx * mag; x++) {
			px = yptr[x];
			if (128 <= (int)px) {
                Bnow(x / mag, y / mag) = LIFES_ALIVE;
			}
		}
	}
//	isreact = 1;

	return;
}


int
cvcapture(cv::Mat *cap)
{
	cv::VideoCapture	capture(0);
	cv::Mat				capframe, capcrop, grayimg, grayimg2;
	cv::Rect			rectcrop;
//	Float	umin, umax, vmin, vmax;
	int		key;
	uchar	lut[256];

//	capture.set(CV_CAP_PROP_FRAME_WIDTH,  CAPSIZEX);
//	capture.set(CV_CAP_PROP_FRAME_HEIGHT, CAPSIZEY);
	if (! capture.isOpened()) {
		fprintf(stderr, "cvcapture: isOpened() failed.\n");
		return (-1);
	}
	capture >> capframe;
	while (0 <= cv::waitKey(30)) {	// '@' key event remains
	}
	if (capframe.size().width == 0 || capframe.size().height == 0) {
		fprintf(stderr, "cvcapture: cap width or height zero.\n");
#if defined(__APPLE__)
		fprintf(stderr, "(macos: try \"sudo killall VDCAssistant\")\n");
#endif
		return (-1);
	}
	if (szx * mag <= capframe.size().width &&
		szy * mag <= capframe.size().height) {

// camera is bigger than frame
		rectcrop.x = (capframe.size().width - szx * mag) / 2;
		rectcrop.y = (capframe.size().height - szy * mag) / 2;
		rectcrop.width = szx * mag;
		rectcrop.height = szy * mag;
		while (1) {
			capture >> capframe;
			capcrop = capframe(rectcrop);
			cv::imshow(windowtitle, capcrop);
			if (0 <= (key = cv::waitKey(30))) {
				key = key & 0xff;
				if (key == 0x1b) {
					return (-1);
				} else if (key == ' ') {
					break;
				}
			}
		}

	} else {
		double	rx, ry;
		cv::Mat	capresize;

		while (1) {
			capture >> capframe;
			cv::imshow("capture", capframe);
			if (0 <= (key = cv::waitKey(30))) {
				key = key & 0xff;
				if (key == 0x1b) {
					return (-1);
				} else if (key == ' ') {
					break;
				}
			}
		}
		cv::destroyWindow("capture");
		rx = (double)(szx * mag) / capframe.size().width;
		ry = (double)(szy * mag) / capframe.size().height;
		if (rx < ry) {
			rx = ry;
		}
		cv::resize(capframe, capresize, cv::Size(0, 0), rx, rx,
			cv::INTER_NEAREST);
		rectcrop.x = (capresize.size().width - szx * mag) / 2;
		rectcrop.y = (capresize.size().height - szy * mag) / 2;
		rectcrop.width = szx * mag;
		rectcrop.height = szy * mag;
		capcrop = capresize(rectcrop);
	}
	cv::waitKey(500);
	capture.release();

// to grayscale
	cv::cvtColor(capcrop, grayimg, CV_RGB2GRAY);
	cv::imshow(windowtitle, grayimg);
	cv::waitKey(500);

// contrast
#define	GAMMA	(1.8)
	for (int i = 0; i < 256; i++) {
		lut[i] = (int)(255. * pow((float)i / 255., 1. / GAMMA));
	}
	cv::LUT(grayimg, cv::Mat(cv::Size(256, 1), CV_8U, lut), grayimg2);
	cv::imshow(windowtitle, grayimg2);
	cv::waitKey(500);

//// calculate Laplacian of image
//	cv::Laplacian(grayimg, *cap, CV_8U, 3);
////	cv::convertScaleAbs(timg, *cap, 1, 0);

// Canny
	cv::Canny(grayimg2, *cap, 20., 100., 3);
	cv::imshow(windowtitle, *cap);
	cv::waitKey(500);

	return (0);
}


#endif	// USECV
