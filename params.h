// SIZE_X don't exceed 255 if not INTASINT (Uint8 is char)
#define	SIZE_S	(32)
#define	MAG_S	(10)
#define	SIZE_M	(64)
#define	MAG_M	(8)
#define	SIZE_L	(128)
#define	MAG_L	(4)
#define	SIZE_LL	(252)
#define	MAG_LL	(3)
//#define	SIZE_DX	(160)
//#define	SIZE_DY	(96)
//#define	MAG_D	(5)
#define	SIZE_DX	(200)
#define	SIZE_DY	(120)
#define	MAG_D	(4)
#define	SIZE_DDX	(256)
#define	SIZE_DDY	(178)
#define	MAG_DD	(4)

#define MOUSEDOT	(3)
#define	TXTCRDX		(20)
#define	TXTCRDY		(szy * mag - 30)
#define	TXTCOL1		16, 16, 16
#define	TXTCOL2		240, 240, 240

#if defined(USECV)
# define	CVTXTFACE	cv::FONT_HERSHEY_SCRIPT_SIMPLEX
# define	CVTXTCRD	cv::Point(TXTCRDX, TXTCRDY)
# define	CVTXTSCL	(2)
# define	CVTXTCOL1	cv::Scalar(TXTCOL1)
# define	CVTXTCOL2	cv::Scalar(TXTCOL2)
# define	CVTXTTHK1	(8)
# define	CVTXTTHK2	(4)
# define	CAPSIZEX	(1280)
# define	CAPSIZEY	(720)
#endif

#if defined(USESDL)
# define	SDLFONTPATH	"../fonts/CCArtSansLf-Light.ttf"
# define	SDLFONTSIZE	(90)
# define	SDLTXTCRDX	(TXTCRDX)
# define	SDLTXTCRDY	(TXTCRDY - 60)
# define	SDLTEDGEOFFSETX	(3)
# define	SDLTEDGEOFFSETY	(2)
#endif

#if defined(SDLAUDIO)
# define	SDLAUDIO_FREQ	(22050)
# define	SDLAUDIO_DEVNO	(2)		// survey with SDL_GetNumAudioDevices()
#endif

#define	EDGENEU_BORDER	(2)
#define	EDGENEU_COLOR	192, 192, 192

#define	LIFE_BORDER	(1)


#define	MAXSZ	(2560)
//#define	MAXSZ	(SIZE_LL * MAG_LL)

#if ! defined(ANISTOP)
# define	ANISTOP	(100)	// draw frame once in ANISTOP iteration
#endif
#if ! defined(KEYBOARDWAITMS)
# define	KEYBOARDWAITMS	(10)
#endif
#define	NBENCH	(100)	// * ANISTOP generations to stop

#if defined(CUDAFMM)
//# define	CUDA_FMMSIZE	(128)
# define	CUDA_FMMSIZE	szy
#endif


#if defined(LEANCTL)
# define	leanrate	(0.50)	// change rate of k at maximum lean and edge
#endif

#define	UINIT	(0.50)
#define	VINIT	(0.25)
#define	DX 		(1.)	// space step
#define	DT		(.1)	// time step
//	T = 10.;		// total time
//	n = int(T / dt);	// N iter
#define	SHAKEU	(.04)	// width of random value when shake
#define	SHAKEV	(.025)

#define	RANDOTMIN	(4)	// size of random dots
#define	RANDOTMAX	(8)
#define	Epsilon	(1.e-16)	// should be much greater than FLT_MIN

#define	ESC	(0x1b)

typedef struct {
	int		interest;
	Float	du;
	Float	dv;
	Float	f;
	Float	k;
	const char	*name;
	int		nrandot;
	int		shakecnt;
} paramset_t;

// here are interesting parameters; last 2 params are for autoinit().

#if defined(_MAIN_)
paramset_t	paramset[] = 
{
	{4, 0.16, 0.08, 0.035, 0.065, "Bacteria 1",    10, -1},
	{7, 0.14, 0.06, 0.035, 0.065, "Bacteria 2",     3, -1},
	{8, 0.16, 0.08, 0.060, 0.062, "Coral",         10, -1},
	{8, 0.19, 0.05, 0.060, 0.062, "Fingerprint",    5, -1},
	{2, 0.10, 0.10, 0.018, 0.050, "Spirals",        8,  5},
	{8, 0.12, 0.08, 0.020, 0.050, "Spirals Dense",  4, 20},
	{2, 0.10, 0.16, 0.020, 0.050, "Spirals Fast",  15, 30},
	{8, 0.16, 0.08, 0.020, 0.055, "Unstable",      10, -1},
	{3, 0.16, 0.08, 0.050, 0.065, "Worms 1",       10, -1},
	{7, 0.16, 0.08, 0.054, 0.063, "Worms 2",       10, -1},
	{8, 0.16, 0.08, 0.035, 0.060, "Zebrafish",      2, -1},
	{1, 0.16, 0.08, 0.0620, 0.0609, "U-skate?",    10, -1},
	{4, 0.16, 0.08, 0.025, 0.059, "U-skate 2",     10, -1},
	{2, 0.16, 0.08, 0.0460, 0.0594, "Negatons",    20, -1}
};
int	nparamset = ((int)(sizeof(paramset) / sizeof(paramset_t)));
#else
extern paramset_t	paramset[];
extern int			nparamset;
#endif
#define	STARTPARAMSETNO	(1)
