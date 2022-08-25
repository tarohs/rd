/*
 * rd_gray_scott - reaction & diffusion Gray-Scott model
 *
 * (c) 2017 by SASAKI, Taroh (sasaki.taroh@gmail.com)
 */
#define	VERSION	 "3.5.1-20180117"
/*
 * see "param.h" for iteration options, "arch.h" for optimizing options.
 *
 * version history:
 * 2017.
 *  10. 28--11. 3: ver 1.1: proto version on Python3 + OpenCV3
 *               : OpenCV mouse/keyboard operation
 *         11.  4: ver 1.2: initial C cersion, tested by MacOS LLVM + OpenCV3
 *             *7: ver 2.2: optimized iter code (slant add for laprasian)
 *	             : 32x32 LED matrix simulation
 *               : paramset introduced
 *               : pthread iter tested
 *               : OpenCV resize
 *             7?: tested by Raspberry Pi 3 gcc
 *	            8: (CUDA OpenCV tested - later obsolated)
 *                 - tested by NVIDIA Jetson TK1 gcc + OpenCV 3
 *               : tested by GPD/Intel ATOM + ubuntu on Win10 + gcc + OpenCV3
 *               : some size added
 *            *10: ver 2.3: introduced SIMD pipline verctor operation
 *                 - ARM NEON instructions
 *                 - Intel SSE instruction
 *               : specifying size (x, y) separately, any size available
 *	           13: OpenCL/OpenCV3 tested
 *               : interesting param sets selected, introduced intlevel
 *             16: introduced cmake, source divided
 *               : display text (paramset name)
 *               : Conway's life game added
 *         12--17: tested w/ASUS tinker gcc
 *               : tested tinker + RPi touch display gajets
 *         15--18: lean control by keyboard + HID i/f'ed Arduino sensor module
 *            *24: ver 2.4: pthread for iter & (disp or (disp + react))
 *        *23--26: ver 3.1: CUDA GPU iter
 *                 tested by NVIDIA Jetson TX1 gcc + nvcc
 *             27: optimized GPU: iter code, BLOCKSIZE,
 *                 __const__ removed from iter loop, -use_fast_math
 *             28: optimized GPU: __shared__ memory w/ part-dup blocks
 *                 (GPU on TX1 overclock here)
 *             29: (CUDA works on TK1)
 *               : (CUDAMAP implemented but slower)
 *             30: CUDAFMM (find min & max) on CUDA
 *          12. 1: edge displayed
 *             *2: ver 3.2: added SDL2 drawing, react key/mouse (ex. conlife)
 *              3: SDL2 paramset name displayed
 *              4: added SDL2 touch panel, tested on Tinker (Macbook)
 *             *5: ver 3.3: added SDL2 audio, tested on Tinker
 *            5-7: (doubled action in a thread (# of thread halfed)
 *                 in cudaiter.cu:cudagsblk() kernel - not so effective)
 *              7: (corrected paramset select actions)
 *            8-9: (tried some optimization in CUDA)
 *             10: added SDL2 draw/react for conlife
 *                 (ver 3.3.2 for Mathematical Art 2017)
 *          11-13: (OpenGL native functions used for SDL2 - not effective)
 *          14-15: added video capture for OpenCV
 *             19: (lifeboard is dynamically malloced, no MAXSIZE in prog)
 *            -26: (small bug around cam, etc. fixed.  tested w/opencv3.4.0)
 *     2018. 1. 6: (Raspi Zero works: non-NEON bug fixed)
 *          15-16: ver 3.4.1: sdlaudio.cc independent, SDL audio can be
 *               : used with OpenCV2.  audio dev can be specified,
 *               : bufsize varies dep on x * y.  audio workes on macOS.
 *             17: ver 3.5.1: reduced audio latency.
 */

#define		_MAIN_
#include	"common.h"
#if defined(USECV)
# include	"cvdraw.h"
#endif

void	dispandreact(void);
void	iter(void);
void	disp(void);
void	waitreact(void);

int		conlife = 0;
#if defined(USECV)
//cv::TickMeter		tickmeter;
#endif


