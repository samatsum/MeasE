#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Channel.hpp"
#include <sstream>
#include <iostream>

/*
ユーザーやチャンネルの誰がどこにいるか、どんな状態か、

引数なしは、ユーザーの所属。
引数がチャンネルなら、全メンバー
引数がニックなら、そのユーザー情報

:server 352 <requester> <channel> <user> <host> <server> <nick> <flags> :<hopcount> <realname>

チャンネル内では表示されない？

共通の処理が多いから合理化もできそうだが。
存在しない場合の挙動は、一応最後にしてるが、時間があったら場所を変更（致命的ではない。）

*/

void CommandHandler::handleWho(const Message& msg, Client& client)
{
	if (!requireAuth(client, "WHO"))
		return;

	std::string target;
	if (msg.params.empty())
		target = "";
	else
		target = msg.params[0];

	//引数なし、所属するチャンネル一覧だが、見れない？
	if (target.empty())
	{
		Channel* userChan = m_server.findChannelByMember(client.getFd());
		if (!userChan)
		{
			std::vector<std::string> params;
			params.push_back(client.getNickName());
			params.push_back("*");
			sendMsg(client.getFd(), buildMessage("", "315", params, "End of WHO list"));
			return;
		}

		const std::map<int, Client*>& members = userChan->getMembers();
		for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it)
		{
			Client* c = it->second;
			std::vector<std::string> params;
			params.push_back(client.getNickName());      // <requester>
			params.push_back(userChan->getName());       // <channel>
			params.push_back(c->getUserName());          // <user>
			params.push_back(c->getHost());              // <host>
			params.push_back(m_server.getServerName());  // <server>
			params.push_back(c->getNickName());          // <nick>
			params.push_back("H");                       // <flags>(H=here,G=gone,@=op)
			sendMsg(client.getFd(), buildMessage("", "352", params, "0 " + c->getRealName()));
		}

		std::vector<std::string> endParams;
		endParams.push_back(client.getNickName());
		endParams.push_back(userChan->getName());
		sendMsg(client.getFd(), buildMessage("", "315", endParams, "End of WHO list"));
		return;
	}

	//チャンネル指定時
	Channel* ch = m_server.findChannel(target);
	if (ch)
	{
		const std::map<int, Client*>& members = ch->getMembers();
		for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it)
		{
			Client* c = it->second;
			std::vector<std::string> params;
			params.push_back(client.getNickName());
			params.push_back(ch->getName());
			params.push_back(c->getUserName());
			params.push_back(c->getHost());
			params.push_back(m_server.getServerName());
			params.push_back(c->getNickName());
			params.push_back("H");
			sendMsg(client.getFd(), buildMessage("", "352", params, "0 " + c->getRealName()));
		}

		std::vector<std::string> endParams;
		endParams.push_back(client.getNickName());
		endParams.push_back(ch->getName());
		sendMsg(client.getFd(), buildMessage("", "315", endParams, "End of WHO list"));
		return;
	}

	//ニックネーム指定時
	std::map<int, Client>& clients = m_server.getClients();
	bool found = false;
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		Client& c = it->second;
		if (c.getNickName() == target)
		{
			found = true;
			std::vector<std::string> params;
			params.push_back(client.getNickName());
			params.push_back("*");
			params.push_back(c.getUserName());
			params.push_back(c.getHost());
			params.push_back(m_server.getServerName());
			params.push_back(c.getNickName());
			params.push_back("H");
			sendMsg(client.getFd(), buildMessage("", "352", params, "0 " + c.getRealName()));
			break;
		}
	}

	std::vector<std::string> endParams;
	endParams.push_back(client.getNickName());
	endParams.push_back(target);
	sendMsg(client.getFd(), buildMessage("", "315", endParams, "End of WHO list"));

	if (!found && !target.empty() && target[0] != '#')
		sendError(client, "401", target, "No such nick");
}
