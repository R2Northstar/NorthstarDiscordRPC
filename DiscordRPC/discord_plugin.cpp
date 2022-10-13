// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "dllmain.h"
#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>
#include <chrono>
#include <algorithm>


#include "library/discord.h"

#define DLLEXPORT __declspec(dllexport)
#include "plugin_abi.h" // Make sure you copy over plugin_abi.h from the NorthstarLauncher project before building

#if defined(_WIN32)
#pragma pack(push, 1)

struct BitmapImageHeader
{
	uint32_t const structSize{ sizeof(BitmapImageHeader) };
	int32_t width{ 0 };
	int32_t height{ 0 };
	uint16_t const planes{ 1 };
	uint16_t const bpp{ 32 };
	uint32_t const pad0{ 0 };
	uint32_t const pad1{ 0 };
	uint32_t const hres{ 2835 };
	uint32_t const vres{ 2835 };
	uint32_t const pad4{ 0 };
	uint32_t const pad5{ 0 };

	BitmapImageHeader& operator=(BitmapImageHeader const&) = delete;
};

struct BitmapFileHeader
{
	uint8_t const magic0{ 'B' };
	uint8_t const magic1{ 'M' };
	uint32_t size{ 0 };
	uint32_t const pad{ 0 };
	uint32_t const offset{ sizeof(BitmapFileHeader) + sizeof(BitmapImageHeader) };

	BitmapFileHeader& operator=(BitmapFileHeader const&) = delete;
};
#pragma pack(pop)
#endif

struct DiscordState
{
	discord::User currentUser;

	std::unique_ptr<discord::Core> core;
};

namespace
{
	volatile bool interrupted{ false };
}

GameState* gameStatePtr = 0;
ServerInfo* serverInfoPtr = 0;
PlayerInfo* playerInfoPtr = 0;

void* (*getPluginData)(PluginObject);

extern "C" DLLEXPORT void initializePlugin(void* getPluginData_external)
{
	getPluginData = (void* (*)(PluginObject))getPluginData_external;
	gameStatePtr = (GameState*)getPluginData(PluginObject::GAMESTATE);
	serverInfoPtr = (ServerInfo*)getPluginData(PluginObject::SERVERINFO);
	playerInfoPtr = (PlayerInfo*)getPluginData(PluginObject::PLAYERINFO);
	std::thread discord(main, 0, (char**)0);
	discord.detach();
}

DiscordState state{};
bool wasInGame;
bool resetSinglePlayerTimer = true;

static struct PluginData
{
	char map[32];
	char mapDisplayName[64];
	char playlist[32];
	char playlistDisplayName[64];
	int players;
	int maxPlayers;
	bool loading;
	int ourScore;
	int secondHighestScore;
	int highestScore;
	int scoreLimit;
	int endTime;
} pluginData;

