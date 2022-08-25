#include	"common.h"
#if defined(USECV)
# include	"cvdraw.h"
#elif defined(USESDL)
# include	<SDL.h>
# include	<SDL_keycode.h>
#endif

#define	PINIT	(.15)
#define	ALIVECOLOR	240, 240, 240

void	coninit(void);
void	condraw(void);
int		conreact(void);
#if defined(USECV)
void	cvconmouse(int event, int rx, int ry, int flags, void *param);
#endif

// devide conway life routine not to affect to main rd system.

#define	ul(x, y)	Bnow((x - 1 + szx) % szx, (y - 1 + szy) % szy)
#define	up(x, y)	Bnow((x     + szx) % szx, (y - 1 + szy) % szy)
#define	ur(x, y)	Bnow((x + 1 + szx) % szx, (y - 1 + szy) % szy)
#define	le(x, y)	Bnow((x - 1 + szx) % szx, (y     + szy) % szy)
//#define	hr(x, y)	Bnow( x                 ,  y                 )
#define	ri(x, y)	Bnow((x + 1 + szx) % szx, (y     + szy) % szy)
#define	dl(x, y)	Bnow((x - 1 + szx) % szx, (y + 1 + szy) % szy)
#define	dn(x, y)	Bnow((x     + szx) % szx, (y + 1 + szy) % szy)
#define	dr(x, y)	Bnow((x + 1 + szx) % szx, (y + 1 + szy) % szy)

#if defined(USESDL)
extern SDL_Window	*window;
extern SDL_Renderer	*renderer;
#endif


void
conwaylife(void)
{
	int		cnt;

	pswitch = 0;
	ishold = 0;
	coninit();
	while (1) {
		while (ishold) {
			conreact();
		}
		for (int y = 0; y < szy; y++) {
			for (int x = 0; x < szx; x++) {
				cnt = ul(x, y) + up(x, y) + ur(x, y) +
					  le(x, y) +            ri(x, y) +
					  dl(x, y) + dn(x, y) + dr(x, y);
				if (cnt <= 1 || 4 <= cnt) {
					Bnxt(x, y) = LIFES_DEAD;
				} else if (cnt == 3) {
					Bnxt(x, y) = LIFES_ALIVE;
				} else {	// cnt == 2
					Bnxt(x, y) = Bnow(x, y);
				}
			}
		}
		condraw();
		pswitch = 1 - pswitch;	// for init with 'x' key, updt gener here
		if (conreact()) {
			break;
		}
	}
	exit(0);
}


void
coninit(void)
{
	if ((lifeboard  = (int *)malloc(szy * szx * 2 * sizeof(int))
        ) == NULL
	   ) {
		exit(30);
	}
	for (int y = 0; y < szy; y++) {
		for (int x = 0; x < szx; x++) {
			if (rand() < (int)(RAND_MAX * PINIT)) {
				Bnow(x, y) = LIFES_ALIVE;
			} else {
				Bnow(x, y) = LIFES_DEAD;
			}
		}
	}
	windowtitle = "Conway life game";
#if defined(USECV)
	cv::namedWindow(windowtitle, CV_WINDOW_AUTOSIZE);
		//CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO
	cvMoveWindow(windowtitle, 0, 0);
	cv::setMouseCallback(windowtitle, cvconmouse);
#elif defined(USESDL)
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		exit(4);
	}
	if (SDL_CreateWindowAndRenderer(szx * mag + 1, szy * mag + 1,
			SDL_WINDOW_BORDERLESS,
			&window, &renderer) != 0) {
		exit(61);
	}
	SDL_SetWindowTitle(window, windowtitle);
#endif

	return;
}


