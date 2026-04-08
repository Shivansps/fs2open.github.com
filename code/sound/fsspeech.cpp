/*
 * Code created by Thomas Whittaker (RT) for a FreeSpace 2 source code project
 *
 * You may not sell or otherwise commercially exploit the source or things you 
 * created based on the source.
 *
*/ 

#include "globalincs/pstypes.h"
#include "osapi/osregistry.h"
#include "sound/fsspeech.h"
#include "sound/speech.h"
#include "options/Option.h"


extern int Cmdline_freespace_no_sound;

const size_t MAX_SPEECH_BUFFER_LEN = 4096;

static int speech_inited = 0;

bool FSSpeech_play_from[FSSPEECH_FROM_MAX];
const char *FSSpeech_play_id[FSSPEECH_FROM_MAX] =
{
	"SpeechTechroom",
	"SpeechBriefings",
	"SpeechIngame",
	"SpeechMulti"
};

char Speech_buffer[MAX_SPEECH_BUFFER_LEN] = "";
size_t  Speech_buffer_len;

static bool ttsrate_change(float new_val, bool initial)
{
	if (initial) {
		return false;
	}
	speech_set_rate(new_val);
	return true;
}

static bool ttsingame_change(bool new_val, bool initial)
{
	if (initial) {
		return false;
	}
	FSSpeech_play_from[FSSPEECH_FROM_INGAME] = new_val;
	return true;
}

static bool ttsmulti_change(bool new_val, bool initial)
{
	if (initial) {
		return false;
	}
	FSSpeech_play_from[FSSPEECH_FROM_MULTI] = new_val;
	return true;
}

static bool ttsbriefing_change(bool new_val, bool initial)
{
	if (initial) {
		return false;
	}
	FSSpeech_play_from[FSSPEECH_FROM_BRIEFING] = new_val;
	return true;
}

static bool ttstechroom_change(bool new_val, bool initial)
{
	if (initial) {
		return false;
	}
	FSSpeech_play_from[FSSPEECH_FROM_TECHROOM] = new_val;
	return true;
}

static bool ttsvolume_change(float new_val, bool initial)
{
	if (initial) {
		return false;
	}
	speech_set_volume((unsigned short) new_val);
	return true;
}

static SCP_vector<int> ttsvoice_enumerator()
{
	SCP_vector<int> vals;
	auto voices = speech_enumerate_voices();
	for (int i = 0; i < static_cast<int>(voices.size()); ++i) {
		vals.push_back(i);
	}
	return vals;
}

static SCP_string ttsvoice_display(int id)
{
	auto voices = speech_enumerate_voices();
	if (voices.empty() || id < 0 || static_cast<size_t>(id) >= voices.size()) {
        return "No voices loaded";
    }
    SCP_string out;
	sprintf(out, "(%d) %s", id + 1, voices[id].c_str());
	return out;
}

static bool ttsvoice_change(int id, bool initial)
{
	if (initial) {
		return false;
	}
	auto voices = speech_enumerate_voices();
	if (voices.empty() || id < 0 || static_cast<size_t>(id) >= voices.size()) {
        return false;
    }
	speech_set_voice(id);
	return true;
}

static auto SpeechVoiceOption = options::OptionBuilder<int>("Speech.Voice",
	std::pair<const char*, int>{"TTS Voice", -1},
	std::pair<const char*, int>{"The voice used to read text", -1})
	.category(std::make_pair("Audio", 1826))
	.level(options::ExpertLevel::Beginner)
	.enumerator(ttsvoice_enumerator)
	.display(ttsvoice_display)
	.flags({ options::OptionFlags::ForceMultiValueSelection })
	.default_val(0)
	.change_listener(ttsvoice_change)
	.importance(3)
	.finish();

static auto SpeechVolumeOption = options::OptionBuilder<float>("Speech.Volume",
	std::pair<const char*, int>{"TTS Volume", -1},
	std::pair<const char*, int>{"Volume used for playing TTS speech", -1})
	.category(std::make_pair("Audio", 1826))
	.range(0.0f, 100.0f)
	.default_val(100.0f)
	.change_listener(ttsvolume_change)
	.importance(2)
	.finish();

static auto SpeechRateOption = options::OptionBuilder<float>("Speech.Rate",
	std::pair<const char*, int>{"TTS Rate", -1},
	std::pair<const char*, int>{"Speed of the TTS voice (100 = normal)", -1})
	.category(std::make_pair("Audio", 1826))
	.range(50.0f, 150.0f)
	.default_val(100.0f)
	.change_listener(ttsrate_change)
	.importance(1)
	.finish();

static auto SpeechBriefingOption = options::OptionBuilder<bool>("Speech.Briefing",
	std::pair<const char*, int>{"TTS in briefings", -1},
	std::pair<const char*, int>{"Enable or disable TTS in briefings", -1})
	.category(std::make_pair("Audio", 1826))
	.level(options::ExpertLevel::Beginner)
	.change_listener(ttsbriefing_change)
	.default_val(true)
	.importance(0)
	.finish();

static auto SpeechTechroomOption = options::OptionBuilder<bool>("Speech.Techroom",
	std::pair<const char*, int>{"TTS in techroom", -1},
	std::pair<const char*, int>{"Enable or disable TTS in techroom", -1})
	.category(std::make_pair("Audio", 1826))
	.level(options::ExpertLevel::Beginner)
	.change_listener(ttstechroom_change)
	.default_val(true)
	.importance(0)
	.finish();

