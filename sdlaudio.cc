#include	"common.h"

#if defined(SDLAUDIO)	// whole this file

//#include	<SDL.h>
static int		aubufsendp = 0;


void
sdlaudioinit(void)
{
	SDL_AudioSpec		want, have;

#if ! defined(USESDL)	// SDLAUDIO but not SDL graphics, SDL_Init() here
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		exit(4);
	}
#endif

	if ((aubuf = (Uint8 *)malloc(szxy * 2 * sizeof(Uint8))) == NULL) {
		fprintf(stderr, "SDL audio malloc failed, cannot play audio.\n");
		withaudio = 0;
	}
//	if ((int)sizeof(aubuf) < szxy * 2) {
//		fprintf(stderr,
//			"(warning) too big image (> %d), cannot play audio.\n",
//			(int)sizeof(aubuf));
//		withaudio = 0;
//	}
	if (withaudio) {
		if (40000 <= szxy) {
			aubufsize = 8192;
		} else {
			aubufsize = 4096;
		}
		if (szxy < aubufsize / 2 / (int)sizeof(Uint8)) {
			aubufsize = szxy * 2 * (int)sizeof(Uint8);
		}
		aubuflow = aubufsize / 2;
		
		want.freq = SDLAUDIO_FREQ;
		want.format = AUDIO_U8;
		want.channels = 2;
		want.samples = aubufsize / 2 / sizeof(Uint8);
		want.callback = NULL;
		if (audiodevno < 0 || SDL_GetNumAudioDevices(0) <= audiodevno) {
			fprintf(stderr, "no such audio device #%d\n", audiodevno);
			exit(71);
		}
		if ((audev = SDL_OpenAudioDevice(
				SDL_GetAudioDeviceName(audiodevno, 0),
				0, &want, &have,
				SDL_AUDIO_ALLOW_ANY_CHANGE)) == 0) {
			fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
			exit(71);
		}
		fprintf(stderr, "audio device ch %d (%s): "
			"freq %d, format %d(/%d), "
			"channels %d, samples %d, callback %p\n",
			audiodevno, // (int)audev,
			SDL_GetAudioDeviceName(audiodevno, 0),
			have.freq, (int)have.format, (int)want.format,
			(int)have.channels, (int)have.samples, have.callback);
		if (want.freq != have.freq || want.format != have.format ||
				want.channels != have.channels) {
			fprintf(stderr,
				"(warning) have freq/fmt/ch differs from want "
				"(don't play audio)\n");
			withaudio = 0;
			SDL_CloseAudioDevice(audev);
		}
//		SDL_PauseAudioDevice(audev, 1);
//		SDL_Delay(100);
		SDL_PauseAudioDevice(audev, 0);
	}
	steleosw = 0;

	return;
}


void
sdlqueueaudio(void)
{
	int queuedsize;
	int vacantsize;

	if (withaudio) {
		queuedsize = (int)SDL_GetQueuedAudioSize(audev);
//fprintf(stderr, "[%d/%d]", queuedsize, aubufsize);
		if (queuedsize <= aubuflow) {
			vacantsize = aubufsize - queuedsize;
//fprintf(stderr, "%d--", aubufsendp);
			if (aubufsendp + vacantsize <= szxy * 2 * (int)sizeof(Uint8)) {
				SDL_QueueAudio(audev, aubuf + aubufsendp, vacantsize);
//fprintf(stderr, "%d ", vacantsize);
//fprintf(stderr, "\n");
				aubufsendp = (aubufsendp + vacantsize) %
								(szxy * 2 * sizeof(Uint8));
			} else {
				SDL_QueueAudio(audev, aubuf + aubufsendp,
					szxy * 2 * sizeof(Uint8) - aubufsendp);
//fprintf(stderr, "%d+", (int)(szxy * 2 * sizeof(Uint8) - aubufsendp));
				SDL_QueueAudio(audev, aubuf,
					vacantsize - (szxy * 2 * sizeof(Uint8) - aubufsendp));
//fprintf(stderr, "%d ",
//(int)(vacantsize - (szxy * 2 * sizeof(Uint8) - aubufsendp)));
				aubufsendp =
					vacantsize - (szxy * 2 * sizeof(Uint8)- aubufsendp);
			}
////			SDL_PauseAudioDevice(audev, 1);
////			SDL_Delay(10);
////			SDL_PauseAudioDevice(audev, 0);
//			fprintf(stderr, "queue: %u\n", SDL_GetQueuedAudioSize(audev));
		} else {
//			fprintf(stderr, "skip %u\n", SDL_GetQueuedAudioSize(audev));
		}
	}
	steleosw = (steleosw + 1) % 2;

	return;
}


void
sdlaudioquit(void)
{
	SDL_CloseAudioDevice(audev);
	free(aubuf);

	return;
}

#endif
