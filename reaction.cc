#include	"common.h"

#if defined(USESDL)
extern void	sdlprintparamsetname(const char *name);
#endif


void
reactkey(int key)
{
	if (key == 0) {
		return;
	}
//
// abcdefghijklmnopqrs
// <--------|--------> 9 steps of lean (a-z: x, A-Z: y)

#if defined(LEANCTL)
	if ('a' <= key && key <= 's') {
		leanx = (double)(key - 'j') / 9.;
		mklean();
		return;
	} else if ('A' <= key && key <= 'S') {
		leany = (double)(key - 'J') / 9.;
		mklean();
		return;
	}
#endif
//printf("[%c]", key);
	switch (key) {
	case ESC:
		isreact = -1;
		return;
	case ' ':
		ishold = 1 - ishold;
		break;
	case '0':
   		switchparamset(SWPARAM_INC);
		break;
	case '9':
   		switchparamset(SWPARAM_DEC);
		break;
	case '1':
   		switchparamset(SWPARAM_RND);
		break;
  	case 'z':
   		shake();
		break;
   	case 'x':
 		inituv();
		break;
   	case 'X':
   		autoinit();
		break;
   	case 'y':
   		randot(1);
		break;
   	case 'w':
   		switchedge();
		break;
#if defined(USECV)
	case '@':
		cvcapuv();
		break;
#endif
	}
	return;
}



void
switchparamset(int swp)
{
	if (benchmark) {
		currentparamsetno = 3;	// fingerprint
	} else {
		if (swp == SWPARAM_DEC) {
			do {
				currentparamsetno = (currentparamsetno - 1 + nparamset)
					% nparamset;
			} while (Currentparamset.interest < intlevel);
		} else {
			if (swp == SWPARAM_RND) {
				currentparamsetno = rand() % nparamset;
			} else if (swp == SWPARAM_INC) {
				currentparamsetno++;
			} else if (swp == SWPARAM_NOP) {
			} else {
				currentparamsetno = swp;
			}
			currentparamsetno = (currentparamsetno + nparamset) % nparamset;
		 	while (Currentparamset.interest < intlevel) {
				currentparamsetno = (currentparamsetno + 1) % nparamset;
			}
		}
	}
	fprintf(stderr, "param #%d: %s (Du, Dv, f, k: %lf, %lf, %lf, %lf)\n",
		currentparamsetno,
		Currentparamset.name,
		(double)Currentparamset.du,
		(double)Currentparamset.dv,
		(double)Currentparamset.f,
		(double)Currentparamset.k);
	isreact = 1;
#if defined(LEANCTL)
	mklean();	// Currentparamset.f may be changed
#endif
#if defined(USESDL)
	sdlprintparamsetname(Currentparamset.name);
#endif

	return;
}


// note: when THITERDISPREACT && ! CUDA (both iter & reaction thread modifies
//   same array), calling inituv() while iteration will not perfectly
//   cleanup the u, v plains - meaningless.
void
inituv(void)
{
    fprintf(stderr, "clear\n");
	for (int y = 0; y < szy; y++) {
		for (int x = 0; x < szx; x++) {
			Unow(y, x) = 1.;
			Vnow(y, x) = 0.;
#if ! defined(CUDA)	// modify both plain
			Unxt(y, x) = 1.;
			Vnxt(y, x) = 0.;
//# if defined(OPTEQ) | defined(LEANCTL)	// also clear laprasian
//			lsu[y][x] = 0.;
//			lsv[y][x] = 0.;
//# endif
#endif
		}
	}
#if defined(LEANCTL)
	mklean();
#endif
	isreact = 1;

	return;
}


void
drawdot(int x, int y, int w)
{
	if (x < w) {
		x = w;
	} else if (szx <= x + w) {
		x = szx - w - 1;
	}
	if (y < w) {
		y = w;
	} else if (szy <= y + w) {
		y = szy - w - 1;
	}
	for (int yy = y - w; yy < y + w; yy++) {
		for (int xx = x - w; xx < x + w; xx++) {
			Unow(yy, xx) = UINIT;
			Vnow(yy, xx) = VINIT;
#if ! defined(CUDA)	// modify both plain
			Unxt(yy, xx) = UINIT;
			Vnxt(yy, xx) = VINIT;
#endif
		}
	}
	isreact = 1;

	return;
}


void
randot(int ndots)
{
	int		rx, ry, rw;

    fprintf(stderr, "randot\n");
    for (int i = 0; i < ndots; i++) {
		rx = rand() % szx;
		ry = rand() % szy;
		rw = (rand() % RANDOTMIN) + (RANDOTMAX - RANDOTMIN + 1);
		drawdot(rx, ry, rw);
    }
	isreact = 1;

	return;
}


void
shake(void)
{
	double		rnu, rnv;

    fprintf(stderr, "shake\n");
	for (int y = 0; y < szy; y++) {
		for (int x = 0; x < szx; x++) {
			rnu = 2 * SHAKEU * (rand() / RAND_MAX) - SHAKEU;
			rnv = 2 * SHAKEV * (rand() / RAND_MAX) - SHAKEV;
			Unow(y, x) += rnu;
			Vnow(y, x) += rnv;
#if ! defined(CUDA)	// modify both plain
			Unxt(y, x) += rnu;
			Vnxt(y, x) += rnv;
#endif
		}
	}
	isreact = 1;

	return;
}


#if defined(LEANCTL)
void
mklean(void)
{
// x, y     0        szx/2        szx
//          <----------|----------->
// lnx[x] 1-(leanx|y   1.         1+(leanx|y*leanrate)
// lny[y]    *leanrate)
//
// flny[] is Currentparamset.f * ln[y] (above)
// note: if szy != szx, f offset by distance is decided by szx unit,
// to avoid lean "feelings" on x/y differs.

	fprintf(stderr, "lean (x, y) = (%lf, %lf), (1+/-%lf, 1+/-%lf)\n",
		(double)leanx, (double)leany,
		(double)(leanx * leanrate),
		(double)(leany * leanrate));
	for (int i = 0; i < szx; i++) {
		lnx[i] = 1. + ((double)(i - szx / 2) / (double)(szx / 2))
						* leanx * leanrate;
	}
	for (int i = 0; i < szy; i++) {
		flny[i] = (1. + ((double)(i - szy / 2) / (double)(szx / 2))
						* leany * leanrate
		          ) * Currentparamset.f;
	}
	isreact = 1;
#if defined(CUDA)
	cudachangelnx();
	cudachangeflny();
#endif
	return;
}
#endif


void
autoinit(void)
{
	inituv();
	if (benchmark) {
		drawdot(szx / 2, szy / 2, 6);
		shkcnt = -1;
	} else {
	    randot(Currentparamset.nrandot);
        shkcnt = Currentparamset.shakecnt;
	}
	return;
}


void
switchedge(void)
{
	fprintf(stderr, "switch edge: ");
	if (edgetype == EDGE_NEUMANN) {
		edgetype = EDGE_DONUT;
		fprintf(stderr, "donut.\n");
	} else {	// EDGE_DONUT
		edgetype = EDGE_NEUMANN;
		fprintf(stderr, "neumann.\n");
	}
	isreact = 1;
	return;
}
