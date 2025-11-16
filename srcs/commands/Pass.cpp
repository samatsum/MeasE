#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"

void CommandHandler::handlePass(const Message& msg, Client& client)
{
	if (client.isRegistered()) {
		sendError(client, "462", "PASS", "You may not reregister");
		return;
	}

	if (msg.params.empty()) {
		sendError(client, "461", "PASS", "Not enough parameters");
		return;
	}

	const std::string& password = msg.params[0];
	if (password == m_server.getPassword()) {
		client.setAuthenticated(true);
		std::cout << "[fd " << client.getFd() << "] PASS accepted" << std::endl;
	} else {
		sendError(client, "464", "PASS", "Password incorrect");
		std::cout << "[fd " << client.getFd() << "] PASS incorrect, closing connection" << std::endl;
		m_server.requestClientClose(client.getFd());
		return;
	}

	tryRegister(client);
}

/*

PASS（1459）
パスワードは、接続する前に設定する必要がある。
461 ERR_NEEDMOREPARAMS パラメータ不足
462 ERR_ALREADYREGISTRED すでに登録済み
464 ERR_PASSWDMISMATCH パスワード不一致（オペレーター関連かも）

*/
