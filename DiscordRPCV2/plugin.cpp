#define _CRT_SECURE_NO_WARNINGS

#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

#include "lib/lib.h"
#include "include/base64.h"

#include <random>

#include "lib_discord/discord.h"
#include "lib/squirrel.h"

#include "pch.h"

bool wasInGame;
bool resetSinglePlayerTimer = true;

ConVar* Cvar_ns_discord_allow_join;
ConVar* Cvar_ns_discord_include_password;

struct DiscordState {
	discord::User currentUser;

	std::unique_ptr<discord::Core> core;
};

namespace {
	volatile bool interrupted{ false };
}

std::string generateUUID() {
	std::srand(std::time(0)); //use current time as seed for random generator
	int random_variable = std::rand();
	return std::to_string(random_variable);
}

GameState previousGameState = GameState::LOADING;

int PluginLoop()
{

	char path[MAX_PATH]{};
	GetModuleFileNameA(NULL, path, MAX_PATH);
	std::string exepath = std::string(path) + " -discord";
	if (strstr(GetCommandLineA(), "-discord"))
		SetEnvironmentVariable(L"DISCORD_INSTANCE_ID", L"1");

	DiscordState state{};

	discord::Core* core{};

	auto result = discord::Core::Create(941428101429231617, DiscordCreateFlags_NoRequireDiscord, &core);
	state.core.reset(core);

	if (!state.core) {
		spdlog::warn("Failed to instantiate discord core! (err {})", static_cast<int>(result));
		return -1;
	}

	state.core->SetLogHook(
		discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
			switch (level) {
			case discord::LogLevel::Error:
				spdlog::error("{}", message);
				break;
			case discord::LogLevel::Warn:
				spdlog::warn("{}", message);
				break;
			case discord::LogLevel::Info:
				spdlog::info("{}", message);
				break;
			case discord::LogLevel::Debug:
				spdlog::debug("{}", message);
				break;
			}
		});

	state.core->ActivityManager().RegisterCommand(exepath.c_str()); // Set-up for invites

	discord::Activity activity{};

	state.core->ActivityManager().OnActivityJoin.Connect(
		[&activity](const char* secret) {
			spdlog::info("Join {}", secret);
			g_pPlugin->RelayInvite(secret);
			activity.GetParty().SetId(generateUUID().c_str());;
		});


	activity.SetDetails("");
	activity.SetState("Loading...");
	activity.GetAssets().SetSmallImage("northstar");
	activity.GetAssets().SetSmallText("");
	activity.GetAssets().SetLargeImage("");
	activity.GetAssets().SetLargeText("");

	activity.GetParty().SetId(generateUUID().c_str());
	activity.GetParty().SetPrivacy(discord::ActivityPartyPrivacy::Private);
	activity.SetType(discord::ActivityType::Playing);
	state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
		spdlog::info("{} updating activity!", (result == discord::Result::Ok) ? "Succeeded" : "Failed");
	});

	std::signal(SIGINT, [](int) { interrupted = true; });

	do
	{
		state.core->RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		std::string details = "Score: ";

		if (g_pPlugin->mutex.try_lock_shared() && true) {

			activity.SetState(g_pPlugin->presence->playlist_displayname.c_str());
			activity.GetAssets().SetLargeImage(g_pPlugin->presence->map.c_str());
			activity.GetAssets().SetLargeText(g_pPlugin->presence->playlist_displayname.c_str());

			switch (g_pPlugin->presence->state) {
			case GameState::LOADING:
			case GameState::MAINMENU:
			case GameState::LOBBY:
				if (g_pPlugin->presence->state != previousGameState) {
					const auto p1 = std::chrono::system_clock::now().time_since_epoch();
					const auto currentTime = std::chrono::duration_cast<std::chrono::seconds>(p1).count();
					activity.GetTimestamps().SetStart(currentTime);
				}
				break;
			default:
				break;
			}
			switch (g_pPlugin->presence->state) {
			case GameState::LOADING:
				activity.GetParty().GetSize().SetCurrentSize(0);
				activity.GetParty().GetSize().SetMaxSize(0);
				activity.GetSecrets().SetJoin("");
				activity.SetDetails("Loading...");
				activity.SetState("Loading...");
				activity.GetAssets().SetLargeImage("northstar");
				activity.GetAssets().SetLargeText("Titanfall 2 + Northstar");
				activity.GetAssets().SetSmallImage("");
				activity.GetAssets().SetSmallText("");
				activity.GetTimestamps().SetEnd(0);
				break;
			case GameState::MAINMENU:
				activity.GetParty().GetSize().SetCurrentSize(0);
				activity.GetParty().GetSize().SetMaxSize(0);
				activity.GetSecrets().SetJoin("");
				activity.SetDetails("Main Menu");
				activity.SetState("On Main Menu");
				activity.GetAssets().SetLargeImage("northstar");
				activity.GetAssets().SetLargeText("Titanfall 2 + Northstar");
				activity.GetAssets().SetSmallImage("");
				activity.GetAssets().SetSmallText("");
				activity.GetTimestamps().SetEnd(0);
				break;
			case GameState::LOBBY:
				activity.GetParty().GetSize().SetCurrentSize(0);
				activity.GetParty().GetSize().SetMaxSize(0);
				activity.GetSecrets().SetJoin("");
				activity.SetDetails("Lobby");
				activity.SetState("In the Lobby");
				activity.GetAssets().SetLargeImage("northstar");
				activity.GetAssets().SetLargeText("Titanfall 2 + Northstar");
				activity.GetAssets().SetSmallImage("");
				activity.GetAssets().SetSmallText("");
				activity.GetTimestamps().SetEnd(0);
				break;
			case GameState::INGAME:
				activity.SetState(g_pPlugin->presence->playlist_displayname.c_str());
				activity.SetDetails(g_pPlugin->presence->map_displayname.c_str());
				activity.GetAssets().SetLargeImage(g_pPlugin->presence->map.c_str());
				activity.GetAssets().SetLargeText(g_pPlugin->presence->map_displayname.c_str());
				activity.GetParty().GetSize().SetCurrentSize(g_pPlugin->presence->current_players);
				activity.GetParty().GetSize().SetMaxSize(g_pPlugin->presence->max_players);
				if (g_pPlugin->presence->playlist == "campaign") {
					activity.GetParty().GetSize().SetCurrentSize(0);
					activity.GetParty().GetSize().SetMaxSize(0);
					activity.GetTimestamps().SetEnd(0);
				}
				else {
					activity.SetState(g_pPlugin->presence->playlist_displayname.c_str());
					details = fmt::format("Score: {} - {} (First to {})", g_pPlugin->presence->own_score, g_pPlugin->presence->other_highest_score, g_pPlugin->presence->max_score);
					activity.SetDetails(details.c_str());

					const auto p1 = std::chrono::system_clock::now().time_since_epoch();
					const auto test = std::chrono::duration_cast<std::chrono::seconds>(p1).count();
					activity.GetTimestamps().SetEnd(test + g_pPlugin->presence->timestamp_end);

					if (g_pPlugin->presence->password == "") {
						std::string invite = fmt::format("northstar://{}@{}", "server", g_pPlugin->presence->id);
						activity.GetSecrets().SetJoin(invite.c_str());
					}
					else if (Cvar_ns_discord_include_password != nullptr && Cvar_ns_discord_include_password->GetBool()) {
						std::string invite = fmt::format("northstar://{}@{}:{}", "server", g_pPlugin->presence->id, base64_encode(g_pPlugin->presence->password));
						activity.GetSecrets().SetJoin(invite.c_str());
					}

					if (Cvar_ns_discord_allow_join && Cvar_ns_discord_allow_join->GetBool())
						activity.GetParty().SetPrivacy(discord::ActivityPartyPrivacy::Public);
					else
						activity.GetParty().SetPrivacy(discord::ActivityPartyPrivacy::Private);
				}
				break;
			}

			previousGameState = g_pPlugin->presence->state;

			g_pPlugin->mutex.unlock();

			state.core->ActivityManager().UpdateActivity(
				activity, [](discord::Result result) {});
		}
	} while (!interrupted);

	return 0;
}

void Plugin::Main() {
	std::thread discord(PluginLoop);
	discord.detach();
}

void Plugin::DLLLoadEngine() {
	Cvar_ns_discord_allow_join = new ConVar("ns_discord_allow_join", "0", FCVAR_ARCHIVE, "test 123");
	Cvar_ns_discord_include_password = new ConVar("ns_discord_include_password", "0", FCVAR_ARCHIVE, "test 123");
}

void Plugin::DLLLoadClient() {}
void Plugin::DLLLoadServer() {}