/*
 * rd_gray-scott: reaction-diffusion Gray Scott model
 *
 * ver. 2017/11/15-1647 (c) by taroh
 */

//#define	USECV		// if not send to LED matrix (NOT IMPREMENTED NOW)
					// when use cv, define either (or none) CL/CV resize.
//#undef	USESDL
#undef	THREAD		// divide lap/update calc into 4: not effective...
#undef	LEDSIM		// valid only when neither CL/CV resize are not available.
#undef	OVERSCAN	// define if 2x2 u, v matrix is averaged to mapped onto
					// displayed mag x mag pixels.
#undef	QUANTIZE	// for LED, quantize RGB into few value
#define	LEANCTL		// parameter varies by lean and pos of screen
#undef	OPTEQ		// optimize equation, reduce array access (LEANCTL void)
					// see also CUDAOPT for using CUDA (LEANCTL available)

#define	ONLYGOODSET	// only interesting parameter sets are selected

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <string>
#include <cstring>
#if defined(SDLAUDIO)
# include	<SDL.h>
#endif

#include	"arch.h"
#include	"params.h"
#include	"extern.h"

void	switchparamset(int swp);
#define	SWPARAM_INC		(-1)
#define	SWPARAM_DEC		(-2)
#define	SWPARAM_RND		(-3)
#define	SWPARAM_NOP		(-4)
void	drawdot(int x, int y, int w);
void	randot(int ndots);
void	shake(void);
void	inituv(void);
#if defined(LEANCTL)
void	mklean(void);
#endif
void	autoinit(void);
void	switchedge(void);
void	mallocmx(void);
void	freemx(void);
void	reactkey(int key);
void	usage(std::string s);
void	getopt(int argc, char **argv);
void	conwaylife(void);
void	findminmax(Float *umin, Float *umax, Float *vmin, Float *vmax);

#define	Switchplain()	pswitch = 1 - pswitch;	// switch now and nxt
void	iter(void);
#if defined(OPTEQ) | defined(LEANCTL)
void	malloclmx(void);
void	freelmx(void);
#endif

#if defined(USECV)
void	cvinitdisp(void);
void	cvmousedot(int event, int rx, int ry, int flags, void *param);
void	cvmapbgr(void);
void	cvshow(void);
void	cvgetkey(void);
void	cvcapuv(void);
void	cvcapconlife(void);
#endif

#if defined(USESDL)
void	sdlinit(void);
void	sdlmousedot(int event, int rx, int ry, int flags, void *param);
void	sdlmapbgr(void);
void	sdlshow(void);
void	sdlgetkey(void);
void	sdlconmouse(int event, int rx, int ry, int flags, void *param);
void	sdlquit(void);
#endif

#if defined(SDLAUDIO)
void	sdlaudioinit(void);
void	sdlqueueaudio(void);
void	sdlaudioquit(void);
#endif

#if defined(CUDA)
void	cudaiter(void);
void	cudaupload(void);
void	cudadownload(void);
void	cudamallocmx(void);
void	cudafreemx(void);
void	cudachangelnx(void);
void	cudachangeflny(void);
# if defined(CUDAMAP)
void	cudamapdownload(void);
# endif
# if defined(CUDAFMM)
void	cudafindminmax(Float *umin, Float *umax, Float *vmin, Float *vmax);
# endif
#endif
