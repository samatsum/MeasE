#include "../../includes/CommandHandler.hpp"



void CommandHandler::handlePing(const Message& msg, Client& client)
{
	std::string token;

	if (!msg.trailing.empty()) {
		token = msg.trailing;
	} else if (!msg.params.empty()) {
		token = msg.params[0];
	} else {
		sendError(client, "409", "PING", "No origin specified");
		std::cout << "[fd " << client.getFd() << "] PING missing token" << std::endl;
		return;
	}

	std::vector<std::string> params;
	params.push_back(m_serverName); // RFC2812: PONG <servername> :<token>
	std::string out = buildMessage("", "PONG", params, token);
	sendMsg(client.getFd(), out);

	std::cout << "[fd " << client.getFd() << "] PONG sent: " << token << std::endl;
}

/*
Error Reply
409 ERR_NOORIGIN- 発信元パラメータがない PING または PONG メッセージ。(2812)

*/