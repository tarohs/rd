#include	"common.h"

#if defined(CUDA)	// while this module

#include	<cuda.h>
#if defined(CUDAMAP)
# include	"cvdraw.h"
#endif

float	*d_u, *d_v;
float	*d_lnx;
float	*d_flny;
float	*d_lsu, *d_lsv, *d_lu, *d_lv;
#if defined(CUDAMAP)
# define	ZMAP	(szxy * mag * mag * 3)
# define	ZZMAP	(ZMAP * sizeof(unsigned char))
unsigned char	*d_map;
#endif
#if defined(CUDAFMM)
float	*d_minmaxarr;
float	*d_minmaxres;	// {min(d_u), max(d_u), min(d_v), max(d_v)}
//Float	minmaxres[4];	// on host
#endif


__constant__ int	dc_szy, dc_szx, dc_szxy, dc_mag;
//__constant__ int	dc_pswitch, dc_edgetype;
__constant__ int	dc_edgetype;
__constant__ float	dc_du_dxdx, dc_dv_dxdx, dc_k;
__constant__ int	dc_pbase0, dc_pbase1;

#if defined(CUDALSOPT)
__global__ void	cudamklsk(float *d_u, float *d_v, float *d_lsu, float *d_lsv,
	int d_pswitch);
__global__ void	cudaupdatek(float *d_u, float *d_v, float *d_lu, float *d_lv,
	float *d_flny, float *d_lnx, int d_pswitch);
#elif defined(CUDABLOPT)
__global__ void cudagsblk(float *d_u, float *d_v, float *d_flny, float *d_lnx,
				int d_pswitch);
#else
__global__ void	cudagsk(float *d_u, float *d_v, float *d_flny, float *d_lnx,
				int d_pswitch);
#endif
#if defined(CUDAMAP)
void	cudamapbgr(void);
__global__ void cudamapk(float *d_u, float *d_v,
		unsigned char *d_map, int pswitch,
		float d_udif, float d_umin, float d_vdif, float d_vmin);
#endif
#if defined(CUDAFMM)
void	cudafindminmax(void);
__global__ void cudafindminmaxk(float *source, int size, float *result);
#endif


void
cudamallocmx(void)
{
	if (cudaMalloc((void **)&d_u, UVMATRIXSIZE)
			!= cudaSuccess ||
		cudaMalloc((void **)&d_v, UVMATRIXSIZE)
			!= cudaSuccess ||
		cudaMalloc((void **)&d_lnx, sizeof(float) * szx)
			!= cudaSuccess ||
		cudaMalloc((void **)&d_flny, sizeof(float) * szy)
			!= cudaSuccess
	   ) {
		fprintf(stderr, "(cudamallocmx) %s\n",
			cudaGetErrorString(cudaGetLastError()));
		exit(31);
	}
//	printf("u %p, v %p +%x\n", d_u, d_v, UVMATRIXSIZE);
#if defined(CUDALSOPT)
	if (cudaMalloc((void **)&d_lsu, sizeof(float) * szxy)
			!= cudaSuccess ||
		cudaMalloc((void **)&d_lsv, sizeof(float) * szxy)
			!= cudaSuccess
		) {
		exit(32);
	}
#endif
#if defined(CUDAMAP)
	if (cudaMalloc((void **)&d_map, ZZMAP) != cudaSuccess) {
		exit(32);
	}
#endif
#if defined(CUDAFMM)
	int		nsrch = (szxy + CUDA_FMMSIZE - 1) / CUDA_FMMSIZE;
	if (cudaMalloc((void **)&d_minmaxarr, sizeof(float) * 2 * nsrch)
		 != cudaSuccess ||
		cudaMalloc((void **)&d_minmaxres, sizeof(float) * 4)
		 != cudaSuccess
	   ) {
		exit(32);
	}
#endif

	return;
}


