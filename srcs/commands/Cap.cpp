#include "../../includes/CommandHandler.hpp"
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <iostream>

//IRCv3の対応で、まあ無視でもいいと思うが、一応。
void CommandHandler::handleCAP(const Message& msg, Client& client)
{
	if (msg.params.empty()) {
		std::cout << "[fd " << client.getFd() << "] CAP called with no params" << std::endl;
		return;
	}

	std::string subcmd = makeUppercase(msg.params[0]);
	std::string nick;

	if (client.getNickName().empty()) {
		nick = "*";
	} else {
		nick = client.getNickName();
	}

	if (subcmd == "LS") {
		// 現状、サポートしているCAPはないので、空で返す
		std::string reply = ":" + m_serverName + " CAP " + nick + " LS :\r\n";
		if (send(client.getFd(), reply.c_str(), reply.size(), 0) == -1) {

			std::cerr << "Send CAP LS reply failed: " << std::strerror(errno) << std::endl;
		}
		std::cout << "[fd " << client.getFd() << "] CAP LS -> (no capabilities)" << std::endl;
	}
	else if (subcmd == "END") {
		
		std::cout << "[fd " << client.getFd() << "] CAP END" << std::endl;
	}
	else {

		std::cout << "[fd " << client.getFd() << "] CAP subcommand ignored: " 
		          << subcmd << std::endl;
	}
}
