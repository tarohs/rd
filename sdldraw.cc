#include	"common.h"

#if defined(USESDL)	// whole this file

void	sdlmousedot(int rx, int ry, int radius);
#if defined(CUDAFMM)
void	cudafindminmax(void);
#endif
void	sdldrawfancy(void);

//#include	<SDL.h>
#include	<SDL_keycode.h>
#include	<SDL_ttf.h>
#if defined(USEGL)
# include	<SDL_opengl.h>
# include	<glu.h>
#endif

SDL_Window *window;
SDL_Renderer *renderer;
SDL_GLContext	glcontext;

// Stuff for text rendering
TTF_Font    *font;
SDL_Color textColor = {TXTCOL2, 255};
SDL_Color tedgeColor = {TXTCOL1, 255};
SDL_Texture *texttexture, *tedgetexture;
SDL_Rect textrect, tedgerect;


void
sdlinit(void)
{
	SDL_RendererInfo	rendererinfo;

// enable double buffering
#if defined(USEGL)
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif

	if (
#if defined(SDLAUDIO)	// both USESDL && SDLAUDIO (SDL_Init() here)
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)
#else
		SDL_Init(SDL_INIT_VIDEO)
#endif
	   != 0) {
		exit(4);
	}
//	if (SDL_CreateWindowAndRenderer(szx * mag, szy * mag,
//			SDL_WINDOW_BORDERLESS,
//			&window, &renderer) != 0) {
//		fprintf(stderr, "SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
//		exit(61);
//	}
	windowtitle = "Gray-Scott";
	if ((window = SDL_CreateWindow(windowtitle, 0, 0, szx * mag, szy * mag,
			SDL_WINDOW_BORDERLESS
#if defined(USEGL)
			| SDL_WINDOW_OPENGL
#endif
			)) == NULL) {
		fprintf(stderr, "SDL_CreateWindow(): %s\n", SDL_GetError());
		exit(61);
	}
	if ((renderer = SDL_CreateRenderer(window, -1, 0)) == NULL) {
		fprintf(stderr, "SDL_CreateRenderer(): %s\n", SDL_GetError());
		exit(62);
	}
#if defined(USEGL)
// create OpenGL context
    if ((glcontext = SDL_GL_CreateContext(window)) == NULL) {
    	exit(63);
	}
#endif

//	SDL_SetWindowTitle(window, windowtitle);
//	SDL_SetWindowTitle(window, windowtitle);
	SDL_GetRendererInfo(renderer, &rendererinfo);
	fprintf(stderr, "SDL renderer: %s\n", rendererinfo.name);

	if (dispparamname) {
    	if (TTF_Init() == -1) {
        	exit(63);
    	}
		if ((font = TTF_OpenFont(SDLFONTPATH, SDLFONTSIZE)) == nullptr) {
			fprintf(stderr, "TTF_OpenFont(%s, %d): %s\n",
				SDLFONTPATH, SDLFONTSIZE, TTF_GetError());
			exit(62);
		}
	}

	return;
}


// we don't need callback for mause (polled)
/*
void
sdlmousedot(int event, int rx, int ry, int flags, void *param)
{
	return;
}
*/


// actually key and mouse
void
sdlgetkey(void)
{
	SDL_Event	event;
	int		kc, key;
	int		mx, my;

	while (SDL_PollEvent(&event)) {		// thread unsafe!!
		if (event.type == SDL_KEYDOWN) {
			key = '\0';
			kc = event.key.keysym.sym & 0xff;
			if ((SDLK_0 <= kc && kc <= SDLK_9) ||
				 kc == SDLK_ESCAPE ||
				 kc == SDLK_SPACE) {
				key = kc;
			} else if (SDLK_a <= kc && kc <= SDLK_z) {
				if (event.key.keysym.mod & KMOD_SHIFT) {
					key = kc - 'a' + 'A';
				} else {
					key = kc;
				}
			}
			reactkey(key);
		} else if ((event.type == SDL_MOUSEBUTTONDOWN &&
					event.button.button == SDL_BUTTON_LEFT) ||
				   (event.type == SDL_MOUSEMOTION &&
					event.motion.state & SDL_BUTTON_LMASK)
   	           ) {
					SDL_GetMouseState(&mx, &my);
					sdlmousedot(mx, my, MOUSEDOT);
		} else if ((event.type == SDL_FINGERDOWN &&
					event.tfinger.pressure > .1) ||
				   (event.type == SDL_FINGERMOTION &&
					event.tfinger.pressure > .1)
   	           ) {
					mx = (int)(event.tfinger.x);
					my = (int)(event.tfinger.y);
					sdlmousedot(mx, my,
						(int)(event.tfinger.pressure * (float)MOUSEDOT));
		}
	}

	return;
}


