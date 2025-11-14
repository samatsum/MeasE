#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include <sys/socket.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cerrno>

/*
気が向いたら処理する。

*/

void CommandHandler::handleQuit(const Message& msg, Client& client)
{
	std::string message;

	if (!msg.trailing.empty())
		message = msg.trailing;
	else if (!msg.params.empty())
		message = msg.params[0];
	else
		message = "Client Quit";

	if (client.isRegistered()) {
		std::string prefix = client.makePrefix();
		std::string quitMsg = buildMessage(prefix, "QUIT", std::vector<std::string>(), message);

		m_server.removeClientFromAllChannels(client.getFd());
		m_server.broadcastAll(quitMsg);

		std::cout << "[fd " << client.getFd() << "] QUIT (registered): " << message << std::endl;
	} else {
		std::cout << "[fd " << client.getFd() << "] QUIT (unregistered): " << message << std::endl;
	}

	m_server.requestClientClose(client.getFd());
}
