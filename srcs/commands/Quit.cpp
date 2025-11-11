#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include <sys/socket.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cerrno>

/*
登録確認はいらないが、
登録状況に応じた離脱処理が必要（結局登録確認が必要ってやつよ。）

*/

// QUITコマンド処理、チャンネルからの退室通知も行う。
void CommandHandler::handleQuit(const Message& msg, Client& client) {
	
	std::string message;

	// RFCではQUIT <Quit Message>が任意。空ならデフォルトを補う。
	if (!msg.trailing.empty())
		message = msg.trailing;
	else if (!msg.params.empty())
		message = msg.params[0];
	else
		message = "Client Quit";

	std::string prefix = client.makePrefix();
	std::string	quitMsg = ":" + prefix + " QUIT :" + message + "\r\n";

	// ここでRFC的には、他クライアント／チャンネルに通知が必要。
	if (client.isRegistered()) {
		m_server.removeClientFromAllChannels(client.getFd());
		m_server.broadcastAll(quitMsg);
		std::cout << "[fd " << client.getFd() << "] is quitting" << std::endl;
	}

	m_server.requestClientClose(client.getFd());
	std::cout << "[fd " << client.getFd() << "] QUIT: " << message << std::endl;
}
