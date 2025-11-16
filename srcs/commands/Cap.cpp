#include "../../includes/CommandHandler.hpp"

void CommandHandler::handleCAP(const Message& msg, Client& client)
{
	if (msg.params.empty()) {

		sendError(client, "461", "CAP", "Not enough parameters");
		return;
	}

	std::string subcmd = makeUppercase(msg.params[0]);

	std::string nick;
	if (client.getNickName().empty())
		nick = "*";
	else
		nick = client.getNickName();

	if (subcmd == "LS") {

		std::vector<std::string> params;
		params.push_back(nick);
		params.push_back("LS");

		std::string out = buildMessage(m_serverName, "CAP", params, "");
		sendMsg(client.getFd(), out);

		std::cout << "[fd " << client.getFd() << "] CAP LS -> (no capabilities)" << std::endl;
	}
	else if (subcmd == "END") {

		std::cout << "[fd " << client.getFd() << "] CAP END" << std::endl;
	}
	else {

		std::cout << "[fd " << client.getFd() << "] CAP ignored: " 
		          << subcmd << std::endl;
	}
}