int
main(int argc, char **argv)
{
	int		nloops = 0, lcnt;
//	int		gencnt = 0;
#if defined(CUDA)
	int		lastreact = 1;
#endif

	currentparamsetno = STARTPARAMSETNO;
#if defined(SDLAUDIO)
	audiodevno = SDLAUDIO_DEVNO;
#endif

	getopt(argc, argv);
	if (conlife) {
		conwaylife();
	}
	if (benchmark) {
		nloops = (int)((double)szx / 32. * (double)NBENCH);
		lcnt = 0;
#if defined(USECV)
//		tickmeter.start();
#endif
	}
	srand((unsigned)time(NULL));
	pswitch = 0;
	edgetype = EDGE_NEUMANN;

	mallocmx();
#if defined(CUDA)
	cudamallocmx();
#elif defined(OPTEQ) | defined(LEANCTL)
	malloclmx();
#endif
	shkcnt = -1;
	ishold = 0;
#if defined(LEANCTL) | defined(CUDA)
	leanx = 0.;
	leany = 0.;
#endif

#if defined(USECV)
	cvinitdisp();
#elif defined(USESDL)
	sdlinit();
#endif
#if defined(SDLAUDIO)
	sdlaudioinit();
#endif
//	gencnt = 0;
	switchparamset(SWPARAM_NOP);
	autoinit();
	dt = DT;
	dx = DX;
	while (1) {
// reset reaction flag (global) here;
// mouse reaction will occur before waitreact().
		isreact = 0;
		while (ishold) {
			waitreact();
		}

#if defined(THITERDISPREACT)
# if defined(CUDA)
		if (lastreact) {
// if mouse reaction (drawn onto flame buffer) or
// keyboard reaction (flame buffer will be totally modified) exists, 
// last one time iter result (GPU buffer) disposed
// (not downloaded onto flame buffer).
			cudaupload();
		} else {
// download result of iter from GPU buffer to flame buffer,
// display (& get keyboard reaction for next)
			cudadownload();
		}
		std::thread thiter(cudaiter);
# else
		std::thread thiter(iter);
# endif	// CUDA or not
		std::thread thdispandreact(dispandreact);
		thiter.join();
		thdispandreact.join();

#elif defined(THITERDISP)
# if defined(CUDA)
		if (lastreact) {
			cudaupload();
		} else {
			cudadownload();
		}
		std::thread thiter(cudaiter);
# else
		std::thread thiter(iter);
# endif // CUDA or not
		std::thread thdisp(disp);
		thiter.join();
		thdisp.join();
		waitreact();

#else	// no thread for iter & disp/react
# if defined(CUDA)
		if (lastreact) {
			cudaupload();
		}
		cudaiter();
		if (! isreact) {	// while iteration, mouse reaction didn't occur
			cudadownload();
		}
# else
		iter();
# endif
		dispandreact();
#endif

		if (isreact < 0) {	// reaction for keyboard quit
			break;
		}
#if defined(CUDA)
		lastreact = isreact;
#endif
    	if (benchmark) {
        	lcnt++;
        	if (nloops < lcnt) {
#if defined(USECV)
//				tickmeter.stop();
//				std::cerr << "benchmark took "
//					<< tickmeter.getTimeMilli() << "[ms]\n";
#endif
           		break;
			}
		}
//		gencnt += anistop;
		continue;
	}

	freemx();
#if defined(CUDA)
	cudafreemx();
#elif defined(OPTEQ) | defined(LEANCTL)
	freelmx();
#endif
#if defined(SDLAUDIO)
	sdlaudioquit();
#endif
#if defined(USESDL)
	sdlquit();
#endif

	exit(0);
}


void
dispandreact(void)
{
#if defined(CUDAMAP)
	cudamapdownload();
#endif
#if defined(USECV)
	cvmapbgr();
	cvshow();
#elif defined(USESDL)
	sdlmapbgr();
	sdlshow();
#endif

	waitreact();

	return;
}


void
disp(void)
{
#if defined(CUDAMAP)
	cudamapdownload();
#endif
#if defined(USECV)
	cvmapbgr();
	cvshow();
#elif defined(USESDL)
	sdlmapbgr();
	sdlshow();
#endif

	return;
}