void
cudafreemx(void)
{
	cudaFree(&d_u);
	cudaFree(&d_v);
	cudaFree(&d_lnx);
	cudaFree(&d_flny);
#if defined(CUDALSOPT)
	cudaFree(&d_lsu);
	cudaFree(&d_lsv);
#endif
#if defined(CUDAMAP)
	cudaFree(&d_map);
#endif

	return;
}



void
cudaupload(void)
{
	float	du_dxdx, dv_dxdx;

	du_dxdx = Currentparamset.du / dx / dx;
	dv_dxdx = Currentparamset.dv / dx / dx;
	if (cudaMemcpyToSymbol(dc_du_dxdx, &du_dxdx, sizeof(float))
			!= cudaSuccess ||
		cudaMemcpyToSymbol(dc_dv_dxdx, &dv_dxdx, sizeof(float))
			!= cudaSuccess ||
		cudaMemcpyToSymbol(dc_k,  &Currentparamset.k,  sizeof(float))
			!= cudaSuccess ||
	    cudaMemcpyToSymbol(dc_szy, &szy, sizeof(int))
			!= cudaSuccess ||
		cudaMemcpyToSymbol(dc_szx, &szx, sizeof(int))
			!= cudaSuccess ||
		cudaMemcpyToSymbol(dc_szxy, &szxy, sizeof(int))
			!= cudaSuccess ||
		cudaMemcpyToSymbol(dc_mag, &mag, sizeof(int))
			!= cudaSuccess ||
		cudaMemcpyToSymbol(dc_pbase0, &pbase0, sizeof(int))
			!= cudaSuccess ||
		cudaMemcpyToSymbol(dc_pbase1, &pbase1, sizeof(int))
			!= cudaSuccess ||
		cudaMemcpyToSymbol(dc_edgetype, &edgetype, sizeof(int))
			!= cudaSuccess
		) {
		exit(33);
	}
// half sized copy don't work... why?
//	if (cudaMemcpy(d_u + pswitch * zszxy, umatrix + pswitch * zszxy, zszxy,
//			cudaMemcpyHostToDevice) != cudaSuccess ||
//		cudaMemcpy(d_v + pswitch * zszxy, vmatrix + pswitch * zszxy, zszxy,
//			cudaMemcpyHostToDevice) != cudaSuccess ||
	if (cudaMemcpy(d_u, umatrix, UVMATRIXSIZE,
			cudaMemcpyHostToDevice) != cudaSuccess ||
		cudaMemcpy(d_v, vmatrix, UVMATRIXSIZE,
			cudaMemcpyHostToDevice) != cudaSuccess
	   ) {
		exit(34);
	}
	return;
}


void
cudadownload(void)
{
//	if (cudaMemcpy(umatrix + pswitch * zszxy, d_u + pswitch * zszxy, zszxy,
//			cudaMemcpyDeviceToHost) != cudaSuccess ||
//		cudaMemcpy(vmatrix + pswitch * zszxy, d_v + pswitch * zszxy, zszxy,
//			cudaMemcpyDeviceToHost) != cudaSuccess
	if (cudaMemcpy(umatrix, d_u, UVMATRIXSIZE,
			cudaMemcpyDeviceToHost) != cudaSuccess ||
		cudaMemcpy(vmatrix, d_v, UVMATRIXSIZE,
			cudaMemcpyDeviceToHost) != cudaSuccess
	   ) {
		exit(35);
	}
	return;
}


void
cudachangelnx(void)
{
	if (cudaMemcpy(d_lnx, lnx, sizeof(float) * szx,
			cudaMemcpyHostToDevice) != cudaSuccess) {
		exit(36);
	}
	return;
}


void
cudachangeflny(void)
{
	if (cudaMemcpy(d_flny, flny, sizeof(float) * szy,
			cudaMemcpyHostToDevice) != cudaSuccess) {
		exit(37);
	}
	return;
}


