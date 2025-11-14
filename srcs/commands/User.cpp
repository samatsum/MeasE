#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include <iostream>
#include <unistd.h>

/*
1459準拠

*/

//USERコマンドの処理
void CommandHandler::handleUser(const Message& msg, Client& client)
{
	if (client.isRegistered()) {
		sendError(client, "462", "USER", "You may not reregister");
		return;
	}

	if (msg.params.size() < 3) {
		sendError(client, "461", "USER", "Not enough parameters");
		return;
	}

	client.setUserName(msg.params[0]);
	client.setRealName(msg.trailing);
	std::cout << "[fd " << client.getFd() << "] USER set to "
	          << msg.params[0] << " (" << client.getRealName() << ")" << std::endl;

	tryRegister(client);
}