void
waitreact(void)
{
#if defined(USECV)
	cvgetkey();
#endif
#if defined(USESDL)
	sdlgetkey();
#endif
	if (shkcnt == 0) {
		shake();
	}
	if (0 <= shkcnt) {
		shkcnt--;
	}
	return;
}


void
findminmax(Float *umin, Float *umax, Float *vmin, Float *vmax)
{
	Float	t;
	*umin = 99.;
	*umax = -99.;
	*vmin = 99.;
	*vmax = -99.;

	for (Int y = 0; y < szy; y++) {
		for (Int x = 0; x < szx; x++) {
			t = fabs(Unow(y, x));
			if (t < *umin) {
				*umin = t;
			} else if (*umax < t) {
				*umax = t;
			}
			t = fabs(Vnow(y, x));
			if (t < *vmin) {
				*vmin = t;
			} else if (*vmax < t) {
				*vmax = t;
			}
		}
	}
	return;
}


void
mallocmx(void)
{
	szxy = szx * szy;				// globally used
	zszxy = sizeof(float) * szxy;	// globally used
	zszx = sizeof(float) * szx;		// globally used
// note: on DONUT world, ls[uv]matrix needs below-right cell of
//       downmost-rightmost corner. -(1)
//       [uv]matrix needs to be referred above cell of upmost-leftmost corner,
//       left of (1) (i.e. below cell of downmost-rightmost) x 2 plains.
//       therefore, here declare:
//           (szxy) + (szx) + 1 cells for ls[uv]matrix, -(2)
//           2 * ((szx) + (szxy) + (szx)) cells for [uv]matrix,
//              pbase0: index for upmost-leftmost cell when pswitch == 0,
//              pbase1:                                when pswitch == 1.
//       (2): SSE/NEON does 4-cells vector operation, "+1" padded to +(szx).
//
//#define	UVMATRIXSIZE	(2 * (zszxy + 2 * zszx))
//#define	LSUVMATRIXSIZE	(zszxy + 2 * zszx)
	if ((umatrix = (float *)malloc(UVMATRIXSIZE)) == NULL ||
	    (vmatrix = (float *)malloc(UVMATRIXSIZE)) == NULL
	   ) {
		exit(30);
	}
#if defined(LEANCTL)
	if ((lnx  = (float *)malloc(szx * sizeof(float))) == NULL ||
	    (flny = (float *)malloc(szy * sizeof(float))) == NULL
	   ) {
		exit(30);
	}
#endif
	pbase0 = szx;
	pbase1 = szx + szxy + szx + szx;

	return;
}


void
freemx(void)
{
	free(umatrix);
	free(vmatrix);

	return;
}


void
usage(std::string s)
{
	std::cerr << 
"usage: rd_gray_scott OPTIONS\n"
"\n"
"OPTIONS:\n"
"-zSIZEOPT: specify size; SIZEOPT are:\n"
"    s: small  (" << SIZE_S << " x " << SIZE_S << ", magnify x"
	 	<< MAG_S << ")\n"
"    m: medium (" << SIZE_M << " x " << SIZE_M << ", magnify x"
		<< MAG_M << ")\n"
"    l: large  (" << SIZE_L << " x " << SIZE_L << ", magnify x"
		<< MAG_L << ")\n"
"    L: Large  (" << SIZE_LL << " x " << SIZE_LL << ", magnify x"
		<< MAG_LL << ")\n"
"    d: display  (" << SIZE_DX << " x " << SIZE_DY << ", magnify x"
		<< MAG_D << ")\n"
"    D: display  (" << SIZE_DDX << " x " << SIZE_DDY << ", magnify x"
		<< MAG_DD << ")\n"
"    SIZEXxSIZEY: specify # cells";

//	if (sizeof(Int) == 1) {
//		std::cerr << ", must be < 255";
//	}
#if defined(SSE)
	std::cerr << " (SIZEX must be mul of 4)";
#endif
	std::cerr << "\n"
"-mMAG: magnify MAG\n"
"-iINTLVL: interest level; higher INTLVL more interesting ps are avaialble\n"
"-pN: start with paramset #N\n"
"-x: with text; display paramater set name\n"
#if defined(SDLAUDIO)
"-s[DEVNO]: with audio (with DEVNO)"
#endif
"-a: animation stop # of iter (default " << ANISTOP << ")\n"
"-w: wait msec (default " << KEYBOARDWAITMS << ")\n"
"-b: benchmark; stops after several steps\n"
"-c: conway life game (-abipsx are ignored)\n"
"\n"
"rd_gray_scott, version " VERSION "\n"
"(c) 2017 by taroh (sasaki.taroh@gmail.com)\n";

	if (s != "") {
		std::cerr << "\n" + s + "\n";
	}
	exit(1);
}


