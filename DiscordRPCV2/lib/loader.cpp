#include "loader.h"
#include "lib.h"

#include "squirrel.h"
#include <iostream>

#include "../include/spdlog/sinks/base_sink.h"
#include "logging.h"

void PLUGIN_INIT(PluginNorthstarData* data) {
	g_pPlugin = new Plugin;
	g_pPlugin->logger = (logger_t)GetProcAddress(data->northstarModule, "PLUGIN_LOG");;
	spdlog::default_logger()->sinks().pop_back();
	spdlog::default_logger()->sinks().push_back(std::make_shared<my_sink>());
	spdlog::info("PluginLibrary-plugin succesfully initialised!");

	g_pPlugin->Init(data);
	g_pPlugin->Main();
}

void PLUGIN_INIT_SQVM_CLIENT(SquirrelFunctions* funcs) {
	InitializeSquirrelVM_CLIENT(funcs);
}

void PLUGIN_INIT_SQVM_SERVER(SquirrelFunctions* funcs) {
	InitializeSquirrelVM_SERVER(funcs);
}

void PLUGIN_INFORM_SQVM_CREATED(ScriptContext context, CSquirrelVM* sqvm) {
	switch (context)
	{
	case ScriptContext::CLIENT:
		g_pSquirrel<ScriptContext::CLIENT>->VMCreated(sqvm);
		break;
	case ScriptContext::SERVER:
		g_pSquirrel<ScriptContext::SERVER>->VMCreated(sqvm);
		break;
	case ScriptContext::UI:
		g_pSquirrel<ScriptContext::UI>->VMCreated(sqvm);
		break;
	default:
		spdlog::warn("PLUGIN_INFORM_SQVM_CREATED called with unknown ScriptContext {}", context);
	}
}

void PLUGIN_INFORM_SQVM_DESTROYED(ScriptContext context) {
	switch (context)
	{
	case ScriptContext::CLIENT:
		g_pSquirrel<ScriptContext::CLIENT>->VMDestroyed();
		break;
	case ScriptContext::SERVER:
		g_pSquirrel<ScriptContext::SERVER>->VMDestroyed();
		break;
	case ScriptContext::UI:
		g_pSquirrel<ScriptContext::UI>->VMDestroyed();
		break;
	default:
		spdlog::warn("PLUGIN_INFORM_SQVM_DESTROYED called with unknown ScriptContext {}", context);
	}
}

// TODO: this stuff should be saved to global structs
// since this is async, these should have shared_mutex's

void PLUGIN_RESPOND_SERVER_DATA(PluginServerData* data) {
	if (data == nullptr) {
		return;
	}
	spdlog::info("Got Server data back from NS");
	g_pPlugin->LoadServerData(data);
}
void PLUGIN_RESPOND_GAMESTATE_DATA(PluginGameStateData* data) {
	if (data == nullptr) {
		return;
	}
	spdlog::info("Got GameState data back from NS");
	g_pPlugin->LoadGameStateData(data);
}