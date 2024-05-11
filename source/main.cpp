#include <psp2/sysmodule.h>

#include "config.h"
#include "fs.h"
#include "gui.h"
#include "log.h"
#include "power.h"
#include "textures.h"
#include "utils.h"

int _newlib_heap_size_user = 192 * 1024 * 1024;

namespace Services {
	void Init(void) {
		GUI::Init();
		Utils::InitAppUtil();
		sceSysmoduleLoadModule(SCE_SYSMODULE_JSON);

		if (!FS::DirExists("ux0:data/VITAHomebrewSorter/backup")) {
			FS::MakeDir("ux0:data/VITAHomebrewSorter/backup");
		}

		if (!FS::DirExists("ux0:data/VITAHomebrewSorter/loadouts")) {
			FS::MakeDir("ux0:data/VITAHomebrewSorter/loadouts");
		}
		
		Log::Init();
		Textures::Init();
		Power::InitThread();
		Config::Load();
	}

	void Exit(void) {
		Textures::Exit();
		Log::Exit();
		sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON);
		Utils::EndAppUtil();
		GUI::Exit();
	}
}

int main(int argc, char *argv[]) {
	Services::Init();
	GUI::RenderLoop();
	Services::Exit();
	return 0;
}
