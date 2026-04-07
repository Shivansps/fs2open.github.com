//

#include "AboutDialogModel.h"

#include <project.h>
#include <graphics/2d.h>

extern const char* fs2_open_credit_text;

namespace fso::fred::dialogs {

AboutDialogModel::AboutDialogModel(QObject* parent, EditorViewport* viewport)
	: AbstractDialogModel(parent, viewport)
{
}

bool AboutDialogModel::apply()
{
	return true;
}

void AboutDialogModel::reject()
{
}

SCP_string AboutDialogModel::getVersionString()
{
	SCP_string graphicsAPI;
	switch (gr_screen.mode) {
	case GR_OPENGL:
		graphicsAPI = "OpenGL";
		break;
	case GR_VULKAN:
		graphicsAPI = "Vulkan";
		break;
	}

	SCP_string result = "qtFRED - FreeSpace Editor, Version ";
	result += FS_VERSION_FULL;
	if (!graphicsAPI.empty()) {
		result += " ";
		result += graphicsAPI;
	}
	return result;
}

SCP_string AboutDialogModel::getCopyrightString()
{
	return "Based on FRED2_OPEN: Copyright \xc2\xa9 1999 Volition, Inc. All Rights Reserved";
}

SCP_vector<SCP_string> AboutDialogModel::getQtFREDCredits()
{
	return {
		"Initial Qt port by groscask",
		"Developed to replace FRED2: Cyborg, m!m, Mike \"MjnMixael\" Nelson, The Force",
		"With contributions from: BMagnu, DahBlount, Goober5000, jg18, niffiwan, plieblang, the-maddin, z64555",
	};
}

SCP_vector<SCP_string> AboutDialogModel::getGraphicsCredits()
{
	switch (gr_screen.mode) {
	case GR_OPENGL:
		return {
			"Ported to OpenGL by RandomTiger",
			"Original FSO OpenGL port by Phreak and Fry_Day",
		};
	case GR_VULKAN:
		// TODO: Add Vulkan port credits when available
		return { "Vulkan not ported yet" };
	}
	return {};
}

SCP_string AboutDialogModel::getSCPCreditsText()
{
	return fs2_open_credit_text;
}

SCP_string AboutDialogModel::getQuoteString()
{
	return "FRED2 has been deprecated. We regret nothing.";
}

} // namespace fso::fred::dialogs