void
sdlmousedot(int rx, int ry, int radius)
{
	rx /= mag;
	ry /= mag;
	if (rx < 0 || szx <= rx ||
		ry < 0 || szy <= ry ||
		(rx == 0 && ry == 0)) {
// bug: SDL touchpad slide causes setting dot at (0, 0), so omit this.
		return;
	}
	drawdot(rx, ry, radius);
	isreact = 1;

	return;
}


void
sdlmapbgr(void)
{
	Float	umin, umax, vmin, vmax,
			udif, vdif;
#if ! defined(USEGL)
	SDL_Rect	rect;
#endif

	Uint8	pr, pg, pb;
//	static uchar	map[MAXSZ][MAXSZ][3];

#if ! defined(USEGL)
	rect.w = mag;
	rect.h = mag;
#endif

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

	Audioresetp;

	for (Int yy = 0; yy < szy; yy++) {
#if ! defined(USEGL)
		rect.y = yy * mag;
#endif
		for (Int xx = 0; xx < szx; xx++) {
#if defined(QUANTIZE)
			pr = (Uint8)(
						(Float)((int)((Unow(yy, xx) - umin) / udif * 8.))
						* (255. / 8.));
			pb = (Uint8)(
						(Float)((int)((Vnow(yy, xx) - vmin) / vdif * 8.))
						* (255. / 8.));
#else
			pr = (Uint8)((Unow(yy, xx) - umin) / udif * 255.);
			pb = (Uint8)((Vnow(yy, xx) - vmin) / vdif * 255.);
#endif
			pg = (Uint8)(255 - abs((int)pr - (int)pb));
#if defined(USEGL)
			glColor3ub(pr, pg, pb);
			glRecti(xx * mag, yy * mag, (xx + 1) * mag, (yy + 1) * mag);
#else
			rect.x = xx * mag;
    		SDL_SetRenderDrawColor(renderer, pr, pg, pb, 255);
			SDL_RenderFillRect(renderer, &rect);
#endif

			Audioset(pb);
		}
	}
	Audioqueue;

	return;
}


void
sdlprintparamsetname(const char *name)
{
	SDL_Surface* textsur  = TTF_RenderText_Blended(font, name, textColor);
	SDL_Surface* tedgesur = TTF_RenderText_Blended(font, name, tedgeColor);
//	SDL_Surface* blended = TTF_RenderText_Solid(font, name, textColor);

	texttexture = SDL_CreateTextureFromSurface(renderer, textsur);
	SDL_FreeSurface(textsur);
	tedgetexture = SDL_CreateTextureFromSurface(renderer, tedgesur);
	SDL_FreeSurface(tedgesur);

	SDL_QueryTexture(texttexture, NULL, NULL, &textrect.w, &textrect.h);
	textrect.x = SDLTXTCRDX;
	textrect.y = SDLTXTCRDY;
	SDL_QueryTexture(texttexture, NULL, NULL, &tedgerect.w, &tedgerect.h);
	tedgerect.x = SDLTXTCRDX + SDLTEDGEOFFSETX;
	tedgerect.y = SDLTXTCRDY + SDLTEDGEOFFSETY;

	return;
}


void
sdldrawfancy(void)
{
//	SDL_Surface	*txtimage;
//
	if (dispparamname) {
		SDL_RenderCopy(renderer, tedgetexture, nullptr, &tedgerect); 
		SDL_RenderCopy(renderer, texttexture, nullptr, &textrect); 

//		txtfont = TTF_OpenFont(SDLFACENAME, SDLTXTSCL);
//		txtimage = TTF_RenderUTF8_Blended(txtfont, "test", white);
//		
//		
//		cv::putText(im, (cv::String)Currentparamset.name, TXTCRD,
//			TXTFACE, TXTSCL, TXTCOL1, TXTTHK1, CV_AA, false);
//		cv::putText(im, (cv::String)Currentparamset.name, TXTCRD,
//			TXTFACE, TXTSCL, TXTCOL2, TXTTHK2, CV_AA, false);
	}
	if (edgetype == EDGE_NEUMANN) {
		SDL_Rect	rect;
    	SDL_SetRenderDrawColor(renderer, EDGENEU_COLOR, 255);
		for (int i = 0; i < EDGENEU_BORDER; i++) {
			rect.x = 0 + i;
			rect.y = 0 + i;
			rect.w = szx * mag - 1 - i;
			rect.h = szy * mag - 1 - i;
			SDL_RenderDrawRect(renderer, &rect);
		}
	}
	return;
}


void
sdlshow(void)
{
#if ! defined(USEGL)
	sdldrawfancy();
	SDL_RenderPresent(renderer);
#else
	SDL_GL_SwapWindow(window);
#endif

	return;
}


void
sdlquit(void)
{
#if defined(USEGL)
	SDL_GL_DeleteContext(glcontext);
#endif

#if defined(SDLAUDIO)
	SDL_ClearQueuedAudio(audev);
	SDL_CloseAudioDevice(audev);
#endif

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return;
}
#endif	// USESDL