int main(int, char**)
{
	char path[MAX_PATH]{};
	GetModuleFileNameA(NULL, path, MAX_PATH);
	std::string exepath{ path };
	exepath += " -discord";

    DiscordState state{};

    discord::Core* core{};

    SetEnvironmentVariable(L"DISCORD_INSTANCE_ID", L"1");
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

    //state.core->ActivityManager().RegisterCommand("steam://run-game-id/123");
    state.core->ActivityManager().RegisterSteam(123);

    state.core->ActivityManager().OnActivityJoin.Connect(
        [](const char* secret) {
            std::cout << "Join " << secret << "\n";
        });
    state.core->ActivityManager().OnActivitySpectate.Connect(
        [](const char* secret) { std::cout << "Spectate " << secret << "\n"; });
    state.core->ActivityManager().OnActivityJoinRequest.Connect([](discord::User const& user) {
        std::cout << "Join Request " << user.GetUsername() << "\n";
        });
    state.core->ActivityManager().OnActivityInvite.Connect(
        [](discord::ActivityActionType, discord::User const& user, discord::Activity const&) {
            std::cout << "Invite " << user.GetUsername() << "\n";
        });

    state.core->LobbyManager().OnLobbyUpdate.Connect(
        [](std::int64_t lobbyId) { std::cout << "Lobby update " << lobbyId << "\n"; });

    state.core->LobbyManager().OnLobbyDelete.Connect(
        [](std::int64_t lobbyId, std::uint32_t reason) {
            std::cout << "Lobby delete " << lobbyId << " (reason: " << reason << ")\n";
        });

    state.core->LobbyManager().OnMemberConnect.Connect(
        [](std::int64_t lobbyId, std::int64_t userId) {
            std::cout << "Lobby member connect " << lobbyId << " userId " << userId << "\n";
        });

    state.core->LobbyManager().OnMemberUpdate.Connect(
        [](std::int64_t lobbyId, std::int64_t userId) {
            std::cout << "Lobby member update " << lobbyId << " userId " << userId << "\n";
        });

    state.core->LobbyManager().OnMemberDisconnect.Connect(
        [](std::int64_t lobbyId, std::int64_t userId) {
            std::cout << "Lobby member disconnect " << lobbyId << " userId " << userId << "\n";
        });

    state.core->LobbyManager().OnLobbyMessage.Connect([&](std::int64_t lobbyId,
        std::int64_t userId,
        std::uint8_t* payload,
        std::uint32_t payloadLength) {
            std::vector<uint8_t> buffer{};
            buffer.resize(payloadLength);
            memcpy(buffer.data(), payload, payloadLength);
            std::cout << "Lobby message " << lobbyId << " from " << userId << " of length "
                << payloadLength << " bytes.\n";

            char fourtyNinetySix[4096];
            state.core->LobbyManager().GetLobbyMetadataValue(lobbyId, "foo", fourtyNinetySix);

            std::cout << "Metadata for key foo is " << fourtyNinetySix << "\n";
        });

    state.core->LobbyManager().OnSpeaking.Connect(
        [&](std::int64_t, std::int64_t userId, bool speaking) {
            std::cout << "User " << userId << " is " << (speaking ? "" : "NOT ") << "speaking.\n";
        });

    discord::Activity activity{};
    activity.SetDetails("TF3SDK");
    activity.SetState("TF3SDK Debug Build 0.1.4");
    activity.GetAssets().SetSmallImage("the");
    activity.GetAssets().SetSmallText("i mage");
    activity.GetAssets().SetLargeImage("the");
    activity.GetAssets().SetLargeText("u mage");

    activity.GetParty().GetSize().SetCurrentSize(1);
    activity.GetParty().GetSize().SetMaxSize(5);
    activity.GetParty().SetId("party id");
    activity.GetParty().SetPrivacy(discord::ActivityPartyPrivacy::Public);
    activity.SetType(discord::ActivityType::Playing);
    state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
        std::cout << ((result == discord::Result::Ok) ? "Succeeded" : "Failed")
            << " updating activity!\n";
        });

    discord::LobbyTransaction lobby{};
    state.core->LobbyManager().GetLobbyCreateTransaction(&lobby);
    lobby.SetCapacity(2);
    lobby.SetMetadata("foo", "bar");
    lobby.SetMetadata("baz", "bat");
    lobby.SetType(discord::LobbyType::Public);
    state.core->LobbyManager().CreateLobby(
        lobby, [&state](discord::Result result, discord::Lobby const& lobby) {
            if (result == discord::Result::Ok) {
                std::cout << "Created lobby with secret " << lobby.GetSecret() << "\n";
                std::array<uint8_t, 234> data{};
                state.core->LobbyManager().SendLobbyMessage(
                    lobby.GetId(),
                    reinterpret_cast<uint8_t*>(data.data()),
                    static_cast<uint32_t>(data.size()),
                    [](discord::Result result) {
                        std::cout << "Sent message. Result: " << static_cast<int>(result) << "\n";
                    });
            }
            else {
                std::cout << "Failed creating lobby. (err " << static_cast<int>(result) << ")\n";
            }

            discord::LobbySearchQuery query{};
            state.core->LobbyManager().GetSearchQuery(&query);
            query.Limit(1);
            state.core->LobbyManager().Search(query, [&state](discord::Result result) {
                if (result == discord::Result::Ok) {
                    std::int32_t lobbyCount{};
                    state.core->LobbyManager().LobbyCount(&lobbyCount);
                    std::cout << "Lobby search succeeded with " << lobbyCount << " lobbies.\n";
                    for (auto i = 0; i < lobbyCount; ++i) {
                        discord::LobbyId lobbyId{};
                        state.core->LobbyManager().GetLobbyId(i, &lobbyId);
                        std::cout << "  " << lobbyId << "\n";
                    }
                }
                else {
                    std::cout << "Lobby search failed. (err " << static_cast<int>(result) << ")\n";
                }
                });
        });

    std::signal(SIGINT, [](int) { interrupted = true; });

    do {
        state.core->RunCallbacks();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (!interrupted);

    return 0;
}
