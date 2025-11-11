#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Channel.hpp"
#include <sstream>
#include <iostream>

typedef std::map<std::string, Channel>::iterator ChannelIt;

void CommandHandler::handleNames(const Message& msg, Client& client)
{
	std::string target;
	if (msg.params.empty())
		target = "";
	else
		target = msg.params[0];

	if (target.empty())
	{
		std::map<std::string, Channel>& channels = m_server.getChannels();
		for (ChannelIt it = channels.begin(); it != channels.end(); ++it)
		{
			const Channel& ch = it->second;
			std::ostringstream oss;
			const std::map<int, Client*>& members = ch.getMembers();

			for (std::map<int, Client*>::const_iterator mit = members.begin(); mit != members.end(); ++mit)
				oss << mit->second->getNickName() << " ";

			sendMsg(client.getFd(), "353", client.getNickName(),
			        "= " + ch.getName() + " :" + oss.str());
			sendMsg(client.getFd(), "366", client.getNickName(),
			        ch.getName() + " :End of NAMES list");
		}
		return;
	}

	// 指定チャンネルのみ
	Channel* ch = m_server.findChannel(target);
	if (!ch)
	{
		sendMsg(client.getFd(), "366", client.getNickName(),
		        target + " :End of NAMES list");
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

	sendMsg(client.getFd(), "353", client.getNickName(),
	        "= " + ch->getName() + " :" + oss.str());
	sendMsg(client.getFd(), "366", client.getNickName(),
	        ch->getName() + " :End of NAMES list");
}
