#include	"common.h"

void	update(int sx, int ex, int sy, int ey);
void	edge(void);
#if defined(OPTEQ) | defined(LEANCTL)
void	mklap(int sx, int ex, int sy, int ey);
#endif
void	dupud(void);


// anistop times iteration loop.
void
iter(void)
{
#if defined(OPTEQ) | defined(LEANCTL)
	dtdu_dxdx = dt * Currentparamset.du / dx / dx;
	dtdv_dxdx = dt * Currentparamset.dv / dx / dx;
#endif
	if (edgetype == EDGE_NEUMANN) {
		for (Int g = 0; g < anistop; g++) {
#if defined(THREAD)
# if defined(OPTEQ) | defined(LEANCTL)
			std::thread t1(mklap, 1, szx / 2 - 1, 1, szy / 2 - 1);
			std::thread t2(mklap, szx / 2, szy - 2, 1, szy / 2 - 1);
			std::thread t3(mklap, 1, szx / 2 - 1, szy / 2, szy - 2);
			std::thread t4(mklap, szx / 2, szx - 2, szy / 2, szy - 2);
			t1.join();
			t2.join();
			t3.join();
			t4.join();
# endif
			std::thread t5(update, 1, szx / 2 - 1, 1, szy / 2 - 1);
			std::thread t6(update, szx / 2, szx - 2, 1, szy / 2 - 1);
			std::thread t7(update, 1, szx / 2 - 1, szy / 2, szy - 2);
			std::thread t8(update, szx / 2, szx - 2, szy / 2, szy - 2);
			t5.join();
			t6.join();
			t7.join();
			t8.join();
#else	// not THREAD
# if defined(OPTEQ) | defined(LEANCTL)
			mklap(1, szx - 2, 1, szy - 2);
# endif
			update(1, szx - 2, 1, szy - 2);
#endif // not THREAD
			edge();
			Switchplain();
		}

	} else if (edgetype == EDGE_DONUT) {
#if defined(THREAD)
		fprintf(stderr, "edgetype donut is not ready for thread.\n");
		exit(101);
#else
		for (Int g = 0; g < anistop; g++) {
			dupud();
			mklap(0, szx - 1, 0, szy - 1);
			update(0, szx - 1, 0, szy - 1);
			Switchplain();
		}
#endif

	} else {
		fprintf(stderr, "unknown edge type (%d).\n", (int)edgetype);
		exit(101);
	}
	return;
}


#if defined(OPTEQ) | defined(LEANCTL)
// reduce re-access of array:
// lsX are sum of X(x, y - 1) and X(x - 1, y)  (where X in {U, V})
// laprasian is made from lsX(x, y) + lsX(x + 1, y + 1) - 4 X(x, y).
// here lu[][], lv[][] are multipled by dt * du / dx / dx.

