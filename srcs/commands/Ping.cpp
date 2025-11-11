#include "../../includes/CommandHandler.hpp"
#include <sys/socket.h>//これいらんやろ。
#include <cerrno>
#include <cstring>
#include <iostream>

/*
Error Reply
409 ERR_NOORIGIN- 発信元パラメータがない PING または PONG メッセージ。(2812)

*/

void CommandHandler::handlePing(const Message& msg, Client& client)
{
	std::string token;
	if (!msg.trailing.empty()) {
		token = msg.trailing;
	} else if (!msg.params.empty()) {
		token = msg.params[0];
	} else {
		sendMsg(client.getFd(), "409", client.getNickName(), "No origin specified");
		std::cout << "[fd " << client.getFd() << "] PING missing trailing" << std::endl;
		return;
	}

	std::string out = "PONG :" + token + "\r\n";
	if (send(client.getFd(), out.c_str(), out.size(), 0) == -1) {
		std::cerr << "Send PONG failed: " << std::strerror(errno) << std::endl;
	}
	std::cout << "[fd " << client.getFd() << "] PONG sent: " << token << std::endl;
}
