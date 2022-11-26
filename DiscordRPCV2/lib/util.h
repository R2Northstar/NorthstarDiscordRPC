#include "../pch.h"

#include <shared_mutex>

class ServerDataClass {
	public:
		std::string id;
		std::string name;
		std::string description;
		std::string password;

		bool isLocal;
};

class GameStateDataClass {
	public:
		std::string map;
		std::string mapDisplayname;
		std::string playlist;
		std::string playlistDisplayName;

		int currentPlayers;
		int maxPlayers;

		int ownScore;
		int otherHighestScore;
		int maxScore;

		int timestamp;
};