void
mklap(int sx, int ex, int sy, int ey)
{
#if defined(SSE) | defined(NEON)
# if defined(SSE)
#  define	MLOAD1(f)	_mm_load1_ps(f)
#  define	MLOAD(f)	_mm_load_ps(f)
#  define	MLOADU(f)	_mm_loadu_ps(f)
#  define	MSTORE(f, m)	_mm_store_ps(f, m)
#  define	MADD(a, b)	_mm_add_ps(a, b)
#  define	MSUB(a, b)	_mm_sub_ps(a, b)
#  define	MMUL(a, b)	_mm_mul_ps(a, b)
# elif defined(NEON)
#  define	MLOAD1(f)	vld1q_dup_f32(f)
#  define	MLOAD(f)	vld1q_f32(f)
#  define	MLOADU(f)	MLOAD(f)
#  define	MSTORE(f, m)	vst1q_f32(f, m)
#  define	MADD(a, b)	vaddq_f32(a, b)
#  define	MSUB(a, b)	vsubq_f32(a, b)
#  define	MMUL(a, b)	vmulq_f32(a, b)
# endif
	M128	mu, ml, mh, m4, mdu, mdv;
	float	c4 = 4.;

	m4 = MLOAD1(&c4);
	mdu = MLOAD1(&dtdu_dxdx);
	mdv = MLOAD1(&dtdv_dxdx);

//	for (Int y = sy; y <= ey + 1; y++)
//		for (Int x = (sx / 4 * 4); x <= ex + 1; x += 4)
	for (int p = sy * szx + (sx / 4 * 4), pp = (pswitch ? pbase1 : pbase0) + p;
		 p <= (ey + 1) * szx + (ex + 1);
		 p += 4, pp += 4) {
// note: this part will automatically be optimized for SSE on gcc.
//			Lsu(y, x) = Unow(y - 1, x) + Unow(y, x - 1);
			mu = MLOAD(&(umatrix[pp - szx]));	// Unow(y - 1, x)
			ml = MLOADU(&(umatrix[pp - 1]));	// Unow(y, x - 1)
			mh = MADD(mu, ml);
			MSTORE(&(lsumatrix[p]), mh);		// Lsu(y, x)
//			Lsv(y, x) = Vnow(y - 1, x) + Vnow(y, x - 1);
			mu = MLOAD(&(vmatrix[pp - szx]));	// Vnow(y - 1, x)
			ml = MLOADU(&(vmatrix[pp - 1]));	// Vnow(y, x - 1)
			mh = MADD(mu, ml);
			MSTORE(&(lsvmatrix[p]), mh);		// Lsv(y, x)
	}
# else	/* not using SSE nor NEON */
	for (Int y = sy; y <= ey + 1; y++) {
		for (Int x = sx; x <= ex + 1; x++) {
			Lsu(y, x) = Unow(y - 1, x) + Unow(y, x - 1);
			Lsv(y, x) = Vnow(y - 1, x) + Vnow(y, x - 1);
		}
	}
# endif

# if defined(SSE) | defined(NEON)
//	for (Int y = sy; y <= ey; y++)
//		for (Int x = (sx / 4 * 4); x <= ex; x += 4)
	for (int p = sy * szx + (sx / 4 * 4), pp = (pswitch ? pbase1 : pbase0) + p;
		 p <= ey * szx + ex;
		 p += 4, pp += 4) {
			mu = MLOAD(&(lsumatrix[p]));	// Lsu(y, x)
			ml = MLOADU(&(lsumatrix[p + szx + 1]));	// Lsu(y + 1, x + 1)
			mh = MADD(mu, ml);
			mu = MLOAD(&(umatrix[pp]));	// Unow(y, x)
			ml = MMUL(mu, m4);
			mh = MSUB(mh, ml);
			mh = MMUL(mh, mdu);
			MSTORE(&(lumatrix[p]), mh);	// Lu(y, x)
//			Lu(y, x) = (Lsu(y, x) + Lsu(y + 1, x + 1) - 4. * Unow(y, x))
//						* dtdu_dxdx;
			mu = MLOAD(&(lsvmatrix[p]));	// Lsv(y, x)
			ml = MLOADU(&(lsvmatrix[p + szx + 1]));	// Lsv(y + 1, x + 1)
			mh = MADD(mu, ml);
			mu = MLOAD(&(vmatrix[pp]));	// Vnow(y, x)
			ml = MMUL(mu, m4);
			mh = MSUB(mh, ml);
			mh = MMUL(mh, mdv);
			MSTORE(&(lvmatrix[p]), mh);	// Lv(y, x)
//			Lv[y][x] = (Lsv(y, x) + Lsv(y + 1, x + 1) - 4. * Vnow(y, x))
//						* dtdv_dxdx;
	}

# else	// original definition
	for (Int y = sy; y <= ey; y++) {
		for (Int x = sx; x <= ex; x++) {
			Lu(y, x) = (Lsu(y, x) + Lsu(y + 1, x + 1) - 4. * Unow(y, x))
						* dtdu_dxdx;
			Lv(y, x) = (Lsv(y, x) + Lsv(y + 1, x + 1) - 4. * Vnow(y, x))
						* dtdv_dxdx;
//fprintf(stderr, "(%d:%d):", y, x);
//fprintf(stderr, "%f-%f\n", Lu(y, x), Lv(y, x));
		}
	}
# endif
	return;
}
#endif	// OPTEQ or LEANCTL


