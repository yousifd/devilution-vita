#include "devilution.h"
#include "miniwin/dsound.h"
#include "stubs.h"
#include <SDL2/SDL.h>

namespace dvl {

ULONG DirectSound::Release()
{
	Mix_CloseAudio();
	return 0;
};

HRESULT DirectSound::CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOute)
{
	if (pcDSBufferDesc->dwFlags != DVL_DSBCAPS_PRIMARYBUFFER) { // Creating primery buffer isn't needed and breaks Music
		*ppDSBuffer = new DirectSoundBuffer();
	}

	return DVL_DS_OK;
};

HRESULT DirectSound::GetCaps(LPDSCAPS pDSCaps)
{
	return DVL_DS_OK;
};

/**
 * @brief SDL handels this for us when using Mix_PlayChannel(-1);
 */
HRESULT DirectSound::DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate)
{
	UNIMPLEMENTED();
	return DVL_DS_OK;
};

HRESULT DirectSound::SetCooperativeLevel(HWND hwnd, DWORD dwLevel)
{
	return DVL_DS_OK;
};

///// DirectSoundBuffer /////

ULONG DirectSoundBuffer::Release()
{
	Mix_FreeChunk(chunk);

	return 0;
};

/**
 * @brief Only used for handeling duplicates 
 */
HRESULT DirectSoundBuffer::GetStatus(LPDWORD pdwStatus)
{
	return DVL_DSERR_INVALIDPARAM;
};

HRESULT DirectSoundBuffer::Lock(DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
    LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{
	*pdwAudioBytes1 = dwBytes;
	*ppvAudioPtr1 = malloc(dwBytes);

	return DVL_DS_OK;
};

HRESULT DirectSoundBuffer::Play(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
{
	int channel = Mix_PlayChannel(-1, chunk, 0);
	if (channel == -1) {
		SDL_Log("To few channels, skipping sound\n");
		return DVL_DS_OK;
	}

	Mix_Volume(channel, volume);
	int panned = 255 - 255 * abs(pan) / 10000;
	Mix_SetPanning(channel, pan <= 0 ? 255 : panned, pan >= 0 ? 255 : panned);

	return DVL_DS_OK;
};

HRESULT DirectSoundBuffer::SetFormat(LPCWAVEFORMATEX pcfxFormat)
{
	return DVL_DS_OK;
};

HRESULT DirectSoundBuffer::SetVolume(LONG lVolume)
{
	volume = MIX_MAX_VOLUME - MIX_MAX_VOLUME * lVolume / VOLUME_MIN;

	return DVL_DS_OK;
};

HRESULT DirectSoundBuffer::SetPan(LONG lPan)
{
	pan = lPan;
	return DVL_DS_OK;
};

HRESULT DirectSoundBuffer::Stop()
{
	for (int i = 1; i < Mix_AllocateChannels(-1); i++) {
		if (Mix_GetChunk(i) != chunk) {
			continue;
		}

		Mix_HaltChannel(i);
	}

	return DVL_DS_OK;
};

HRESULT DirectSoundBuffer::Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{
	SDL_RWops *rw = SDL_RWFromConstMem(pvAudioPtr1, dwAudioBytes1);
	if (rw == NULL) {
		SDL_Log(SDL_GetError());
	}

	chunk = Mix_LoadWAV_RW(rw, 1);
	free(pvAudioPtr1);

	return DVL_DS_OK;
};

HRESULT DirectSoundBuffer::Restore()
{
	return DVL_DS_OK;
};

} // namespace dvl
