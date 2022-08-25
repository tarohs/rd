#if defined(_MAIN_)
# define	Extern
#else
# define	Extern	extern
#endif

typedef unsigned char	Uint8;

Extern AFloat	*umatrix, *vmatrix;
Extern Float	dt, dx;
Extern Float	dtdu_dxdx, dtdv_dxdx;
Extern Float	minmaxres[4];

Extern int	szx, szy, szxy, mag;
Extern size_t	zszx, zszxy;
Extern int	pswitch;	// to use u[], v[] plane alternatively.
						//  0: now == 0, nxt == 1,  1: now == 1, nxt == 0
Extern int	pbase0, pbase1;
#define	PNOW	(pswitch ? pbase1 : pbase0)
#define	PNXT	(pswitch ? pbase0 : pbase1)

#define	UVMATRIXSIZE	(2 * (zszxy + 2 * zszx))
#define	LSUVMATRIXSIZE	(zszxy + 2 * zszx)
#define	Unow(y, x)	umatrix[PNOW + (y) * szx + (x)]
#define	Unxt(y, x)	umatrix[PNXT + (y) * szx + (x)]
#define	Vnow(y, x)	vmatrix[PNOW + (y) * szx + (x)]
#define	Vnxt(y, x)	vmatrix[PNXT + (y) * szx + (x)]
#define	Lu(y, x)	lumatrix[(y) * szx + (x)]
#define	Lv(y, x)	lvmatrix[(y) * szx + (x)]
#define	Lsu(y, x)	lsumatrix[(y) * szx + (x)]
#define	Lsv(y, x)	lsvmatrix[(y) * szx + (x)]

#if defined(OPTEQ) | defined(LEANCTL)
//Extern AFloat	lsu[MAXSZ][MAXSZ], lsv[MAXSZ][MAXSZ];
//Extern AFloat	lu[MAXSZ][MAXSZ], lv[MAXSZ][MAXSZ];
Extern AFloat	*lsumatrix, *lsvmatrix;
Extern AFloat	*lumatrix, *lvmatrix;
#endif

Extern int	benchmark;
Extern int	dispparamname;

Extern const char	*windowtitle;

typedef enum {
	EDGE_NEUMANN,
	EDGE_DONUT
} edgetype_t;
Extern edgetype_t	edgetype;

Extern int	shkcnt;		// when initialize, shake after random dot gener.

Extern int	isreact;
Extern int	ishold;

Extern int	currentparamsetno;
#define		Currentparamset		paramset[currentparamsetno]
Extern int	intlevel;

#if defined(LEANCTL)
Extern Float	*lnx, *flny;
Extern Float	leanx, leany;
#endif

// for conway life
Extern int		*lifeboard;		// [2][MAXSZ][MAXSZ];
#define Bnow(x, y)    lifeboard[pswitch       * szx * szy + (y) * szx + (x)]
#define Bnxt(x, y)    lifeboard[(1 - pswitch) * szx * szy + (y) * szx + (x)]
#define	LIFES_ALIVE	(1)
#define	LIFES_DEAD	(0)

Extern int		anistop;
Extern int		waitms;

#if defined(SDLAUDIO)
 Extern SDL_AudioDeviceID	audev;
 Extern int		withaudio;
 Extern int		audiodevno;
 Extern Uint8	*aubuf;
 Extern int		aubufp;
 Extern int		steleosw;
 Extern int		aubufsize;
 Extern int		aubuflow;
# define	Audioresetp		aubufp = steleosw
# define	Audioset(val)	if (withaudio) { \
								aubuf[aubufp] = val; \
								aubufp += 2; \
							}
# define	Audioqueue		sdlqueueaudio()
#else
# define	Audioresetp
# define	Audioset(val)
# define	Audioqueue
#endif
