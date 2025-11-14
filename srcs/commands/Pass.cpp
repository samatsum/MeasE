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


void CommandHandler::handlePass(const Message& msg, Client& client)
{
	//登録済みチェック
	if (client.isRegistered()) {
		sendError(client, "462", "PASS", "You may not reregister");
		return;
	}

	//パラメータ不足
	if (msg.params.empty()) {
		sendError(client, "461", "PASS", "Not enough parameters");
		return;
	}

	//パスワード確認
	const std::string& password = msg.params[0];
	if (password == m_server.getPassword()) {
		client.setAuthenticated(true);
		std::cout << "[fd " << client.getFd() << "] PASS accepted" << std::endl;
	} else {
		sendError(client, "464", "PASS", "Password incorrect");
		std::cout << "[fd " << client.getFd() << "] PASS incorrect, closing connection" << std::endl;
		m_server.requestClientClose(client.getFd()); // 総当たり防止のため切断
		return;
	}

	tryRegister(client);
}