void
cudaiter(void)
{
	dim3	dimBlock(CUDA_BLOCKSIZE / 2, CUDA_BLOCKSIZE);
#if defined(CUDABLOPT)
	dim3	dimGrid((szx + CUDA_BLOCKSIZE - 2 - 1) / (CUDA_BLOCKSIZE - 2),
			        (szy + CUDA_BLOCKSIZE - 2 - 1) / (CUDA_BLOCKSIZE - 2));
#else
	dim3	dimGrid((szx + CUDA_BLOCKSIZE - 1) / CUDA_BLOCKSIZE,
			        (szy + CUDA_BLOCKSIZE - 1) / CUDA_BLOCKSIZE);
#endif
	cudaError_t		stat;

	if (cudaMemcpyToSymbol(dc_pbase0, &pbase0, sizeof(int))
			!= cudaSuccess ||
		cudaMemcpyToSymbol(dc_pbase1, &pbase1, sizeof(int))
			!= cudaSuccess
	   ) {
		exit(38);
	}
	for (Int g = 0; g < anistop; g++) {
#if defined(CUDALSOPT)
		cudamklsk<<<dimGrid, dimBlock>>>(d_u, d_v, d_lsu, d_lsv, pswitch);
		if ((stat = cudaGetLastError()) != cudaSuccess) {
			fprintf(stderr, "(kernel) %s\n", cudaGetErrorString(stat));
			exit(39);
		}
		cudaupdatek<<<dimGrid, dimBlock>>>(d_u, d_v, d_lsu, d_lsv,
			d_flny, d_lnx,
			pswitch);
		if ((stat = cudaGetLastError()) != cudaSuccess) {
			fprintf(stderr, "(kernel) %s\n", cudaGetErrorString(stat));
			exit(40);
		}
#elif defined(CUDABLOPT)
		cudagsblk<<<dimGrid, dimBlock>>>(d_u, d_v, d_flny, d_lnx,
			pswitch);
		if ((stat = cudaGetLastError()) != cudaSuccess) {
			fprintf(stderr, "(kernel) %s\n", cudaGetErrorString(stat));
			exit(41);
		}
#else
		cudagsk<<<dimGrid, dimBlock>>>(d_u, d_v, d_flny, d_lnx,
			pswitch);
		if ((stat = cudaGetLastError()) != cudaSuccess) {
			fprintf(stderr, "(kernel) %s\n", cudaGetErrorString(stat));
			exit(42);
		}
#endif
		Switchplain();
	}

	return;
}


//
// CUDA kernel functions
//


