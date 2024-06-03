#include "SDL3/SDL_audio.h"
#include "editor.h"

YUinstance			inst = {0};
int					WINDOW_WIDTH = 1200;
int					WINDOW_HEIGHT = 800;
int					running = 1;
int					g_saving = 1;
void				*g_buffer;

#define AUDIO_FORMAT SDL_AUDIO_F32LE
#define AUDIO_CHANNELS 1
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 4096

typedef struct 
{
    Uint8 *buffer;
    Uint32 length;
    Uint32 position;
} AudioData;

int
getAudioDevList(int *micSample, SDL_AudioSpec *micSpec)
{
    SDL_AudioDeviceID *adev;
	char *name;
	int micID = 0;
	int device_list;

	if (!(adev = SDL_GetAudioCaptureDevices(&device_list)))
	{
		fprintf(stderr, "Couldnt getaudiocapturedevices\n");
		exit(1);
	}

	 //  NOTE: JE SUIS UNE MERDE J'AI PERDU DU TEMPS POUR CA 
	void *temp = adev;
	while (*adev)
	{
		name = SDL_GetAudioDeviceName(*adev);
		if (name && strstr(name, "Universal Audio"))
		{
			/* printf("\n\nDevice capture %d: %s\n\n", *adev, name); */
			micID = *adev;
		}
		adev++;
	}
	SDL_free(temp);

	// Output
	if (!(adev = SDL_GetAudioOutputDevices(&device_list)))
	{
		fprintf(stderr, "Couldnt getaudiocapturedevices\n");
		exit(1);
	}

	temp = adev;
	while (*adev)
	{
		/* printf("Device output %d: %s\n", *adev, SDL_GetAudioDeviceName(*adev)); */
		adev++;
	}
	SDL_free(temp);

	if (micID == 0)
	{
		printf("We got a zero!!!!!!!!!!\n");
		micID = SDL_AUDIO_DEVICE_DEFAULT_OUTPUT;
	}
	if ((SDL_GetAudioDeviceFormat(micID, micSpec, micSample) < 0))
	{
		fprintf(stderr, "GetFormat %s\n", SDL_GetError());
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}
	return micID;
}

void
print_mic_info(SDL_AudioSpec micSpec, int micSample)
{
	printf("samples: %d\nchannels: %d\nfreq: %d\nformat: %us\n", 
			micSample, micSpec.channels, micSpec.freq, micSpec.format);

	printf("bit: %d\n", SDL_AUDIO_BITSIZE(micSpec.format));
	printf("byte: %d\n", SDL_AUDIO_BYTESIZE(micSpec.format));
	printf("is little endian?: %d\n", SDL_AUDIO_ISLITTLEENDIAN(micSpec.format));
	printf("isint?: %d\n", SDL_AUDIO_ISINT(micSpec.format));
	printf("issigned?: %d\n", SDL_AUDIO_ISSIGNED(micSpec.format));
	/* exit(1); */
}

