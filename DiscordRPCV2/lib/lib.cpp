#include "lib.h"
#include "plugin_abi.h"

#include "../pch.h"

Plugin* g_pPlugin;

void Plugin::Init(PluginNorthstarData* data) {
	_RequestServerData = (PLUGIN_REQUESTS_SERVER_DATA_TYPE)GetProcAddress(data->northstarModule, "PLUGIN_REQUESTS_SERVER_DATA");
	_RequestGameStateData = (PLUGIN_REQUESTS_GAMESTATE_DATA_TYPE)GetProcAddress(data->northstarModule, "PLUGIN_REQUESTS_GAMESTATE_DATA");

	_RelayInvite = (PLUGIN_RELAY_INVITE)GetProcAddress(data->northstarModule, "PLUGIN_RELAY_INVITE");

	server = new ServerDataClass();
	gameState = new GameStateDataClass();
}

void Plugin::RequestServerData() {
	_RequestServerData((PLUGIN_RESPOND_SERVER_DATA_TYPE*)&PLUGIN_RESPOND_SERVER_DATA);
}
void Plugin::RequestGameStateData() {
	_RequestGameStateData((PLUGIN_RESPOND_GAMESTATE_DATA_TYPE*)&PLUGIN_RESPOND_GAMESTATE_DATA);
}

void Plugin::RelayInvite(const char* invite) {
	_RelayInvite(invite);
}

void Plugin::LoadServerData(PluginServerData* data) {
	std::unique_lock lock(mutex);

	server->id = data->id;
	server->name = data->name;
	server->description = data->description;
	server->password = data->password;

	server->isLocal = data->is_local;
}

void Plugin::LoadGameStateData(PluginGameStateData* data) {
	std::unique_lock lock(mutex);

	gameState->map = data->map;
	gameState->mapDisplayname = data->map_displayname;
	gameState->playlist = data->playlist;
	gameState->playlistDisplayName = data->playlist_displayname;

	gameState->currentPlayers = data->current_players;
	gameState->maxPlayers = data->max_players;
	gameState->ownScore = data->own_score;
	gameState->otherHighestScore = data->other_highest_score;
	gameState->maxScore = data->max_score;

	gameState->timestamp = data->timestamp_end;
}