#if ! defined(CUDALSOPT) && ! defined(CUDABLOPT)	// simple method
__global__ void
cudagsk(float *d_u, float *d_v, float *d_flny, float *d_lnx, int d_pswitch)
{
	int		gy = blockIdx.y * blockDim.y + threadIdx.y;
    int		gx = blockIdx.x * blockDim.x + threadIdx.x;
	int		x, y, pnow, plf, prt, pab, pbl;
	float	lu, lv;
	float	uvv;
	int		d_pbasenow = (d_pswitch ? dc_pbase1 : dc_pbase0);
	int		d_pbasenxt = (d_pswitch ? dc_pbase0 : dc_pbase1);

	if (dc_szy <= gy || dc_szx <= gx) {	// for block padding threads
		return;
	}
#if defined(CUDAINTERLEAVE)	// swap bit (0, 1) and (2, 3)
	gy = (gy & ~0xf) | ((gy & 0xc) >> 2) | ((gy & 0x3) << 2);
	gx = (gx & ~0xf) | ((gx & 0xc) >> 2) | ((gx & 0x3) << 2);
#endif
	if (dc_edgetype == EDGE_NEUMANN) {
		if (gy == 0) {
			y = 1;
		} else if (gy == dc_szy - 1) {
			y = dc_szy - 2;
		} else {
			y = gy;
		}
		if (gx == 0) {
			x = 1;
		} else if (gx == dc_szx - 1) {
			x = dc_szx - 2;
		} else {
			x = gx;
		}
	} else {	// EDGE_DONUT
		y = gy;
		x = gx;
	}
#define	D_f	(d_flny[y] * d_lnx[x])

	pnow = d_pbasenow + y * dc_szx + x;
	pab = pnow - dc_szx;
	pbl = pnow + dc_szx;
	if (y == 0) {
		pab = pab + dc_szxy;
	} else if (y == dc_szy - 1) {
		pbl = pbl - dc_szxy;
	}
	plf = pnow - 1;
	prt = pnow + 1;
	if (x == 0 && y == 0) {
		plf = plf + dc_szxy;
	} else if (x == dc_szx - 1 && y == dc_szy - 1) {
		prt = prt - dc_szxy;
	}

#define		Pnxt 	(d_pbasenxt + gy * dc_szx + gx)
#define		D_unow	d_u[pnow]
#define		D_vnow	d_v[pnow]
	lu = (
			d_u[pab]		// Unow(y - 1, x)
			+ d_u[plf]		// Unow(y, x - 1)
			+ d_u[prt]		// Unow(y, x + 1)
			+ d_u[pbl]		// Unow(y + 1, x)
			- 4. * D_unow	// Unow(y, x)
		) * dc_du_dxdx;
	lv = (
			d_v[pab]		// Vnow(y - 1, x)
			+ d_v[plf]		// Vnow(y, x - 1)
			+ d_v[prt]		// Vnow(y, x + 1)
			+ d_v[pbl]		// Vnow(y + 1, x)
			- 4. * D_vnow	// Vnow(y, x)
		) * dc_dv_dxdx;
	uvv	= D_unow * D_vnow * D_vnow;
	d_u[Pnxt]					// Unxt(gy, gx) 
		= D_unow + DT * (lu + -uvv + D_f * (1. - D_unow));
	d_v[Pnxt]					// Vnxt(gy, gx)
		= D_vnow + DT * (lv + uvv - (D_f + dc_k) * D_vnow);

	return;
}
#endif	// simple method


#if defined(CUDALSOPT)	// laprasian optimized

__global__ void
cudamklsk(float *d_u, float *d_v, float *d_lsu, float *d_lsv, int d_pswitch)
{
	int		y = blockIdx.y * blockDim.y + threadIdx.y;
    int		x = blockIdx.x * blockDim.x + threadIdx.x;
	int		ppnow, pplf, ppab;
	int		d_pbasenow = (d_pswitch ? dc_pbase1 : dc_pbase0);
//	int		d_pbasenxt = (d_pswitch ? dc_pbase0 : dc_pbase1);

	if (dc_szy <= y || dc_szx <= x) {	// for block padding threads
		return;
	}
	ppnow = y * dc_szx + x;
	pplf = ppnow - 1;
	ppab = ppnow - dc_szx;
	if (pplf < 0) {
		pplf = pplf + dc_szxy;
	}
	if (ppab < 0) {
		ppab = ppab + dc_szxy;
	}
	d_lsu[ppnow] = d_u[d_pbasenow + pplf] + d_u[d_pbasenow + ppab];
	d_lsv[ppnow] = d_v[d_pbasenow + pplf] + d_v[d_pbasenow + ppab];
	
	return;
}