void
update(int sx, int ex, int sy, int ey)
{
#if defined(LEANCTL)
	Float	unow, vnow, f, fy, uvv;
#elif defined(OPTEQ)
	Float	unow, vnow, dtf, fk;
#else
	Float	lu, lv;
#endif

// LEANCTL is 3% slower because dt * Currentparamset.f (dtf) &
//		Currentparamset.f + Currentparamset.k (fk)
// are not fixed value among the loop, since f varies dep on (x, y).
#if defined(LEANCTL)
	for (int y = sy, py = sy * szx; y <= ey; y++, py += szx) {
		fy = flny[y];
		for (int x = sx, p = py + sx, ppnow = (pswitch ? pbase1 : pbase0) + p,
				ppnxt = (pswitch ? pbase0 : pbase1) + p;
			 x <= ex;
			 x++, p++, ppnow++, ppnxt++) {
			unow = umatrix[ppnow];	// Unow(y, x)
			vnow = vmatrix[ppnow];	// Vnow(y, x)
			uvv = unow * vnow * vnow;
			f = fy * lnx[x];
// on below 2 eqs dt as variable is 6% faster than DT as constant...
			umatrix[ppnxt] = unow + Lu(y, x) +		// Unxt(y, x) =
				dt * (-uvv + f * (1 - unow));
			vmatrix[ppnxt] = vnow + Lv(y, x) +		// Vnxt(y, x) =
				dt * (uvv - (f + Currentparamset.k) * vnow);
//if (y == 1 && x == 1) { fprintf(stderr, "%f-%f]", unow, vnow); }
		}
	}

// reduce re-calculation of dynamic values (u, v).
#elif defined(OPTEQ)
	dtf = dt * Currentparamset.f;
	fk = Currentparamset.f + Currentparamset.k;
	for (Int y = sy; y < ey; y++) {
		for (Int x = sx; x < ex; x++) {
			unow = Unow(y, x);
			vnow = Vnow(y, x);
// on below 2 eqs dt as variable is 6% faster than DT as constant...
			Unxt(y, x) = unow
				+ Lu(y, x)
				- dt * unow * (vnow * vnow + Currentparamset.f) + dtf;
			Vnxt(y, x) = vnow
				+ Lv(y, x)
				+ dt * vnow * (unow * vnow - fk);
// below are slower
//          Unxt(y, x) = unow * (1. - dt * (vnow * vnow + Currentparamset.f))
//				+ dtf + Lu(y, x);
//          Vnxt(y, x) = vnow * (1. + dt * (unow * vnow - fk))
//              + Lv(y, x);
		}
	}

#else	// original definition
	for (Int y = sy; y < ey; y++) {
		for (Int x = sx; x < ex; x++) {
			lu = (Unow(y - 1, x) + Unow(y, x - 1)
				+ Unow(y, x + 1) + Unow(y + 1, x)
				- 4. * Unow(y, x)) / dx / dx;
			lv = (Vnow(y - 1, x) + Vnow(y, x - 1)
				+ Vnow(y, x + 1) + Vnow(y + 1, x)
				- 4. * Vnow(y, x)) / dx / dx;
			Unxt(y, x) = Unow(y, x)
				+ dt * (Currentparamset.du * lu 
						- Unow(y, x) * Vnow(y, x) * Vnow(y, x)
						+ Currentparamset.f * (1. - Unow(y, x))
					   );
			Vnxt(y, x) = Vnow(y, x)
				+ dt * (Currentparamset.dv * lv
						+ Unow(y, x) * Vnow(y, x) * Vnow(y, x)
						- (Currentparamset.f + Currentparamset.k) * Vnow(y, x)
					   );
		}
	}
#endif

	return;
}


void
edge(void)
{
// Neumann condition: derivatives at the edges
// are null.
	for (Int i = 1; i < szy - 1; i++) {
		Unxt(i, 0) = Unxt(i, 1);
		Vnxt(i, 0) = Vnxt(i, 1);
		Unxt(i, szx - 1) = Unxt(i, szx - 2);
		Vnxt(i, szx - 1) = Vnxt(i, szx - 2);
	}
	for (Int i = 0; i < szx; i++) {
		Unxt(0, i) = Unxt(1, i);
		Unxt(szy - 1, i) = Unxt(szy - 2, i);
		Vnxt(0, i) = Vnxt(1, i);
		Vnxt(szy - 1, i) = Vnxt(szy - 2, i);
	}
	return;
}


void
dupud(void)
{
// donut condition: duplicate edges of [uv]matrix ([UV]now page):
//        downmost edge to above of upmost edge,
//        upmost edge to below of downmost edge
	for (int ps = (pswitch ? pbase1 : pbase0), pd = ps + szxy;
		 ps < (pswitch ? pbase1 : pbase0) + szx; ps++, pd++) {
		umatrix[pd] = umatrix[ps];
		vmatrix[pd] = vmatrix[ps];
	}
	for (int ps = (pswitch ? pbase1 : pbase0) + szxy - szx, pd = ps - szxy;
		 ps < (pswitch ? pbase1 : pbase0) + szxy; ps++, pd++) {
		umatrix[pd] = umatrix[ps];
		vmatrix[pd] = vmatrix[ps];
	}
	return;
}


#if defined(OPTEQ) | defined(LEANCTL)
void
malloclmx(void)
{
	if (
		(lsumatrix = (float *)malloc(LSUVMATRIXSIZE)) == NULL ||
		(lsvmatrix = (float *)malloc(LSUVMATRIXSIZE)) == NULL ||
		(lumatrix = (float *)malloc(zszxy)) == NULL ||
		(lvmatrix = (float *)malloc(zszxy)) == NULL
	) {
		exit(30);
	}
	return;
}


void
freelmx(void)
{
	free(lsumatrix);
	free(lsvmatrix);
	free(lumatrix);
	free(lvmatrix);

	return;
}
#endif