static auto SpeechIngameOption = options::OptionBuilder<bool>("Speech.Ingame",
	std::pair<const char*, int>{"TTS in-game", -1},
	std::pair<const char*, int>{"Enable or disable TTS in-game", -1})
	.category(std::make_pair("Audio", 1826))
	.level(options::ExpertLevel::Beginner)
	.change_listener(ttsingame_change)
	.default_val(true)
	.importance(0)
	.finish();

static auto SpeechMultiOption = options::OptionBuilder<bool>("Speech.Multi",
	std::pair<const char*, int>{"TTS in multiplayer", -1},
	std::pair<const char*, int>{"Enable or disable TTS in multiplayer", -1})
	.category(std::make_pair("Audio", 1826))
	.level(options::ExpertLevel::Beginner)
	.change_listener(ttsmulti_change)
	.default_val(true)
	.importance(0)
	.finish();

void sanitize_text(const char* input, SCP_string& output) {
	output.clear();
	bool saw_dollar = false;
	for (auto ch : unicode::codepoint_range(input)) {
		if (ch == UNICODE_CHAR('$')) {
			saw_dollar = true;
			continue;
		}
		else if (saw_dollar) {
			saw_dollar = false;
			continue;
		}
		unicode::encode(ch, std::back_inserter(output));
	}
}

bool fsspeech_init()
{
	if (speech_inited) {
		return true;
	}

	// if sound is disabled from the cmdline line then don't do speech either
	if (Cmdline_freespace_no_sound) {
		return false;
	}

	if(speech_init() == false) {
		return false;
	}

	if (Using_in_game_options) 
	{
		FSSpeech_play_from[FSSPEECH_FROM_TECHROOM] = SpeechTechroomOption->getValue();
		FSSpeech_play_from[FSSPEECH_FROM_BRIEFING] = SpeechBriefingOption->getValue();
		FSSpeech_play_from[FSSPEECH_FROM_INGAME] = SpeechIngameOption->getValue();
		FSSpeech_play_from[FSSPEECH_FROM_MULTI] = SpeechMultiOption->getValue();
		// Early caching of voices names, needed for sapi not to override initial voice selection
		speech_enumerate_voices();
		speech_set_volume((unsigned short)SpeechVolumeOption->getValue());
		speech_set_voice(SpeechVoiceOption->getValue());
		speech_set_rate(SpeechRateOption->getValue());
	}
	else 
	{
		// Get the settings from the registry
		for (int i = 0; i < FSSPEECH_FROM_MAX; i++) {
			FSSpeech_play_from[i] = static_cast<bool>(os_config_read_uint(nullptr, FSSpeech_play_id[i], 0));
			nprintf(("Speech", "Play %s: %s\n", FSSpeech_play_id[i], FSSpeech_play_from[i] ? "true" : "false"));
		}

		int volume = os_config_read_uint(nullptr, "SpeechVolume", 100);
		speech_set_volume((unsigned short)volume);

		int voice = os_config_read_uint(nullptr, "SpeechVoice", 0);
		speech_set_voice(voice);

		int rate = os_config_read_uint(nullptr, "SpeechRate", 100);
		speech_set_rate(static_cast<float>(rate));
	}

	speech_inited = 1;

	return true;
}

void fsspeech_deinit()
{
	if (!speech_inited)
		return;

	speech_deinit();

	speech_inited = 0;
}

void fsspeech_play(int type, const char *text)
{
	if (text == nullptr) {
		nprintf(("Speech", "Not playing speech because passed text is null.\n"));
		return;
	}

	if (!speech_inited) {
		nprintf(("Speech", "Aborting fsspech_play because speech_inited is false.\n"));
		return;
	}

	if (type >= FSSPEECH_FROM_MAX) {
		nprintf(("Speech", "Aborting fsspeech_play because speech type is out of range.\n"));
		return;
	}

	if (type >= 0 && FSSpeech_play_from[type] == false) {
		nprintf(("Speech", "Aborting fsspeech_play because we aren't supposed to play from type %s.\n", FSSpeech_play_id[type]));
		return;
	}

	SCP_string sanitized_string;
	sanitize_text(text, sanitized_string);

	speech_play(sanitized_string);
}

void fsspeech_stop()
{
	if (!speech_inited)
		return;

	speech_stop();
}

void fsspeech_pause(bool playing)
{
	if (!speech_inited)
		return;

	if(playing) {
		speech_pause();
	} else {
		speech_resume();
	}
}

void fsspeech_start_buffer()
{
	Speech_buffer_len = 0;
	Speech_buffer[0] = '\0';
}

void fsspeech_stuff_buffer(const char *text)
{
	if (!speech_inited)
		return;

	size_t len = strlen(text);

	if(Speech_buffer_len + len < MAX_SPEECH_BUFFER_LEN) {
		strcat_s(Speech_buffer, text);
	}

	Speech_buffer_len += len;
}

void fsspeech_play_buffer(int type)
{
	if (!speech_inited)
		return;

	fsspeech_play(type, Speech_buffer);
}

// Goober5000
bool fsspeech_play_from(int type)
{
	Assert(type >= 0 && type < FSSPEECH_FROM_MAX);

	return (speech_inited && FSSpeech_play_from[type]);
}

// Goober5000
bool fsspeech_playing()
{
	if (!speech_inited)
		return false;

	return speech_is_speaking();
}