void
condraw(void)
{
#if defined(USECV)
	img = cv::Mat::zeros(cv::Size(
		LIFE_BORDER + szx * mag, LIFE_BORDER + szy * mag),
			 CV_8UC3);
#elif defined(USESDL)
	SDL_Rect	rect;

	rect.w = mag - 1;
	rect.h = mag - 1;
   	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

   	SDL_SetRenderDrawColor(renderer, ALIVECOLOR, 255);
#endif

	for (int y = 0; y < szy; y++) {
#if defined(USESDL)
		rect.y = y * mag + 1;
#endif
		for (int x = 0; x < szx; x++) {
			if (Bnow(x, y) == LIFES_ALIVE) {
#if defined(USECV)
				cv::rectangle(img,
					cv::Point(LIFE_BORDER + x * mag, LIFE_BORDER + y * mag),
					cv::Point(LIFE_BORDER + (x + 1) * mag - LIFE_BORDER - 1,
							  LIFE_BORDER + (y + 1) * mag - LIFE_BORDER - 1),
					cv::Scalar(ALIVECOLOR), CV_FILLED);
#elif defined(USESDL)
					rect.x = x * mag + 1;
					SDL_RenderFillRect(renderer, &rect);
#endif
			}
		}
	}
#if defined(USECV)
	cv::imshow(windowtitle, img);
#elif defined(USESDL)
	SDL_RenderPresent(renderer);
#endif
	return;
}


int
conreact(void)
{
	int		key = '\0';

#if defined(USESDL)
	SDL_Event	event;
	int			kc;
#endif

#if defined(USECV)
	key = cv::waitKey(waitms) & 0xff;

#elif defined(USESDL)
	while (SDL_PollEvent(&event)) {		// thread unsafe!!
		if (event.type == SDL_KEYDOWN) {
			kc = event.key.keysym.sym & 0xff;
			if ((SDLK_0 <= kc && kc <= SDLK_9) || kc == SDLK_ESCAPE) {
				key = kc;
			} else if (SDLK_a <= kc && kc <= SDLK_z) {
				if (event.key.keysym.mod & KMOD_SHIFT) {
					key = kc - 'a' + 'A';
				} else {
					key = kc;
				}
			}
		} else {
			int ry, rx = -1;
			if ((event.type == SDL_MOUSEBUTTONDOWN &&
					event.button.button == SDL_BUTTON_LEFT) ||
				   (event.type == SDL_MOUSEMOTION &&
					event.motion.state & SDL_BUTTON_LMASK)
   	           ) {
				SDL_GetMouseState(&rx, &ry);
			} else if ((event.type == SDL_FINGERDOWN &&
					event.tfinger.pressure > .1) ||
				   (event.type == SDL_FINGERMOTION &&
					event.tfinger.pressure > .1)
   	           ) {
					rx = (int)(event.tfinger.x);
					ry = (int)(event.tfinger.y);
			}
			if (rx != -1) {
				rx /= mag;
				ry /= mag;
				if (Bnow(rx, ry) == LIFES_ALIVE) {
					Bnow(rx, ry) = LIFES_DEAD;
				} else {
					Bnow(rx, ry) = LIFES_ALIVE;
				}
			}
		}
	}
	SDL_Delay(waitms);
#endif
	if (key == ESC) {
		return (1);
	} else if (key == ' ') {
		ishold = 1 - ishold;
	} else if (key == 'x' || key == 'X') {
		coninit();
#if defined(USECV)
	} else if (key == '@') {
		cvcapconlife();
#endif
	}

	return (0);
}


#if defined(USECV)
void
cvconmouse(int event, int rx, int ry, int flags, void *param)
{
	rx = (rx - LIFE_BORDER) / mag;
	ry = (ry - LIFE_BORDER) / mag;
	if (rx < 0 || szx <= rx ||
	    ry < 0 || szy <= ry) {
		return;
	}
	if (event == CV_EVENT_LBUTTONDOWN ||
		(event == CV_EVENT_MOUSEMOVE &&
			 (flags & CV_EVENT_FLAG_LBUTTON) != 0)
       ) {
		if (Bnow(rx, ry) == LIFES_ALIVE) {
			Bnow(rx, ry) = LIFES_DEAD;
		} else {
			Bnow(rx, ry) = LIFES_ALIVE;
		}
	}
	return;
}
#endif
