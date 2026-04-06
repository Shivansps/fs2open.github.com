#ifdef FS2_SPEECH
#include <libspeechd.h>
#include "globalincs/pstypes.h"
#include "utils/unicode.h"

static SCP_vector<SCP_string> cached_voices;
static bool voices_cached = false;
static bool Speech_init = false;
static SPDConnection* spd = nullptr;

bool speech_init()
{
	if (Speech_init) {
		return true;
	}
	
    spd = spd_open("freespace_open", "main", nullptr, SPD_MODE_SINGLE);
    if (!spd) {
        mprintf(("Speech: Unable to connect to speech-dispatcher\n"));
        return false;
    }

	Speech_init = true;
	return true;
}

void speech_deinit()
{
	if ( !Speech_init ) {
		return;
	}
	spd_close(spd);
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

	return (spd_say(spd, SPD_TEXT, text.c_str()) >= 0);
}

bool speech_pause()
{
	if ( !Speech_init ) {
		return false;
	}

	spd_pause(spd);
	
	return true;
}

bool speech_resume()
{
	if ( !Speech_init ) {
		return false;
	}

	spd_resume(spd);
	
	return true;
}

bool speech_stop()
{
	if ( !Speech_init ) {
		return false;
	}

	spd_stop(spd);
	
	return true;
}

bool speech_set_volume(unsigned short volume)
{
	if ( !Speech_init ) {
		return false;
	}

	spd_set_volume(spd, volume); 
	
	return true;
}

bool speech_set_voice(int voice)
{
	if ( !Speech_init ) {
		return false;
	}
	
	if (voice < 0 || static_cast<size_t>(voice) >= cached_voices.size()) {
        return false;
    }
	
	spd_set_synthesis_voice(spd, cached_voices[voice].c_str());
	
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

    SPDConnection* connection = spd;
    if ( !Speech_init ) {
    	connection = spd_open("fso_voice_list", "client", NULL, SPD_MODE_SINGLE);
    	if (!connection) {
        	mprintf(("Speech: Unable to connect to speech-dispatcher\n"));
        	voices_cached = true;
        	cached_voices = fsoVoices;
        	return fsoVoices;
    	}
	}

    SPDVoice** voices = spd_list_synthesis_voices(connection);
    
    for (int i = 0; voices[i] != NULL; i++) {
    	SCP_string lang = voices[i]->language;
    	// There are too many we cant add them all
    	// Only add English voices
    	if(lang.find("en") == 0) {
    		SCP_string voiceName;
    		voiceName = voices[i]->name ? voices[i]->name : "unknown";
        	fsoVoices.push_back(voiceName);
        }
    }

    //spd_free_voices(voices);
    if ( !Speech_init ) {
    	spd_close(connection);
	}
	voices_cached = true;
	cached_voices = fsoVoices;
	return fsoVoices;
}

#endif
