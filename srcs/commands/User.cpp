#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include <iostream>
#include <unistd.h>

/*

*/

//USERコマンドの処理
void CommandHandler::handleUser(const Message& msg, Client& client) {

	if (client.isRegistered()) {
		sendMsg(client.getFd(), "462", client.getNickName(), "You may not reregister");
		return;
	}

	if (msg.params.empty()) {

		sendMsg(client.getFd(), "461", "USER", "Not enough parameters");
		return;
	}
	if (msg.params.size() < 3) {
		sendMsg(client.getFd(), "461", "USER", "Not enough parameters");
		return;
	}


	client.setUserName(msg.params[0]);
	client.setRealName(msg.params[1]);
	std::cout << "[fd " << client.getFd() << "] USER set to " << msg.params[0]
	          << " (" << msg.params[1] << ")" << std::endl;
	tryRegister(client);
}
