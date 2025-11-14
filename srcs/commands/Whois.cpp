#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <vector>
#include <string>

/*

311 RPL_WHOISUSER ニック、ユーザー名、ホスト名、実名
312 RPL_WHOISSERVER　接続してるサーバー情報
318 RPL_ENDOFWHOIS　応答の終了
401 ERR_NOSUCHNICK　いねえ
431 ERR_NONICKNAMEGIVEN　ニックなし。
サーバーなし。

エラーは、NONICKNAMEGIVEN、NOSUCHNICK、NOSUCHSERVERのみのエラーハンドリングで行ける。
AWAYは対応しない、オペレーターと、ワイルドカードは未実装。

*/

void CommandHandler::handleWhois(const Message& msg, Client& client)
{
	if (!requireAuth(client, "WHOIS"))
		return;
	if (msg.params.empty()) {
		sendError(client, "431", "WHOIS", "No nickname given");
		return;
	}

	std::string targetNick = msg.params[0];
	Client* target = m_server.findClientByNick(targetNick);

	//該当なし
	if (!target) {
		sendError(client, "401", targetNick, "No such nick");
		return;
	}

	//RPL_WHOISUSER
	{
		std::vector<std::string> params;
		params.push_back(client.getNickName());
		params.push_back(target->getNickName());
		params.push_back(target->getUserName());
		params.push_back(target->getHost());
		params.push_back("*");
		sendMsg(client.getFd(), buildMessage("", "311", params, target->getRealName()));
	}

	//RPL_WHOISSERVER
	{
		std::vector<std::string> params;
		params.push_back(client.getNickName());
		params.push_back(target->getNickName());
		params.push_back(m_server.getServerName());
		sendMsg(client.getFd(), buildMessage("", "312", params, "ft_irc server"));
	}

	//RPL_ENDOFWHOIS
	{
		std::vector<std::string> params;
		params.push_back(client.getNickName());
		params.push_back(target->getNickName());
		sendMsg(client.getFd(), buildMessage("", "318", params, "End of WHOIS list"));
	}
}
