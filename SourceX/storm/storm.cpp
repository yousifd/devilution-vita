#include "devilution.h"
#include "miniwin/ddraw.h"
#include "stubs.h"
#include <Radon.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_endian.h>
#include <SDL2/SDL_mixer.h>
#include <smacker.h>

#include "DiabloUI/diabloui.h"

namespace dvl {

DWORD nLastError = 0;
bool directFileAccess = false;

static std::string getIniPath()
{
	char path[DVL_MAX_PATH];
	int len = GetModuleFileNameA(ghInst, path, DVL_MAX_PATH);
	path[len - 1] = '/';
	strcat(path, "ux0:app/DEVIL0666/diablo.ini");

	return path;
}

static radon::File ini(getIniPath());
static Mix_Chunk *SFileChunk;

void TranslateFileName(char *dst, int dstLen, const char *src)
{
	for (int i = 0; i < dstLen; i++) {
		char c = *src++;
		dst[i] = c == '\\' ? '/' : c;
		if (!c) {
			break;
		}
	}
}

// BOOL SFileCloseArchive(HANDLE hArchive)
// {
// 	UNIMPLEMENTED();
// }

// BOOL SFileCloseFile(HANDLE hFile)
// {
// 	UNIMPLEMENTED();
// }

BOOL SFileDdaBeginEx(HANDLE hFile, DWORD flags, DWORD mask, unsigned __int32 lDistanceToMove,
    signed __int32 volume, signed int pan, int a7)
{
	DWORD bytestoread = SFileGetFileSize(hFile, 0);
	char *SFXbuffer = (char *)malloc(bytestoread);
	SFileReadFile(hFile, SFXbuffer, bytestoread, NULL, 0);

	SDL_RWops *rw = SDL_RWFromConstMem(SFXbuffer, bytestoread);
	if (rw == NULL) {
		SDL_Log(SDL_GetError());
	}
	SFileChunk = Mix_LoadWAV_RW(rw, 1);
	free(SFXbuffer);

	Mix_Volume(0, MIX_MAX_VOLUME - MIX_MAX_VOLUME * volume / VOLUME_MIN);
	int panned = 255 - 255 * abs(pan) / 10000;
	Mix_SetPanning(0, pan <= 0 ? 255 : panned, pan >= 0 ? 255 : panned);
	Mix_PlayChannel(0, SFileChunk, 0);

	return true;
}

BOOL SFileDdaDestroy()
{
	Mix_FreeChunk(SFileChunk);

	return true;
}

BOOL SFileDdaEnd(HANDLE hFile)
{
	Mix_HaltChannel(0);

	return true;
}

BOOL SFileDdaGetPos(HANDLE hFile, DWORD *current, DWORD *end)
{
	*current = 0;
	*end = 1;

	if (Mix_GetChunk(0) != SFileChunk || !Mix_Playing(0)) {
		*current = *end;
	}

	return true;
}

BOOL SFileDdaInitialize(HANDLE directsound)
{
	return true;
}

BOOL SFileDdaSetVolume(HANDLE hFile, signed int bigvolume, signed int volume)
{
	Mix_VolumeMusic(MIX_MAX_VOLUME - MIX_MAX_VOLUME * bigvolume / VOLUME_MIN);

	return true;
}

BOOL SFileGetFileArchive(HANDLE hFile, HANDLE *archive)
{
	UNIMPLEMENTED();
	return true;
}

// LONG SFileGetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
// {
// 	UNIMPLEMENTED();
// }

// BOOL SFileOpenArchive(const char *szMpqName, DWORD dwPriority, DWORD dwFlags, HANDLE
// *phMpq)
// {
// 	UNIMPLEMENTED();
// }

// Converts ASCII characters to lowercase
// Converts slash (0x2F) / backslash (0x5C) to system file-separator
unsigned char AsciiToLowerTable_Path[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
#ifdef _WIN32
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x5C,
#else
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
#endif
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
#ifdef _WIN32
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
#else
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x2F, 0x5D, 0x5E, 0x5F,
#endif
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

BOOL SFileOpenFile(const char *filename, HANDLE *phFile)
{
	//eprintf("%s: %s\n", __FUNCTION__, filename);

	bool result = false;

	if (directFileAccess) {
		char directPath[DVL_MAX_PATH] = "\0";
		for (int i = 0; i < strlen(filename); i++) {
			directPath[i] = AsciiToLowerTable_Path[filename[i]];
		}
		result = SFileOpenFileEx((HANDLE)0, directPath, 0xFFFFFFFF, phFile);
	}
	if (!result && patch_rt_mpq) {
		result = SFileOpenFileEx((HANDLE)patch_rt_mpq, filename, 0, phFile);
	}
	if (!result) {
		result = SFileOpenFileEx((HANDLE)diabdat_mpq, filename, 0, phFile);
	}

	if (!result || !*phFile) {
		eprintf("%s: Not found: %s\n", __FUNCTION__, filename);
	}
	return result;
}

// BOOL SFileOpenFileEx(HANDLE hMpq, const char *szFileName, DWORD dwSearchScope, HANDLE
// *phFile)
// {
// 	UNIMPLEMENTED();
// }

// BOOL SFileReadFile(HANDLE hFile, void *buffer, DWORD nNumberOfBytesToRead, DWORD *read,
// LONG lpDistanceToMoveHigh)
// {
// 	UNIMPLEMENTED();
// }

BOOL SBmpLoadImage(const char *pszFileName, PALETTEENTRY *pPalette, BYTE *pBuffer, DWORD dwBuffersize, DWORD *pdwWidth, DWORD *dwHeight, DWORD *pdwBpp)
{
	HANDLE hFile;
	size_t size;
	PCXHeader pcxhdr;
	BYTE paldata[256][3];
	BYTE *dataPtr, *fileBuffer;
	BYTE byte;

	if (pdwWidth)
		*pdwWidth = 0;
	if (dwHeight)
		*dwHeight = 0;
	if (pdwBpp)
		*pdwBpp = 0;

	if (!pszFileName || !*pszFileName) {
		return false;
	}

	if (pBuffer && !dwBuffersize) {
		return false;
	}

	if (!pPalette && !pBuffer && !pdwWidth && !dwHeight) {
		return false;
	}

	if (!SFileOpenFile(pszFileName, &hFile)) {
		return false;
	}

	while (strchr(pszFileName, 92))
		pszFileName = strchr(pszFileName, 92) + 1;

	while (strchr(pszFileName + 1, 46))
		pszFileName = strchr(pszFileName, 46);

	// omit all types except PCX
	if (!pszFileName || _strcmpi(pszFileName, ".pcx")) {
		return false;
	}

	if (!SFileReadFile(hFile, &pcxhdr, 128, 0, 0)) {
		SFileCloseFile(hFile);
		return false;
	}

	int width = SDL_SwapLE16(pcxhdr.xmax) - SDL_SwapLE16(pcxhdr.xmin) + 1;
	int height = SDL_SwapLE16(pcxhdr.ymax) - SDL_SwapLE16(pcxhdr.ymin) + 1;

	if (pdwWidth)
		*pdwWidth = width;
	if (dwHeight)
		*dwHeight = height;
	if (pdwBpp)
		*pdwBpp = SDL_SwapLE16(pcxhdr.bitsPerPixel);

	if (!pBuffer) {
		SFileSetFilePointer(hFile, 0, 0, 2);
		fileBuffer = NULL;
	} else {
		size = SFileGetFileSize(hFile, 0) - SFileSetFilePointer(hFile, 0, 0, 1);
		fileBuffer = (BYTE *)malloc(size);
	}

	if (fileBuffer) {
		SFileReadFile(hFile, fileBuffer, size, 0, 0);
		dataPtr = fileBuffer;

		for (int j = 0; j < height; j++) {
			for (int x = 0; x < width; dataPtr++) {
				byte = *dataPtr;
				if (byte < 0xC0) {
					*pBuffer = byte;
					pBuffer++;
					x++;
					continue;
				}
				dataPtr++;

				for (int i = 0; i < (byte & 0x3F); i++) {
					*pBuffer = *dataPtr;
					pBuffer++;
					x++;
				}
			}
		}

		free(fileBuffer);
	}

	if (pPalette && pcxhdr.bitsPerPixel == 8) {
		SFileSetFilePointer(hFile, -768, 0, 1);
		SFileReadFile(hFile, paldata, 768, 0, 0);
		for (int i = 0; i < 256; i++) {
			pPalette[i].peRed = paldata[i][0];
			pPalette[i].peGreen = paldata[i][1];
			pPalette[i].peBlue = paldata[i][2];
			pPalette[i].peFlags = 0;
		}
	}

	SFileCloseFile(hFile);

	return true;
}

// int SFileSetFilePointer(HANDLE, int, HANDLE, int)
// {
// 	UNIMPLEMENTED();
// }

HWND SDrawGetFrameWindow(HWND *sdraw_framewindow)
{
	DUMMY();
	return NULL;
}

// BOOL SDrawManualInitialize(HWND hWnd, LPDIRECTDRAW ddInterface, LPDIRECTDRAWSURFACE
// primarySurface, LPDIRECTDRAWSURFACE surface2, LPDIRECTDRAWSURFACE surface3, LPDIRECTDRAWSURFACE
// backSurface, LPDIRECTDRAWPALETTE ddPalette, HPALETTE hPalette)
//{
//	UNIMPLEMENTED();
//}

void *SMemAlloc(unsigned int amount, char *logfilename, int logline, char defaultValue)
{
	// fprintf(stderr, "%s: %d (%s:%d)\n", __FUNCTION__, amount, logfilename, logline);
	assert(amount != -1u);
	return malloc(amount);
}

BOOL SMemFree(void *location, char *logfilename, int logline, char defaultValue)
{
	// fprintf(stderr, "%s: (%s:%d)\n", __FUNCTION__, logfilename, logline);
	assert(location);
	free(location);
	return true;
}

void *SMemReAlloc(void *location, unsigned int amount, char *logfilename, int logline, char defaultValue)
{
	UNIMPLEMENTED();
}

bool getIniValue(const char *sectionName, const char *keyName, char *string, int stringSize, int *dataSize = NULL)
{
	radon::Section *section = ini.getSection(sectionName);
	if (!section)
		return false;

	radon::Key *key = section->getKey(keyName);
	if (!key)
		return false;

	std::string value = key->getStringValue();
	if (dataSize)
		*dataSize = value.length();

	if (string)
		strncpy(string, value.c_str(), stringSize);

	return true;
}

void setIniValue(const char *sectionName, const char *keyName, char *value, int len = 0)
{
	radon::Section *section = ini.getSection(sectionName);
	if (!section) {
		ini.addSection(sectionName);
		section = ini.getSection(sectionName);
	}

	std::string stringValue(value, len ? len : strlen(value));

	radon::Key *key = section->getKey(keyName);
	if (!key) {
		section->addKey(radon::Key(keyName, stringValue));
	} else {
		key->setValue(stringValue);
	}

	ini.saveToFile();
}

BOOL SRegLoadData(const char *keyname, const char *valuename, int size, LPBYTE lpData, BYTE flags, LPDWORD lpcbData)
{
	return getIniValue(keyname, valuename, (char *)lpData, size, (int *)lpcbData);
}

BOOL SRegLoadString(const char *keyname, const char *valuename, BYTE flags, char *buffer, unsigned int buffersize)
{
	return getIniValue(keyname, valuename, buffer, buffersize);
}

BOOL SRegLoadValue(const char *keyname, const char *valuename, BYTE flags, int *value)
{
	char string[10];
	if (getIniValue(keyname, valuename, string, 10)) {
		*value = strtol(string, NULL, 10);
		return true;
	}

	return false;
}

BOOL SRegSaveData(const char *keyname, const char *valuename, int size, BYTE *lpData, DWORD cbData)
{
	setIniValue(keyname, valuename, (char *)lpData, cbData);

	return true;
}

BOOL SRegSaveString(const char *keyname, const char *valuename, BYTE flags, char *string)
{
	setIniValue(keyname, valuename, string);

	return true;
}

BOOL SRegSaveValue(const char *keyname, const char *valuename, BYTE flags, DWORD result)
{
	char str[10];
	sprintf(str, "%d", result);
	setIniValue(keyname, valuename, str);

	return true;
}

BOOL SVidInitialize(HANDLE video)
{
	DUMMY();
	return true;
}

BOOL SVidDestroy()
{
	DUMMY();
	return true;
}

double SVidFrameEnd;
double SVidFrameLength;
BYTE SVidLoop;
smk SVidSMK;
PALETTEENTRY SVidPreviousPalette[256];
SDL_Palette *SVidPalette;
SDL_Surface *SVidSurface;
BYTE *SVidBuffer;
SDL_AudioDeviceID deviceId;
unsigned long SVidWidth, SVidHeight;

BOOL SVidPlayBegin(char *filename, int a2, int a3, int a4, int a5, int flags, HANDLE *video)
{
	if (flags & 0x10000 || flags & 0x20000000) {
		return false;
	}

	SVidLoop = flags & 0x40000;
	bool enableVideo = !(flags & 0x100000);
	bool enableAudio = !(flags & 0x1000000);
	//0x8 // Non-interlaced
	//0x200, 0x800 // Upscale video
	//0x80000 // Center horizontally
	//0x800000 // Edge detection
	//0x200800 // Clear FB

	SFileOpenFile(filename, video);

	int bytestoread = SFileGetFileSize(*video, 0);
	SVidBuffer = DiabloAllocPtr(bytestoread);
	SFileReadFile(*video, SVidBuffer, bytestoread, NULL, 0);

	SVidSMK = smk_open_memory(SVidBuffer, bytestoread);
	if (SVidSMK == NULL) {
		return false;
	}

	deviceId = 0;
	unsigned char channels[7], depth[7];
	unsigned long rate[7];
	smk_info_audio(SVidSMK, NULL, channels, depth, rate);
	if (enableAudio && depth[0] != 0) {
#ifdef SWITCH
		Mix_CloseAudio();
#endif
		smk_enable_audio(SVidSMK, 0, enableAudio);
		SDL_AudioSpec audioFormat;
		SDL_zero(audioFormat);
		audioFormat.freq = rate[0];
		audioFormat.format = depth[0] == 16 ? AUDIO_S16 : AUDIO_U8;
		audioFormat.channels = channels[0];

		deviceId = SDL_OpenAudioDevice(NULL, 0, &audioFormat, NULL, 0);
		if (deviceId == 0) {
			SDL_Log(SDL_GetError());
#ifdef SWITCH
			Mix_OpenAudio(22050, AUDIO_S16LSB, 2, 1024);
			Mix_AllocateChannels(25);
			Mix_ReserveChannels(1);
#endif
			return false;
		} else {
			SDL_PauseAudioDevice(deviceId, 0); /* start audio playing. */
		}
	}

	unsigned long nFrames;
	smk_info_all(SVidSMK, NULL, &nFrames, &SVidFrameLength);
	smk_info_video(SVidSMK, &SVidWidth, &SVidHeight, NULL);

	smk_enable_video(SVidSMK, enableVideo);
	smk_first(SVidSMK); // Decode first frame

	smk_info_video(SVidSMK, &SVidWidth, &SVidHeight, NULL);
	if (renderer) {
		SDL_DestroyTexture(texture);
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SVidWidth, SVidHeight);
		if (texture == NULL) {
			SDL_Log(SDL_GetError());
		}
		if (SDL_RenderSetLogicalSize(renderer, SVidWidth, SVidHeight) <= -1) {
			SDL_Log(SDL_GetError());
		}
	}
	memcpy(SVidPreviousPalette, orig_palette, 1024);

	// Copy frame to buffer
	SVidSurface = SDL_CreateRGBSurfaceWithFormatFrom(
	    (unsigned char *)smk_get_video(SVidSMK),
	    SVidWidth,
	    SVidHeight,
	    8,
	    SVidWidth,
	    SDL_PIXELFORMAT_INDEX8);
	if (SVidSurface == NULL) {
		SDL_Log(SDL_GetError());
	}

	SVidPalette = SDL_AllocPalette(256);
	if (SVidPalette == NULL) {
		SDL_Log(SDL_GetError());
	}
	if (SDL_SetSurfacePalette(SVidSurface, SVidPalette) <= -1) {
		SDL_Log(SDL_GetError());
		return false;
	}

	SVidFrameEnd = SDL_GetTicks() * 1000 + SVidFrameLength;

	return true;
}

BOOL SVidLoadNextFrame()
{
	SVidFrameEnd += SVidFrameLength;

	if (smk_next(SVidSMK) == SMK_DONE) {
		if (!SVidLoop) {
			return false;
		}

		smk_first(SVidSMK);
	}

	return true;
}

BOOL SVidPlayContinue(void)
{
	if (smk_palette_updated(SVidSMK)) {
		SDL_Color colors[256];
		const unsigned char *palette_data = smk_get_palette(SVidSMK);

		for (int i = 0; i < 256; i++) {
			colors[i].r = palette_data[i * 3 + 0];
			colors[i].g = palette_data[i * 3 + 1];
			colors[i].b = palette_data[i * 3 + 2];
			colors[i].a = SDL_ALPHA_OPAQUE;

			orig_palette[i].peFlags = 0;
			orig_palette[i].peRed = palette_data[i * 3 + 0];
			orig_palette[i].peGreen = palette_data[i * 3 + 1];
			orig_palette[i].peBlue = palette_data[i * 3 + 2];
		}
		memcpy(logical_palette, orig_palette, 1024);

		if (SDL_SetPaletteColors(SVidPalette, colors, 0, 256) <= -1) {
			SDL_Log(SDL_GetError());
			return false;
		}
	}

	if (SDL_GetTicks() * 1000 >= SVidFrameEnd) {
		return SVidLoadNextFrame(); // Skip video and audio if the system is to slow
	}

	if (deviceId && SDL_QueueAudio(deviceId, smk_get_audio(SVidSMK, 0), smk_get_audio_size(SVidSMK, 0)) <= -1) {
		SDL_Log(SDL_GetError());
		return false;
	}

	if (SDL_GetTicks() * 1000 >= SVidFrameEnd) {
		return SVidLoadNextFrame(); // Skip video if the system is to slow
	}

	if (renderer) {
		if (SDL_BlitSurface(SVidSurface, NULL, surface, NULL) <= -1) {
			SDL_Log(SDL_GetError());
			return false;
		}
	} else {
		int factor;
		int wFactor = SCREEN_WIDTH / SVidWidth;
		int hFactor = SCREEN_HEIGHT / SVidHeight;
		if (wFactor > hFactor && SCREEN_HEIGHT > SVidHeight) {
			factor = hFactor;
		} else {
			factor = wFactor;
		}
		int scaledW = SVidWidth * factor;
		int scaledH = SVidHeight * factor;

		SDL_Rect pal_surface_offset = { (SCREEN_WIDTH - scaledW) / 2, (SCREEN_HEIGHT - scaledH) / 2, scaledW, scaledH };
		Uint32 format = SDL_GetWindowPixelFormat(window);
		SDL_Surface *tmp = SDL_ConvertSurfaceFormat(SVidSurface, format, 0);
		if (SDL_BlitScaled(tmp, NULL, surface, &pal_surface_offset) <= -1) {
			SDL_Log(SDL_GetError());
			return false;
		}
		SDL_FreeSurface(tmp);
	}

	bufferUpdated = true;
	lpDDSPrimary->Unlock(NULL);

	double now = SDL_GetTicks() * 1000;
	if (now < SVidFrameEnd) {
		SDL_Delay((SVidFrameEnd - now) / 1000); // wait with next frame if the system is to fast
	}

	return SVidLoadNextFrame();
}

BOOL SVidPlayEnd(HANDLE video)
{
	if (deviceId) {
		SDL_ClearQueuedAudio(deviceId);
		SDL_CloseAudioDevice(deviceId);
#ifdef SWITCH
		Mix_OpenAudio(22050, AUDIO_S16LSB, 2, 1024);
		Mix_AllocateChannels(25);
		Mix_ReserveChannels(1);
#endif
		deviceId = 0;
	}

	if (SVidSMK)
		smk_close(SVidSMK);

	if (SVidBuffer) {
		mem_free_dbg(SVidBuffer);
		SVidBuffer = NULL;
	}

	SDL_FreePalette(SVidPalette);
	SDL_FreeSurface(SVidSurface);

	SFileCloseFile(video);
	video = NULL;

	memcpy(orig_palette, SVidPreviousPalette, 1024);
	if (renderer) {
		SDL_DestroyTexture(texture);
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
		if (texture == NULL) {
			SDL_Log(SDL_GetError());
		}
		if (renderer && SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT) <= -1) {
			SDL_Log(SDL_GetError());
		}
	}

