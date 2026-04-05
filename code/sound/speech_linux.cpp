#ifdef FS2_SPEECH
#include <speechd/libspeechd.h>
#include "globalincs/pstypes.h"
#include "utils/unicode.h"

static SCP_vector<SCP_string> cached_voices;
static bool voices_cached = false;

bool speech_init()
{
	if (Speech_init) {
		return true;
	}


	Speech_init = true;
	return true;
}

void speech_deinit()
{
	if ( !Speech_init ) {
		return;
	}

	Speech_init = false;
}

bool speech_play(const SCP_string& text)
{
	if ( !Speech_init ) {
		return false;
	}

	if (text.empty()) {
		nprintf(("Speech", "Not playing speech because passed text is empty.\n"));
		return false;
	}


	return true;
}

bool speech_pause()
{
	if ( !Speech_init ) {
		return false;
	}


	return true;
}

bool speech_resume()
{
	if ( !Speech_init ) {
		return false;
	}


	return true;
}

bool speech_stop()
{
	if ( !Speech_init ) {
		return false;
	}


	return true;
}

bool speech_set_volume(unsigned short volume)
{
	if ( !Speech_init ) {
		return false;
	}


	return true;
}

bool speech_set_voice(int voice)
{
	if ( !Speech_init ) {
		return false;
	}

	return true;
}

bool speech_is_speaking()
{
	if ( !Speech_init ) {
		return false;
	}

	return false;
}

SCP_vector<SCP_string> speech_enumerate_voices()
{
	if (voices_cached) {
		return cached_voices;
	}

	SCP_vector<SCP_string> fsoVoices;


	voices_cached = true;
	cached_voices = fsoVoices;
	return fsoVoices;
}

#endif