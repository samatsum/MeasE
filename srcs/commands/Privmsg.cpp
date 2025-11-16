#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"

void CommandHandler::handlePrivmsg(const Message& msg, Client& client)
{
	if (!requireAuth(client, "PRIVMSG"))
		return;
	if (msg.params.empty()) {
		sendError(client, "411", "PRIVMSG", "No recipient given (PRIVMSG)");
		std::cout << "[fd " << client.getFd() << "] PRIVMSG missing target" << std::endl;
		return;
	}
	if (msg.trailing.empty()) {
		sendError(client, "412", "PRIVMSG", "No text to send");
		std::cout << "[fd " << client.getFd() << "] PRIVMSG missing message" << std::endl;
		return;
	}

	const std::string& target = msg.params[0];
	const std::string& message = msg.trailing;
	if (!message.empty() && message[0] == '!')
	{
		handleBot(msg, client);
		return;
	}

	if (!target.empty() && target[0] == '#')
	{
		Channel* channel = m_server.findChannel(target);
		if (!channel) {
			sendError(client, "403", target, "No such channel");
			std::cout << "[fd " << client.getFd() << "] PRIVMSG failed: channel not found (" << target << ")" << std::endl;
			return;
		}

		if (!channel->hasMember(client.getFd())) {
			sendError(client, "404", target, "Cannot send to channel");
			std::cout << "[fd " << client.getFd() << "] PRIVMSG rejected (not in channel " << target << ")" << std::endl;
			return;
		}

		std::string prefix = client.makePrefix();
		std::vector<std::string> params;
		params.push_back(target);
		std::string out = buildMessage(prefix, "PRIVMSG", params, message);

		channel->broadcast(out, client.getFd());
		std::cout << "[fd " << client.getFd() << "] -> [channel " << target << "] PRIVMSG: " << message << std::endl;
		return;
	}

	//for user
	std::map<int, Client>& clients = m_server.getClients();
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if (it->second.getNickName() == target)
		{
			std::string prefix = client.makePrefix();
			std::vector<std::string> params;
			params.push_back(target);
			std::string out = buildMessage(prefix, "PRIVMSG", params, message);

			m_server.queueMessageForClient(it->first, out);
			std::cout << "[fd " << client.getFd() << "] -> [fd " << it->first << "] PRIVMSG: " << message << std::endl;
			return;
		}
	}

	sendError(client, "401", target, "No such nick/channel");
	std::cout << "[fd " << client.getFd() << "] PRIVMSG target not found: " << target << std::endl;
}

/*
Error　Reply（2812）
401 ERR_NOSUCHNICK - ニックネームなし
402　→マルチサーバなので関係なし
403　ERR_NOSUCHCHANNEL - チャンネルなし
404　ERR_CANNOTSENDTOCHAN - チャンネルに送信できない
405　一度に多くのチャンネルに送信しようとした場合（2つ以上を禁止するべきか、テキスト再確認

2812系のPRIVMSGエラー一覧
411 ERR_NORECIPIENT - 宛先なし
412 ERR_NOTEXTTOSEND - 送信テキストなし
413　ERR_NOTOPLEVEL -?
414　ERR_WILDTOPLEVEL -?
415 ERR_BADMASK -?
413から415系はマスクの不正利用、よお知らん。

*/