__global__ void
cudaupdatek(float *d_u, float *d_v, float *d_lsu, float *d_lsv,
	float *d_flny, float *d_lnx, int d_pswitch)
{
	int		gy = blockIdx.y * blockDim.y + threadIdx.y;
    int		gx = blockIdx.x * blockDim.x + threadIdx.x;
	int		x, y, pnow, ppnow, pprb;
	int		d_pbasenow = (d_pswitch ? dc_pbase1 : dc_pbase0);
	int		d_pbasenxt = (d_pswitch ? dc_pbase0 : dc_pbase1);
	float	uvv;

	if (dc_szy <= gy || dc_szx <= gx) {	// for block padding threads
		return;
	}
	if (dc_edgetype == EDGE_NEUMANN) {
		if (gy == 0) {
			y = 1;
		} else if (gy == dc_szy - 1) {
			y = dc_szy - 2;
		} else {
			y = gy;
		}
		if (gx == 0) {
			x = 1;
		} else if (gx == dc_szx - 1) {
			x = dc_szx - 2;
		} else {
			x = gx;
		}
	} else {	// EDGE_DONUT
		y = gy;
		x = gx;
	}
#define	D_f	(d_flny[y] * d_lnx[x])

	ppnow = y * dc_szx + x;
	pnow = d_pbasenow + ppnow;
	pprb = ppnow + dc_szx + 1;
	if (dc_szxy <= pprb) {
		pprb = pprb - dc_szxy;
	}
#define		Pnxt 	(d_pbasenxt + gy * dc_szx + gx)
#define		D_unow	d_u[pnow]
#define		D_vnow	d_v[pnow]
	uvv	= D_unow * D_vnow * D_vnow;
	d_u[Pnxt]					// Unxt(gy, gx) 
		= D_unow + DT * ((d_lsu[ppnow] + d_lsu[pprb]
						  - 4. * d_u[d_pbasenow + ppnow]) * dc_du_dxdx
						 + (-uvv + D_f * (1. - D_unow))
                        );
	d_v[Pnxt]					// Vnxt(gy, gx)
		= D_vnow + DT * ((d_lsv[ppnow] + d_lsv[pprb]
						  - 4. * d_v[d_pbasenow + ppnow]) * dc_dv_dxdx
						 + (uvv - (D_f + dc_k) * D_vnow)
						);

	return;
}
#endif	// CUDALSOPT