void 
init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		fprintf(stderr, "%s\n", SDL_GetError());
		exit(1);
	}

	inst.window = SDL_CreateWindow("Key capture", WINDOW_WIDTH, WINDOW_HEIGHT,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (inst.window == NULL)
	{
		fprintf(stderr, "%s\n", SDL_GetError());
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}

	SDL_AudioSpec micSpec = {0};
	int micSample = 0;

	// change cette merde
	int micID = getAudioDevList(&micSample, &micSpec);

    SDL_AudioDeviceID outputDevID;
    SDL_AudioDeviceID captureDevID;

	/* use this ??
	 *
	 * SDL_OpenAudioDeviceStream
	 * (SDL_AudioDeviceID devid, const SDL_AudioSpec *spec,
	 * SDL_AudioStreamCallback callback, void *userdata);
	 *
	 */

	print_mic_info(micSpec, micSample);
    /*
	 * micSpec.channels = 1;
	 * micSpec.freq = 44100;
     */
    outputDevID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_OUTPUT, 0);
    captureDevID = SDL_OpenAudioDevice(micID, 0);

	printf("\nOutput: \"%s\"\n", SDL_GetAudioDeviceName(outputDevID));
	printf("Capture: \"%s\"\n\n", SDL_GetAudioDeviceName(captureDevID));

    if (!outputDevID || !captureDevID)
	{
        fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
        SDL_DestroyWindow(inst.window);
		TTF_Quit();
        SDL_Quit();
        exit(1);
    }
	SDL_AudioStream *stream = SDL_CreateAudioStream(&micSpec, &micSpec);
	if (!stream)
	{
        fprintf(stderr, "Failed to create audio stream: %s\n", SDL_GetError());
        SDL_DestroyWindow(inst.window);
		TTF_Quit();
        SDL_Quit();
        exit(1);
	}
	if (SDL_BindAudioStream(captureDevID, stream) == -1)
	{
        fprintf(stderr, "Failed to bind stream: %s\n", SDL_GetError());
        SDL_DestroyWindow(inst.window);
		TTF_Quit();
        SDL_Quit();
        exit(1);
	}
	inst.renderer = SDL_CreateRenderer(inst.window,NULL);
	if (inst.renderer == NULL)
	{
		fprintf(stderr, "%s\n", SDL_GetError());
		SDL_DestroyWindow(inst.window);
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}

	inst.stream = stream;
	inst.oDevID = outputDevID;
	inst.cDevID = captureDevID;
}

void
save_file(FILE *file)
{
    /*
	 * if (g_saving)
	 * 	return;
     */
	size_t bytes_read = 0;
	size_t bytes_written = 0;
	size_t bytes_queued = 0;
	size_t bytes_available = 0;
	/* void *audioBuf[BUFLEN]; */
	void *audioBuf = g_buffer;

	bytes_queued = SDL_GetAudioStreamQueued(inst.stream);
	bytes_available = SDL_GetAudioStreamAvailable(inst.stream);
	if (bytes_queued == 0 || bytes_available == 0)
	{
		printf("bytes_available = %llu\t", bytes_available);
		printf("bytes_queued = %llu\n", bytes_queued);
		return;
	}

	bytes_read = SDL_GetAudioStreamData(inst.stream, audioBuf, 4096*20);

	if (bytes_read < -1)
	{
		fprintf(stderr, "No bytes received from AudioStream..\n");
		return ;
	}

	printf("bytes_available = %llu\t", bytes_available);
	printf("bytes_queued = %llu\t", bytes_queued);
	printf("bytes_read = %llu\n", bytes_read);

	bytes_written = fwrite(audioBuf, sizeof(void *), bytes_read, file);
	if (bytes_written < 0)
	{
		perror("fwrite line 138:");
		exit(1);
	}
}

int
/* WinMain() */
main()
{
	init();
	g_buffer = malloc(4096*20);
	inst.audio_file = fopen("test.wav", "wb");
	if (!inst.audio_file) { perror("Error fopen line 151: "); exit(1); }
	/* SDL_SetAudioStreamPutCallback(inst.stream, save_file, inst.audio_file); */
	/* SDL_SetAudioStreamFormat(inst.stream ) */
	SDL_AudioSpec Ismp = {0};
	SDL_AudioSpec Osmp = {0};
	SDL_GetAudioStreamFormat(inst.stream, &Ismp, &Osmp);
	print_mic_info(Ismp, 0);
	printf("-------------\n");
	print_mic_info(Osmp, 0);
	exit(1);
	while (running)
	{
		save_file(inst.audio_file);
		SDL_SetRenderDrawColor(inst.renderer, 50, 50, 50, 255);
		SDL_RenderClear(inst.renderer);
		Events(inst.e);
		SDL_RenderPresent(inst.renderer);
	}

	SDL_DestroyAudioStream(inst.stream);
	SDL_DestroyRenderer(inst.renderer);
	SDL_DestroyWindow(inst.window);
	SDL_Quit();
	fclose(inst.audio_file);
	free(g_buffer);

    /*
	 * _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE);
	 * _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	 * _CrtDumpMemoryLeaks();
     */

	return 0;
}
