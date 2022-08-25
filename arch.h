
#if defined(__x86_64__)
# define	SSE				// for Intel CPU
# undef		NEON
# define	FLOATASFLOAT	// if not float as double
# define	INTASINT		// if not int as char, uint as uchar
# if defined(__APPLE__)		// MacOS
#  undef	CLRESIZE
#  define	CVRESIZE	
			// on MacOS 10.12 (Qt) + OpenCV 3.2, cv::waitKey() in thread
			// (THITERDISPREACT) fails. even only disp is threaded
			// (THITERDISP) is 3% faster.
#  if defined(USESDL)
#    if defined(USEGL)
#       define	THITERDISPREACT	// SDL is thread unsafe on SDL_PollEvents()
#       undef	THITERDISP
#    else
#       undef	THITERDISPREACT	// SDL is thread unsafe on SDL_PollEvents()
#       define	THITERDISP
#    endif
#  elif defined(USECV)
#   undef	THITERDISPREACT	// cv::waitKey() thread unsafe???
#   define	THITERDISP
#  endif
# endif
#endif

#if defined(__arm__) | defined(__ARM_ARCH)
# undef		SSE
# define	FLOATASFLOAT
# define	INTASINT

# if defined(CUDA) | defined(NVCC)	// ARM with CUDA. Jetson TK1 & TX1 tested.
#  undef	CLRESIZE
#  define	CVRESIZE
//#  undef	CUDARESIZE
#  undef	CUDALSOPT	// slant add optimize
#  define	CUDABLOPT	// block shared memory optimize
#  undef	CUDAMAP	// map (u, v) to color using CUDA
#  undef	CUDAFMM	// find min-max using CUDA
#  define	CUDA_BLOCKSIZE	(32)
#  define	CUDA_MAXBLOCKSIZE	(1024)
#  undef	NEON	// neon optimization is not used with CUDA code,
					// while nvcc don't understand some NEON typedef.
#  undef	CUDAINTERLEAVE
#  if defined(USESDL)
#   undef	THITERDISPREACT
#   undef	THITERDISP
#  elif defined(USECV)
// note: OpenCV (3.4.0) w/o GTHREAD donot allow THITERDISP/THITERDISPREACT
#   define	THITERDISPREACT
#   undef	THITERDISP
#   define	ANISTOP	(250)
#   define	KEYBOARDWAITMS	(50)
#  endif

# else	// ARM without CUDA.  RasPi3 and Tinker tested.
# undef		CLRESIZE
# define	CVRESIZE	
//#  define	NEON			// for ARM CPU
#  if defined(USECV)
#   define	THITERDISPREACT	// iter and disp/react are pararelly done by thread
#   undef	THITERDISP		// iter and disp are pararelly done by thread
#  elif defined(USESDL)
#   undef	THITERDISPREACT
#   undef	THITERDISP
#  endif
# endif
#endif

#if defined(SSE)
# include	<xmmintrin.h>
#elif defined(NEON)
# include	<arm_neon.h>
#endif

#if defined(CUDAMAP)
# include	<cuda.h>
# undef	CVRESIZE
# undef	CLRESIZE
# if ! defined(CUDA)
#  error CUDAMAP needs to define CUDA
# endif
#endif

#if defined(THREAD) | defined(THITERDISPREACT) | defined(THITERDISP)
# include	<thread>
#endif

#if defined(FLOATASFLOAT)
 typedef	float	Float;
# if defined(SSE)
 typedef	__attribute__((aligned(16))) float		AFloat;
 typedef	__m128		M128;
# elif defined(NEON)
 typedef	float			AFloat;
 typedef	float32x4_t		M128;
# else
 typedef	float	AFloat;
# endif
#else
 typedef	double	Float;
#endif
#if defined(INTASINT)
 typedef int		Int;	// fastest for most compiler
 typedef int		Int;
#else
 typedef	char	Int;
 typedef	short	Int;
#endif

