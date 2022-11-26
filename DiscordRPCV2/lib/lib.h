#pragma once

#include "plugin_abi.h"
#include "loader.h"
#include "util.h"

typedef void (*logger_t)(void*);


class Plugin {
	public:
		logger_t logger;
		void Init(PluginNorthstarData* data);

		void Main(); // To be specified by the plugin developer

	private:
		PLUGIN_REQUESTS_SERVER_DATA_TYPE _RequestServerData;
		PLUGIN_REQUESTS_GAMESTATE_DATA_TYPE _RequestGameStateData;
		PLUGIN_RELAY_INVITE _RelayInvite;

	public:
		void RequestServerData();
		void RequestGameStateData();

		void RelayInvite(const char* invite);

		void LoadServerData(PluginServerData* data);
		void LoadGameStateData(PluginGameStateData* data);

		ServerDataClass* server;
		GameStateDataClass* gameState;

		std::shared_mutex mutex;
};

extern Plugin* g_pPlugin;