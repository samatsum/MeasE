#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Channel.hpp"
#include <sstream>
#include <iostream>

/*
なんでこのコマンドを実装したのか、よく覚えてない。

*/

typedef std::map<std::string, Channel>::iterator ChannelIt;

void CommandHandler::handleNames(const Message& msg, Client& client)
{
	if (!requireAuth(client, "NAMES"))
		return;

	std::string target;
	if (!msg.params.empty())
		target = msg.params[0];

	//チャンネル指定なし:すべてのチャンネルを返す 
	if (target.empty())
	{
		std::map<std::string, Channel>& channels = m_server.getChannels();
		for (ChannelIt it = channels.begin(); it != channels.end(); ++it)
		{
			const Channel& ch = it->second;
			const std::map<int, Client*>& members = ch.getMembers();

			std::ostringstream oss;
			for (std::map<int, Client*>::const_iterator mit = members.begin(); mit != members.end(); ++mit)
			{
				const Client* c = mit->second;
				if (ch.isOperator(c->getFd()))
					oss << "@" << c->getNickName() << " ";
				else
					oss << c->getNickName() << " ";
			}

			std::vector<std::string> params;
			params.push_back(client.getNickName());
			params.push_back("=");
			params.push_back(ch.getName());
			sendMsg(client.getFd(), "", "353", params, oss.str());
			sendChanReply(client.getFd(), "366", client.getNickName(), ch.getName(), "End of NAMES list");
		}
		return;
	}

	Channel* ch = m_server.findChannel(target);
	if (!ch)
	{
		sendChanReply(client.getFd(), "366", client.getNickName(), target, "End of NAMES list");
		return;
	}

	std::ostringstream oss;
	const std::map<int, Client*>& members = ch->getMembers();
	for (std::map<int, Client*>::const_iterator mit = members.begin(); mit != members.end(); ++mit)
	{
		const Client* c = mit->second;
		if (ch->isOperator(c->getFd()))
			oss << "@" << c->getNickName() << " ";
		else
			oss << c->getNickName() << " ";
	}

	std::vector<std::string> params;
	params.push_back(client.getNickName());
	params.push_back("=");
	params.push_back(ch->getName());
	sendMsg(client.getFd(), "", "353", params, oss.str());

	sendChanReply(client.getFd(), "366", client.getNickName(), ch->getName(), "End of NAMES list");
}
