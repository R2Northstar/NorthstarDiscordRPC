#include "loader.h"
#include "lib.h"

#include "squirrel.h"
#include <iostream>

#include "../include/spdlog/sinks/base_sink.h"
#include "logging.h"

#include "concommand.h"
#include "convar.h"
#include "sourceinterface.h"

void PLUGIN_INIT(PluginInitFuncs* funcs, PluginNorthstarData* data) {
	g_pPlugin = new Plugin;
	g_pSqAutoBindContainer = new SquirrelAutoBindContainer();
	g_pPlugin->logger = (logger_t)funcs->logger;
	spdlog::default_logger()->sinks().pop_back();
	spdlog::default_logger()->sinks().push_back(std::make_shared<my_sink>());
	spdlog::info("PluginLibrary-plugin succesfully initialised!");

	g_pPlugin->Init(funcs, data);
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
	case ScriptContext::SERVER:
		g_pSquirrel<ScriptContext::SERVER>->VMCreated(sqvm);
	case ScriptContext::UI:
		g_pSquirrel<ScriptContext::UI>->VMCreated(sqvm);
	default:
		spdlog::warn("PLUGIN_INFORM_SQVM_CREATED called with unknown ScriptContext {}", context);
	}
}

void PLUGIN_INFORM_SQVM_DESTROYED(ScriptContext context) {
	switch (context)
	{
	case ScriptContext::CLIENT:
		g_pSquirrel<ScriptContext::CLIENT>->VMDestroyed();
	case ScriptContext::SERVER:
		g_pSquirrel<ScriptContext::SERVER>->VMDestroyed();
	case ScriptContext::UI:
		g_pSquirrel<ScriptContext::UI>->VMDestroyed();
	default:
		spdlog::warn("PLUGIN_INFORM_SQVM_DESTROYED called with unknown ScriptContext {}", context);
	}
}

void PLUGIN_RECEIVE_PRESENCE(PluginGameStatePresence* data) {
	g_pPlugin->LoadPresence(data);
}

void LoadDLLEngine(PluginEngineData* data) {
	ConCommandConstructor = (ConCommandConstructorType)(data->ConCommandConstructor);
	conVarMalloc = (ConVarMallocType)(data->conVarMalloc);
	conVarRegister = (ConVarRegisterType)(data->conVarRegister);

	g_pConVar_Vtable = data->ConVar_Vtable;
	g_pIConVar_Vtable = data->IConVar_Vtable;

	g_pPlugin->DLLLoadEngine();
}

void PLUGIN_INFORM_DLL_LOAD(PluginLoadDLL dll, void* data) {
	switch (dll) {
		case PluginLoadDLL::ENGINE:
			LoadDLLEngine(static_cast<PluginEngineData*>(data));
			break;
		default:
			spdlog::warn("PLUGIN_INFORM_SQVM_DESTROYED called with unknown PluginLoadDLL type {}", dll);
			break; 
	}
}