#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sys/socket.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cerrno>

/*
KICKは相手に通知されない？
セルフキックも可能。

複数チャンネルは対応しない。
*/

void CommandHandler::handleKick(const Message& msg, Client& client) {
	if (!client.isAuthenticated()) {
		sendMsg(client.getFd(), "451", client.getNickName(), "You have not registered");
		return;
	}
	if (msg.params.size() < 2) {
		sendMsg(client.getFd(), "461", client.getNickName(), "KICK :Not enough parameters");
		return;
	}

	std::string channelName = msg.params[0];
	std::vector<std::string> targetNicks;
	std::stringstream ss(msg.params[1]);
	std::string nick;
	while (std::getline(ss, nick, ',')) {
		targetNicks.push_back(nick);
	}
	std::string reason;
	if (msg.trailing.empty()) {
		reason = client.getNickName();
	} else {
		reason = msg.trailing;
	}

	Channel* channel = m_server.findChannel(channelName);
	if (!channel) {
		sendMsg(client.getFd(), "403", client.getNickName(), channelName + " :No such channel");
		return;
	}
	if (!channel->hasMember(client.getFd())) {
		sendMsg(client.getFd(), "442", client.getNickName(), channelName + " :You're not on that channel");
		return;
	}
	if (!channel->isOperator(client.getFd())) {
		sendMsg(client.getFd(), "482", client.getNickName(), channelName + " :You're not channel operator");
		return;
	}


	for (size_t i = 0; i < targetNicks.size(); ++i)
	{
		const std::string& targetNick = targetNicks[i];
		Client* targetClient = NULL;

		std::map<int, Client*>& members = const_cast<std::map<int, Client*>&>(channel->getMembers());
		for (std::map<int, Client*>::iterator it = members.begin(); it != members.end(); ++it)
		{
			if (it->second->getNickName() == targetNick)
			{
				targetClient = it->second;
				break;
			}
		}

		if (targetClient == NULL)
		{
			sendMsg(client.getFd(), "441", client.getNickName(),
					targetNick + " " + channelName + " :They aren't on that channel");
			continue;
		}

		std::ostringstream kickMsg;
		kickMsg << ":" << client.makePrefix()
				<< " KICK " << channelName << " " << targetNick << " :" << reason << "\r\n";

		channel->broadcast(kickMsg.str());
		channel->removeMember(targetClient->getFd());
		targetClient->leaveChannel(channelName);

		if (channel->getMemberCount() == 0)
		{
			m_server.removeChannel(channelName);
			break;
		}
	}
}

// void CommandHandler::handleKick(const Message& msg, Client& client) {
// 	if (!client.isAuthenticated()) {
// 		sendMsg(client.getFd(), "451", client.getNickName(), "You have not registered");
// 		return;
// 	}
// 	if (msg.params.size() < 2) {
// 		sendMsg(client.getFd(), "461", client.getNickName(), "KICK :Not enough parameters");
// 		return;
// 	}

// 	std::string channelName = msg.params[0];
// 	std::string targetNick = msg.params[1];
// 	std::string reason;
// 	if (msg.trailing.empty()) {
// 		reason = client.getNickName();
// 	} else {
// 		reason = msg.trailing;
// 	}

// 	Channel* channel = m_server.findChannel(channelName);
// 	if (!channel) {
// 		sendMsg(client.getFd(), "403", client.getNickName(), channelName + " :No such channel");
// 		return;
// 	}
// 	if (!channel->hasMember(client.getFd())) {
// 		sendMsg(client.getFd(), "442", client.getNickName(), channelName + " :You're not on that channel");
// 		return;
// 	}
// 	if (!channel->isOperator(client.getFd())) {
// 		sendMsg(client.getFd(), "482", client.getNickName(), channelName + " :You're not channel operator");
// 		return;
// 	}

// 	Client* targetClient = NULL;
// 	std::map<int, Client*>& members = const_cast<std::map<int, Client*>&>(channel->getMembers());
// 	for (std::map<int, Client*>::iterator it = members.begin(); it != members.end(); ++it) {
// 		if (it->second->getNickName() == targetNick) {
// 			targetClient = it->second;
// 			break;
// 		}
// 	}
// 	if (!targetClient) {
// 		sendMsg(client.getFd(), "441", client.getNickName(), targetNick + " " + channelName + " :They aren't on that channel");
// 		return;
// 	}

// 	std::ostringstream kickMsg;
// 	kickMsg << ":" << client.makePrefix()
// 	        << " KICK " << channelName << " " << targetNick << " :" << reason << "\r\n";
// 	channel->broadcast(kickMsg.str());

// 	channel->removeMember(targetClient->getFd());
// 	targetClient->leaveChannel(channelName);

// 	//セルフ削除時になる予感
// 	if (channel->getMemberCount() == 0) {
// 		m_server.removeChannel(channelName);
// 	}
// }