#if defined(CUDABLOPT)	// block optimized
__global__ void
cudagsblk(float *d_u, float *d_v, float *d_flny, float *d_lnx, int d_pswitch)
{
	int		by, bx;
	int		gy, gx;
	int		tx, ty, pbx, pby;
	float	uvv, lu, lv;
	int		d_pbasenow = (d_pswitch ? dc_pbase1 : dc_pbase0);
	int		d_pbasenxt = (d_pswitch ? dc_pbase0 : dc_pbase1);
	__shared__ float	ds_bu[CUDA_BLOCKSIZE][CUDA_BLOCKSIZE],
						ds_bv[CUDA_BLOCKSIZE][CUDA_BLOCKSIZE];

// this kernel doubled: index size be half of blocksize, same operation to
// left & right half at once corresponding to an index.
// reduce to half # of threads (only 2% faster...)

	by  = threadIdx.y;
	gy  = blockIdx.y * (blockDim.y - 2) + threadIdx.y - 1;
	bx = threadIdx.x;
//	gx = blockIdx.x * (blockDim.x - 2) + threadIdx.x - 1;
	gx = blockIdx.x * (CUDA_BLOCKSIZE - 2) + threadIdx.x - 1;
	if (dc_szy < gy || dc_szx < gx) {	// for block padding threads
		return;
	}

//........................................................................
// transfer (u, v) from global to shared memory
	tx = gx;
	ty = gy;
	if (tx < 0) {
		tx += dc_szx;
	} else if (dc_szx <= tx) {
		tx -= dc_szx;
	}
	if (ty < 0) {
		ty += dc_szy;
	} else if (dc_szy == ty) {
		ty -= dc_szy;
	}

#define	tnow	(d_pbasenow + ty * dc_szx + tx)
	ds_bu[by][bx] = d_u[tnow];
	ds_bv[by][bx] = d_v[tnow];
//........................................................................
	bx = threadIdx.x + CUDA_BLOCKSIZE / 2;
	tx = gx + CUDA_BLOCKSIZE / 2;
//	if (tx < 0) {
//		tx += dc_szx;
//	} else
	 if (dc_szx <= tx) {
		tx -= dc_szx;
	}
	ds_bu[by][bx] = d_u[tnow];
	ds_bv[by][bx] = d_v[tnow];

	__syncthreads();
//........................................................................
	bx = threadIdx.x;

// iteration:

// (pbx, pby) is iteration point in block,
// (gx, gy) is target point in global matrix.
	pbx = bx;
	pby = by;
	if (dc_edgetype == EDGE_NEUMANN) {
		if (gy == 0) {
			pby = pby + 1;	// should be 3
		} else if (gy == dc_szy - 1) {
			pby = pby - 1;
		}
		if (gx == 0) {
			pbx = pbx + 1;	// should be 3
		} else if (gx == dc_szx - 1) {
			pbx = pbx - 1;
		}
	}

#define		Pnxt 	(d_pbasenxt + gy * dc_szx + gx)
#define		D_f		(d_flny[gy] * d_lnx[gx])
	if (0 < bx && bx < CUDA_BLOCKSIZE - 1 &&
		0 < by && by < CUDA_BLOCKSIZE - 1 &&
			gx < dc_szx && gy < dc_szy) {
				// gx, gy should always be >0 if bx, by >=1.

		lu = (
			ds_bu[pby - 1][pbx]		// Unow(y - 1, x)
			+ ds_bu[pby][pbx - 1]	// Unow(y, x - 1)
			+ ds_bu[pby][pbx + 1]	// Unow(y, x + 1)
			+ ds_bu[pby + 1][pbx]	// Unow(y + 1, x)
			- 4. * ds_bu[pby][pbx]	// Unow(y, x)
		) * dc_du_dxdx;
		lv = (
			ds_bv[pby - 1][pbx]		// Vnow(y - 1, x)
			+ ds_bv[pby][pbx - 1]	// Vnow(y, x - 1)
			+ ds_bv[pby][pbx + 1]	// Vnow(y, x + 1)
			+ ds_bv[pby + 1][pbx]	// Vnow(y + 1, x)
			- 4. * ds_bv[pby][pbx]	// Vnow(y, x)
		) * dc_dv_dxdx;
		uvv	= ds_bu[pby][pbx] * ds_bv[pby][pbx] * ds_bv[pby][pbx];
		d_u[Pnxt]					// Unxt(gy, gx) 
			= ds_bu[pby][pbx] +
				DT * (lu - uvv + D_f * (1. - ds_bu[pby][pbx]));
		d_v[Pnxt]					// Vnxt(gy, gx)
			= ds_bv[pby][pbx] +
				DT * (lv + uvv - (D_f + dc_k) * ds_bv[pby][pbx]);
	}

//........................................................................
	bx += CUDA_BLOCKSIZE / 2;
//    gx = blockIdx.x * (blockDim.x - 2) + threadIdx.x - 1;
	gx += CUDA_BLOCKSIZE / 2;

	if (dc_szx <= gx) {	// for block padding threads
		return;
	}

	pbx = bx;
//	pby = by;
	if (dc_edgetype == EDGE_NEUMANN) {
//		if (gy == 0) {
//			pby = pby + 1;
//		} else if (gy == dc_szy - 1) {
//			pby = pby - 1;
//		}
		if (gx == 0) {
			pbx = pbx + 1;
		} else if (gx == dc_szx - 1) {
			pbx = pbx - 1;
		}
	}

//#define		Pnxt 	(d_pbasenxt + gy * dc_szx + gx)
//#define		D_f		(d_flny[gy] * d_lnx[gx])
	if (0 < bx && bx < CUDA_BLOCKSIZE - 1 &&
		0 < by && by < CUDA_BLOCKSIZE - 1 &&
			gx < dc_szx && gy < dc_szy) {
				// gx, gy should always be >0 if bx, by >=1.

		lu = (
			ds_bu[pby - 1][pbx]		// Unow(y - 1, x)
			+ ds_bu[pby][pbx - 1]	// Unow(y, x - 1)
			+ ds_bu[pby][pbx + 1]	// Unow(y, x + 1)
			+ ds_bu[pby + 1][pbx]	// Unow(y + 1, x)
			- 4. * ds_bu[pby][pbx]	// Unow(y, x)
		) * dc_du_dxdx;
		lv = (
			ds_bv[pby - 1][pbx]		// Vnow(y - 1, x)
			+ ds_bv[pby][pbx - 1]	// Vnow(y, x - 1)
			+ ds_bv[pby][pbx + 1]	// Vnow(y, x + 1)
			+ ds_bv[pby + 1][pbx]	// Vnow(y + 1, x)
			- 4. * ds_bv[pby][pbx]	// Vnow(y, x)
		) * dc_dv_dxdx;
		uvv	= ds_bu[pby][pbx] * ds_bv[pby][pbx] * ds_bv[pby][pbx];
		d_u[Pnxt]					// Unxt(gy, gx) 
			= ds_bu[pby][pbx] +
				DT * (lu - uvv + D_f * (1. - ds_bu[pby][pbx]));
		d_v[Pnxt]					// Vnxt(gy, gx)
			= ds_bv[pby][pbx] +
				DT * (lv + uvv - (D_f + dc_k) * ds_bv[pby][pbx]);
	}
//........................................................................

	return;
}
#endif	// optimize


