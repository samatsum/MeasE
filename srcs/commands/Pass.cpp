#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include <iostream>
#include <unistd.h>

/*

PASS（1459）
パスワードは、接続する前に設定する必要がある。
461 ERR_NEEDMOREPARAMS パラメータ不足
462 ERR_ALREADYREGISTRED すでに登録済み
464 ERR_PASSWDMISMATCH パスワード不一致（オペレーター関連かも）

*/


void CommandHandler::handlePass(const Message& msg, Client& client) {

	if (client.isRegistered()) {
		sendMsg(client.getFd(), "462", client.getNickName(), "You may not reregister");
		return;
	}

	if (msg.params.empty()) {
		sendMsg(client.getFd(), "461", "PASS", "Not enough parameters");
		return;
	}

	if (msg.params[0] == m_server.getPassword()) {

		client.setAuthenticated(true);
		std::cout << "[fd " << client.getFd() << "] PASS accepted" << std::endl;
	} else {

		sendMsg(client.getFd(), "464", client.getNickName(), "Password incorrect");
		//総当り攻撃を許すことになるので、即座に接続を閉じる。
		std::cout << "[fd " << client.getFd() << "] PASS incorrect, closing connection" << std::endl;
		m_server.requestClientClose(client.getFd());
		return;
	}

	tryRegister(client);
}
