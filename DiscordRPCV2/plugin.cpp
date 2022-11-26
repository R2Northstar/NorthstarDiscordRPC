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

void* (*getPluginData)(PluginObject);

bool wasInGame;
bool resetSinglePlayerTimer = true;

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

int main(int, char**)
{

    if (strstr(GetCommandLineA(), "-waitfordebugger")) {
        while (!IsDebuggerPresent()) {
            Sleep(100);
        }
    }

    char path[MAX_PATH]{};
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exepath = std::string(path) + " -discord";

    DiscordState state{};

    discord::Core* core{};

    auto result = discord::Core::Create(938555054145818704, DiscordCreateFlags_Default, &core);
    state.core.reset(core);

    if (!state.core) {
        std::cout << "Failed to instantiate discord core! (err " << static_cast<int>(result)
            << ")\n";
        std::exit(-1);
    }

    state.core->SetLogHook(
        discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
            std::cerr << "Log(" << static_cast<uint32_t>(level) << "): " << message << "\n";
        });

    state.core->ActivityManager().RegisterCommand(exepath.c_str());

    discord::Activity activity{};

    state.core->ActivityManager().OnActivityJoin.Connect(
        [&activity](const char* secret) {
            std::cout << "Join " << secret << "\n";
			g_pPlugin->RelayInvite(secret);
            activity.GetParty().SetId(generateUUID().c_str());;
        });
    state.core->ActivityManager().OnActivityJoinRequest.Connect([](discord::User const& user) {
        std::cout << "Join Request " << user.GetUsername() << "\n";
        });
    state.core->ActivityManager().OnActivityInvite.Connect(
        [](discord::ActivityActionType, discord::User const& user, discord::Activity const&) {
            std::cout << "Invite " << user.GetUsername() << "\n";
        });


    activity.SetDetails("TF3SDK");
    activity.SetState("TF3SDK Debug Build 0.1.4");
    activity.GetAssets().SetSmallImage("the");
    activity.GetAssets().SetSmallText("i mage");
    activity.GetAssets().SetLargeImage("the");
    activity.GetAssets().SetLargeText("u mage");

    activity.GetSecrets().SetJoin("JOIN_SECRET_MAIN");

    activity.GetParty().GetSize().SetCurrentSize(1);
    activity.GetParty().GetSize().SetMaxSize(5);

    activity.GetParty().SetId(generateUUID().c_str());
    activity.GetParty().SetPrivacy(discord::ActivityPartyPrivacy::Public);
    activity.SetType(discord::ActivityType::Playing);
    state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
        std::cout << ((result == discord::Result::Ok) ? "Succeeded" : "Failed")
            << " updating activity!\n";
        });

    std::signal(SIGINT, [](int) { interrupted = true; });

	do
	{
		state.core->RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		std::string details = "Score: ";

		if (g_pPlugin->mutex.try_lock_shared()) {

			activity.SetState(g_pPlugin->gameState->playlistDisplayName.c_str());
			activity.GetAssets().SetLargeImage(g_pPlugin->gameState->map.c_str());
			activity.GetAssets().SetLargeText(g_pPlugin->gameState->mapDisplayname.c_str());
			if (g_pPlugin->gameState->map == "")
			{
				activity.GetParty().GetSize().SetCurrentSize(0);
				activity.GetParty().GetSize().SetMaxSize(0);
				activity.SetDetails("Main Menu");
				activity.SetState("On Main Menu");
				activity.GetAssets().SetLargeImage("northstar");
				activity.GetAssets().SetLargeText("Titanfall 2 + Northstar");
				activity.GetAssets().SetSmallImage("");
				activity.GetAssets().SetSmallText("");
				activity.GetTimestamps().SetEnd(0);
				if (wasInGame)
				{
					const auto p1 = std::chrono::system_clock::now().time_since_epoch();
					activity.GetTimestamps().SetStart(std::chrono::duration_cast<std::chrono::seconds>(p1).count());
					wasInGame = false;
					resetSinglePlayerTimer = true;
				}
			}
			else if (g_pPlugin->gameState->map == "mp_lobby")
			{
				activity.SetState("In the Lobby");
				activity.GetParty().GetSize().SetCurrentSize(0);
				activity.GetParty().GetSize().SetMaxSize(0);
				activity.SetDetails("Lobby");
				activity.GetAssets().SetLargeImage("northstar");
				activity.GetAssets().SetLargeText("Titanfall 2 + Northstar");
				activity.GetAssets().SetSmallImage("");
				activity.GetAssets().SetSmallText("");
				activity.GetTimestamps().SetEnd(0);
				if (wasInGame)
				{
					const auto p1 = std::chrono::system_clock::now().time_since_epoch();
					activity.GetTimestamps().SetStart(std::chrono::duration_cast<std::chrono::seconds>(p1).count());
					wasInGame = false;
					resetSinglePlayerTimer = true;
				}
			}
			else
			{
				if (true)
				{
					// Hack to make singleplayer work for now
					activity.GetParty().GetSize().SetCurrentSize(g_pPlugin->gameState->currentPlayers);
					activity.GetParty().GetSize().SetMaxSize(g_pPlugin->gameState->maxPlayers);
					if (g_pPlugin->gameState->playlist == "campaign") {
						activity.SetState(g_pPlugin->gameState->playlistDisplayName.c_str());
						activity.SetDetails(g_pPlugin->gameState->mapDisplayname.c_str());
						activity.GetParty().GetSize().SetCurrentSize(0);
						activity.GetParty().GetSize().SetMaxSize(0);
						activity.GetTimestamps().SetEnd(0);
						if (resetSinglePlayerTimer) {
							const auto p1 = std::chrono::system_clock::now().time_since_epoch();
							activity.GetTimestamps().SetStart(std::chrono::duration_cast<std::chrono::seconds>(p1).count());
							resetSinglePlayerTimer = false;
						}
					}
					else {
						activity.SetState(g_pPlugin->gameState->playlistDisplayName.c_str());
						details += (g_pPlugin->gameState->ownScore + " - " + g_pPlugin->gameState->otherHighestScore);

						details += " (First to " + std::to_string(g_pPlugin->gameState->maxScore) + ")";
						activity.SetDetails(details.c_str());
						const auto p1 = std::chrono::system_clock::now().time_since_epoch();
						if (g_pPlugin->gameState->timestamp > 0) {
							activity.GetTimestamps().SetEnd(std::chrono::duration_cast<std::chrono::seconds>(p1).count() + g_pPlugin->gameState->timestamp);
							activity.GetTimestamps().SetStart(0);
						}
						std::string invite = fmt::format("northstar://{}@{}:{}", "server", g_pPlugin->server->id, base64_encode(g_pPlugin->server->password));
						activity.GetSecrets().SetJoin(invite.c_str());
						resetSinglePlayerTimer = true;
					}
					wasInGame = true;
				}
				else
				{
					activity.GetParty().GetSize().SetCurrentSize(0);
					activity.GetParty().GetSize().SetMaxSize(0);
					activity.SetDetails("Loading...");
					if (wasInGame) {
						wasInGame = false;
						resetSinglePlayerTimer = true;
					}
				}
			}

			g_pPlugin->mutex.unlock();

			state.core->ActivityManager().UpdateActivity(
				activity, [](discord::Result result) {});
		}

		g_pPlugin->RequestServerData();
		g_pPlugin->RequestGameStateData();
	} while (!interrupted);

    return 0;
}


void Plugin::Main() {
	std::thread discord(main, 0, (char**)0);
	discord.detach();
}