#if defined(CUDAMAP)

void
cudamapdownload(void)
{
	cudamapbgr();
// download map
	if (cudaMemcpy((img.ptr<cv::Vec3b>(0)), d_map, ZZMAP,
			cudaMemcpyDeviceToHost) != cudaSuccess
	   ) {
		exit(42);
	}
//for (int i = 0; i < 320*160*3; i++) {
//(img.ptr<unsigned char>(0, 0))[i] = 128;
//}
	return;
}


void
cudamapbgr(void)
{
	Float	umin, umax, vmin, vmax,
			udif, vdif, t;
	cudaError_t		stat;

	dim3	mapdimBlock(CUDA_BLOCKSIZE, CUDA_BLOCKSIZE);
	dim3	mapdimGrid((szx + CUDA_BLOCKSIZE - 1) / (CUDA_BLOCKSIZE),
			        (szy + CUDA_BLOCKSIZE - 1) / (CUDA_BLOCKSIZE));

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

	cudamapk<<<mapdimGrid, mapdimBlock>>>(d_u, d_v, d_map, pswitch,
		udif, umin, vdif, vmin);
	if ((stat = cudaGetLastError()) != cudaSuccess) {
		fprintf(stderr, "(kernel) %s\n", cudaGetErrorString(stat));
		exit(43);
	}

	return;
}


__global__ void
cudamapk(float *d_u, float *d_v, unsigned char *d_map, int d_pswitch,
	float d_udif, float d_umin, float d_vdif, float d_vmin)
{
	int		y = blockIdx.y * blockDim.y + threadIdx.y;
    int		x = blockIdx.x * blockDim.x + threadIdx.x;

	int		pnow;
	unsigned char		pr, pb, pg;

	pnow = (d_pswitch ? dc_pbase1 : dc_pbase0) + y * dc_szx + x;
#define		D_unow	d_u[pnow]
#define		D_vnow	d_v[pnow]
	pr = (unsigned char)((D_unow - d_umin) / d_udif * 255.);
	pb = (unsigned char)((D_vnow - d_vmin) / d_vdif * 255.);
	pg = (unsigned char)(255 - abs((int)pr - (int)pb));

	for (int yy = 0; yy < dc_mag; yy++) {
		for (int xx = 0; xx < dc_mag; xx++) {
			d_map[((y * dc_mag + yy) * (dc_szx * dc_mag)
					 + (x * dc_mag + xx)) * 3    ]
//				= (unsigned char)(x * 4);
				= pb;
			d_map[((y * dc_mag + yy) * (dc_szx * dc_mag)
					 + (x * dc_mag + xx)) * 3 + 1]
//				= (unsigned char)(x * 2);
				= pg;
			d_map[((y * dc_mag + yy) * (dc_szx * dc_mag)
					 + (x * dc_mag + xx)) * 3 + 2]
//				= (unsigned char)(x * 4);
				= pr;
		}
	}
	return;
}
#endif	// CUDAMAP


