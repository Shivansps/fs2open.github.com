/*
 * Code created by Thomas Whittaker (RT) for a FreeSpace 2 source code project
 *
 * You may not sell or otherwise commercially exploit the source or things you 
 * created based on the source.
 *
*/ 
#ifndef FS2_SPEECH
#if defined(_WIN32)
#if NDEBUG
	#pragma message( "WARNING: You have not compiled speech into this build (use FS2_SPEECH)" )
#endif // NDEBUG
#endif // _WIN32
#else // FS2_SPEECH

#ifdef LAUNCHER
#include "stdafx.h"
#endif	//LAUNCHER

// Since we define these ourself we need to undefine them for the sapi header
#pragma push_macro("strcpy_s")
#pragma push_macro("strncpy_s")
#pragma push_macro("strcat_s")
#pragma push_macro("memset")
#pragma push_macro("memcpy")
#undef strcpy_s
#undef strncpy_s
#undef strcat_s
#undef memset
#undef memcpy

#include <windows.h>
#include <sapi.h>
#include <sphelper.h>

#pragma pushpop_macro("strcpy_s")
#pragma pushpop_macro("strncpy_s")
#pragma pushpop_macro("strcat_s")
#pragma pushpop_macro("memset")
#pragma pushpop_macro("memcpy")

ISpVoice *Voice_device;

#pragma warning(push)
#pragma warning(disable: 4995)
// Visual Studio complains that some functions are deprecated so this fixes that
#include <cstring>
#include <cwchar>
#include <cstdio>
#pragma warning(pop)
#include "globalincs/pstypes.h"
#include "utils/unicode.h"
#include "speech.h"

static SCP_vector<SCP_string> cached_voices;
static bool voices_cached = false;
bool Speech_init = false;

bool speech_init()
{
    HRESULT hr = CoCreateInstance(
		CLSID_SpVoice, 
		NULL, 
		CLSCTX_ALL, 
		IID_ISpVoice, 
		(void **)&Voice_device);

	Speech_init = SUCCEEDED(hr);

	nprintf(("Speech", "Speech init %s\n", Speech_init ? "succeeded!" : "failed!"));
	return Speech_init;
}

void speech_deinit()
{
	if(Speech_init == false) return;
	Voice_device->Release();
}

bool speech_play(const SCP_string& text)
{
	nprintf(("Speech", "Attempting to play speech string %s...\n", text.c_str()));

	if(Speech_init == false) return true;

	if (text.empty()) {
		nprintf(("Speech", "Not playing speech because passed text is empty.\n"));
		return false;
	}

	// Determine the needed amount of data
	auto num_chars = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0);

	if (num_chars <= 0) {
		// Error
		return false;
	}

	std::wstring wide_string;
	wide_string.resize(num_chars);

	auto err = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), &wide_string[0], num_chars);

	if (err <= 0) {
		return false;
	}

	speech_stop();
	return SUCCEEDED(Voice_device->Speak(wide_string.c_str(), SPF_ASYNC, NULL));
}

bool speech_pause()
{
	if(Speech_init == false) return true;
	return SUCCEEDED(Voice_device->Pause());
}

bool speech_resume()
{
	if(Speech_init == false) return true;
	return SUCCEEDED(Voice_device->Resume());
}

bool speech_stop()
{
	if(Speech_init == false) return true;
    return SUCCEEDED(Voice_device->Speak( NULL, SPF_PURGEBEFORESPEAK, NULL ));
}

bool speech_set_volume(unsigned short volume)
{
    return SUCCEEDED(Voice_device->SetVolume(volume));
}

bool speech_set_voice(int voice)
{
	HRESULT                             hr;
	CComPtr<ISpObjectToken>             cpVoiceToken;
	CComPtr<IEnumSpObjectTokens>        cpEnum;
	ULONG                               num_voices = 0;

	//Enumerate the available voices 
	hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum);

	if(FAILED(hr)) return false;

    hr = cpEnum->GetCount(&num_voices);

	if(FAILED(hr)) return false;

	int count = 0;
	// Obtain a list of available voice tokens, set the voice to the token, and call Speak
	while (num_voices -- )
	{
		cpVoiceToken.Release();
		
		hr = cpEnum->Next( 1, &cpVoiceToken, NULL );

		if(FAILED(hr)) {
			return false;
		}

		if(count == voice) {
			return SUCCEEDED(Voice_device->SetVoice(cpVoiceToken));
		}

		count++;
	}
	return false;
}

// Goober5000
bool speech_is_speaking()
{
	HRESULT			hr;
	SPVOICESTATUS	pStatus;

	hr = Voice_device->GetStatus(&pStatus, NULL);
	if (FAILED(hr)) return false;

	return (pStatus.dwRunningState != SPRS_DONE);
}

SCP_vector<SCP_string> speech_enumerate_voices()
{
	if (voices_cached) {
		return cached_voices;
	}

	HRESULT hr = CoCreateInstance(
		CLSID_SpVoice,
		NULL,
		CLSCTX_ALL,
		IID_ISpVoice,
		(void **)&Voice_device);

	if (FAILED(hr)) {
		return SCP_vector<SCP_string>();
	}

	// This code is mostly copied from wxLauncher
	ISpObjectTokenCategory * comTokenCategory = NULL;
	IEnumSpObjectTokens * comVoices = NULL;
	ULONG comVoicesCount = 0;

	// Generate enumeration of voices
	hr = ::CoCreateInstance(CLSID_SpObjectTokenCategory, NULL,
		CLSCTX_INPROC_SERVER, IID_ISpObjectTokenCategory, (LPVOID*)&comTokenCategory);
	if (FAILED(hr)) {
		return SCP_vector<SCP_string>();
	}

	hr = comTokenCategory->SetId(SPCAT_VOICES, false);
	if (FAILED(hr)) {
		return SCP_vector<SCP_string>();
	}

	hr = comTokenCategory->EnumTokens(NULL, NULL, &comVoices);
	if (FAILED(hr)) {
		return SCP_vector<SCP_string>();
	}

	hr = comVoices->GetCount(&comVoicesCount);
	if (FAILED(hr)) {
		return SCP_vector<SCP_string>();
	}

	SCP_vector<SCP_string> voices;
	while (comVoicesCount > 0) {
		ISpObjectToken * comAVoice = NULL;

		comVoices->Next(1, &comAVoice, NULL); // retrieve just one

		LPWSTR id = NULL;
		comAVoice->GetStringValue(NULL, &id);

		auto idlength = wcslen(id);
		auto buffer_size = WideCharToMultiByte(CP_UTF8, 0, id, (int)idlength, nullptr, 0, nullptr, nullptr);

		if (buffer_size > 0) {
			SCP_string voiceName;
			voiceName.resize(buffer_size);
			buffer_size = WideCharToMultiByte(CP_UTF8, 0, id, (int)idlength, &voiceName[0], buffer_size, nullptr, nullptr);

			voices.push_back(voiceName);
		}

		CoTaskMemFree(id);
		comAVoice->Release();
		comVoicesCount--;
	}

	comTokenCategory->Release();
	//only release the voice_device when getting flags
	if (!Speech_init)
		Voice_device->Release();
	voices_cached = true;
	cached_voices = voices;
	return voices;
}

#endif // FS2_SPEECH