void
getopt(int argc, char **argv)
{
	int		j;

	dispparamname = 0;
	benchmark = 0;
	szx = SIZE_S;
	szy = SIZE_S;
	mag = MAG_S;
	intlevel = 5;
	waitms = KEYBOARDWAITMS;
	anistop = ANISTOP;
#if defined(SDLAUDIO)
	withaudio = 0;
#endif

	for (int ap = 1; ap < argc; ap++) {
		if (strcmp(argv[ap], "-b") == 0) {
			benchmark = 1;

		} else if (strcmp(argv[ap], "-x") == 0) {
			dispparamname = 1;

		} else if (strncmp(argv[ap], "-i", 2) == 0) {
			intlevel = atoi(argv[ap] + 2);
			for (j = 0; j < nparamset; j++) {
				if (paramset[j].interest <= intlevel) {
					break;
				}
			}
			if (j == nparamset) {
				usage("no interesting param at this level (" +
					std::to_string(intlevel) + ").");
			}

		} else if (strcmp(argv[ap], "-c") == 0) {
			conlife = 1;

#if defined(SDLAUDIO)
		} else if (strncmp(argv[ap], "-s", 2) == 0) {
			withaudio = 1;
			if (argv[ap][2] != 0) {
			    audiodevno = atoi(argv[ap] + 2);
			}
#endif

		} else if (strncmp(argv[ap], "-w", 2) == 0) {
			waitms = atoi(argv[ap] + 2);

		} else if (strncmp(argv[ap], "-a", 2) == 0) {
			anistop = atoi(argv[ap] + 2);

		} else if (strncmp(argv[ap], "-m", 2) == 0) {
			mag = atoi(argv[ap] + 2);

		} else if (strncmp(argv[ap], "-p", 2) == 0) {
			currentparamsetno = atoi(argv[ap] + 2);
			if (currentparamsetno < 0 || nparamset < currentparamsetno ||
				Currentparamset.interest < intlevel) {
				usage("illegal paramset # (or intlevel below) given");
			}

		} else if (strncmp(argv[ap], "-z", 2) == 0) {
			if (argv[ap][2] == 's') {
				szx = SIZE_S;
				szy = SIZE_S;
				mag = MAG_S;
			} else if (argv[ap][2] == 'm') {
				szx = SIZE_M;
				szy = SIZE_M;
				mag = MAG_M;
			} else if (argv[ap][2] == 'l') {
				szx = SIZE_L;
				szy = SIZE_L;
				mag = MAG_L;
			} else if (argv[ap][2] == 'L') {
				szx = SIZE_LL;
				szy = SIZE_LL;
				mag = MAG_LL;
			} else if (argv[ap][2] == 'd') {
				szx = SIZE_DX;
				szy = SIZE_DY;
				mag = MAG_D;
			} else if (argv[ap][2] == 'D') {
				szx = SIZE_DDX;
				szy = SIZE_DDY;
				mag = MAG_DD;
			} else {
				char *chp;
				szx = atoi(argv[ap] + 2);
				if ((chp = index(argv[ap] + 2, 'x')) == NULL) {
					usage("");
				}
				szy = atoi(chp + 1);
			}
#if defined(SSE)
			if (szx % 4 != 0) {
				std::string s = "(SSE) size X (" + std::to_string(szx) + 
					") should be multiply of 4.";
				usage(s);
			}
#endif

		} else {
			usage("unknown option: " + std::string(argv[ap]));
		}
	}

	if (szx * mag <= 0 || // MAXSZ < szx * mag ||
		szy * mag <= 0 // || MAXSZ < szy* mag
	   ) {
		usage("invalid size (" +
			 std::to_string(szx) + ", " + std::to_string(szy) + ").");
	}

	return;
}