	return true;
}

BOOL SErrDisplayError(DWORD dwErrMsg, const char *logfilename, int logline, const char *message,
    BOOL allowOption, int exitCode)
{
	UNIMPLEMENTED();
}

BOOL SErrGetErrorStr(DWORD dwErrCode, char *buffer, unsigned int bufferchars)
{
	DUMMY();
	return false;
}

DWORD SErrGetLastError()
{
	return nLastError;
}

void SErrSetLastError(DWORD dwErrCode)
{
	nLastError = dwErrCode;
}

void SMemCopy(void *dest, const void *source, unsigned int size)
{
	UNIMPLEMENTED();
}

void SMemFill(void *location, unsigned int length, char fillWith)
{
	UNIMPLEMENTED();
}

void SMemZero(void *location, DWORD length)
{
	UNIMPLEMENTED();
}

int SMemCmp(void *location1, void *location2, DWORD size)
{
	UNIMPLEMENTED();
}

int SStrCopy(char *dest, const char *src, int max_length)
{
	strncpy(dest, src, max_length);
	return strlen(dest);
}

int SStrCmp(const char *string1, const char *string2, unsigned int size)
{
	UNIMPLEMENTED();
}

int SStrCmpI(const char *string1, const char *string2, unsigned int size)
{
	UNIMPLEMENTED();
}

void SDrawMessageBox(char *Text, char *Title, int Flags)
{
	MessageBoxA(NULL, Text, Title, Flags);
}

void SDrawDestroy(void)
{
	DUMMY();
}

BOOLEAN StormDestroy(void)
{
	DUMMY();
	return true;
}

BOOL SFileSetBasePath(char *)
{
	DUMMY();
	return true;
}

void SDrawRealizePalette(void)
{
	DUMMY();
}

BOOL SFileEnableDirectAccess(BOOL enable)
{
	directFileAccess = enable;
	return true;
}

}