#if defined(CUDAFMM)
void
cudafindminmax(Float *umin, Float *umax, Float *vmin, Float *vmax)
{
// find (min, max) of each part #0..#(nsrch - 1) sized CUDA_FMMSIZE in d_u[],
//   and put into d_minmaxarr[#part * 2], d_minmaxarr[#part * 2 + 1]

//	if (CUDA_MAXBLOCKSIZE < szy) {
//		fprintf(stderr, "too large y size (cudafindminmax()).\n");
//		exit(45);
//	}
	int		nsrch = (szxy + CUDA_FMMSIZE - 1) / CUDA_FMMSIZE;
	dim3	mmdimBlock1(nsrch, 1);
	dim3	mmdimGrid1(1, 1);

	cudafindminmaxk<<<mmdimGrid1, mmdimBlock1>>>
		(d_u + (pswitch ? pbase1 : pbase0), CUDA_FMMSIZE, d_minmaxarr);

// find (min, max) in d_minmaxarr[0..(nsrch * 2 - 1)]
//   and put into minmaxres[0], minmaxres[1]
// note: here we don't have to use threads (#threads is 1),
//       but we can utilize same routine.
	dim3	mmdimBlock2(1, 1);
	dim3	mmdimGrid2(1, 1);
	cudafindminmaxk<<<mmdimGrid2, mmdimBlock2>>>
		(d_minmaxarr, nsrch * 2, d_minmaxres);

// same as above, on d_v[]
//	dim3	mmdimBlock1(CUDA_BLOCKSIZE, CUDA_BLOCKSIZE);
//	dim3	mmdimGrid1((szy + CUDA_BLOCKSIZE - 1) / (CUDA_BLOCKSIZE), 1);
	cudafindminmaxk<<<mmdimGrid1, mmdimBlock1>>>
		(d_v + (pswitch ? pbase1 : pbase0), CUDA_FMMSIZE, d_minmaxarr);

//	dim3	mmdimBlock2(1, 1);
//	dim3	mmdimGrid2(1, 1);
	cudafindminmaxk<<<mmdimGrid2, mmdimBlock2>>>
		(d_minmaxarr, nsrch * 2, d_minmaxres + 2);

// now d_minmaxres[0..3] is {umin, umax, vmin, vmax}.
	if (cudaMemcpy(minmaxres, d_minmaxres, sizeof(float) * 4,
			cudaMemcpyDeviceToHost) != cudaSuccess) {
		exit(42);
	}
	*umin = minmaxres[0];
	*umax = minmaxres[1];
	*vmin = minmaxres[2];
	*vmax = minmaxres[3];
	
	return;
}


__global__ void
cudafindminmaxk(float source[], int srchsize, float result[])
{
	int		part = threadIdx.x;
	float	min = 99., max = -99.;

	for (int i = part * srchsize; i < (part + 1) * srchsize; i++) {
		if (dc_szxy <= i) {	// protect exceeding u[]/v[] when
			break;			// szxy is not multiply of CUDA_FMMSIZE.
		}					// when summarize, srchsize never exceeds szxy.
		if (source[i] < min) {
			min = source[i];
		}
		if (max < source[i]) {
			max = source[i];
		}
	}
	result[part * 2    ] = min;
	result[part * 2 + 1] = max;

	return;
}
#endif	// CUDAFMM

#endif	// CUDA
