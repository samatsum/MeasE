#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sstream>
#include <iostream>
#include <cstdlib>

/*
324の返答が、ちとあとでチェック

・ユーザーモードのフラグ変更
たとえば、チャンネル

*/

void CommandHandler::handleMode(const Message& msg, Client& client) {
	if (!client.isAuthenticated())
		return sendMsg(client.getFd(), "451", client.getNickName(), "You have not registered");

	if (msg.params.empty())
		return sendMsg(client.getFd(), "461", client.getNickName(), "MODE :Not enough parameters");

	if (msg.params[0][0] != '#')
		return handleUserMode(msg, client);

	handleChannelMode(msg, client);
}

//ユーザーモードは非対応
void CommandHandler::handleUserMode(const Message& msg, Client& client) {
	(void)msg;
	sendMsg(client.getFd(), "501", client.getNickName(), ":User modes are not supported on this server");
}

void CommandHandler::handleChannelMode(const Message& msg, Client& client) {

	const std::string& target = msg.params[0];
	Channel* channel = m_server.findChannel(target);

	if (!channel)
		return sendMsg(client.getFd(), "403", client.getNickName(), target + " :No such channel");
	if (!channel->hasMember(client.getFd()))
		return sendMsg(client.getFd(), "442", client.getNickName(), target + " :You're not on that channel");

	// --- モード確認だけ ---
	if (msg.params.size() == 1)
		return replyChannelModes(client, *channel, target);

	// --- 権限チェック ---
	if (!channel->isOperator(client.getFd()))
		return sendMsg(client.getFd(), "482", client.getNickName(), target + " :You're not channel operator");

	applyChannelMode(msg, client, *channel);
}

void CommandHandler::replyChannelModes(Client& client, Channel& channel, const std::string& target) {
	const ChannelModes& modes = channel.getModes();

	std::ostringstream modeLine;
	modeLine << "+";
	if (modes.inviteOnly) modeLine << "i";
	if (modes.topicProtected) modeLine << "t";
	if (!modes.passKey.empty()) modeLine << "k";
	if (modes.userLimit > 0 && modes.userLimit != 1000) modeLine << "l";

	sendMsg(client.getFd(), "324", client.getNickName(), target + " " + modeLine.str());
}

void CommandHandler::applyChannelMode(const Message& msg, Client& client, Channel& channel) {
	std::string modeFlags = msg.params[1];
	bool adding = true;
	ChannelModes modes = channel.getModes();
	size_t paramIndex = 2;

	for (size_t i = 0; i < modeFlags.size(); ++i) {
		char m = modeFlags[i];

		if (m == '+') {
			adding = true;
			continue;
		}
		if (m == '-') {
			adding = false;
			continue;
		}

		switch (m) {
			case 'i':
				modes.inviteOnly = adding;
				break;
			case 't':
				modes.topicProtected = adding;
				break;
			case 'k':
				handleModeKey(msg, client, channel, modes, adding, paramIndex);
				break;
			case 'o':
				handleModeOp(msg, client, channel, adding, paramIndex);
				break;
			case 'l':
				handleModeLimit(msg, client, channel, modes, adding, paramIndex);
				break;
			default:
				sendMsg(client.getFd(), "472", client.getNickName(),
				        std::string(1, m) + " :is unknown mode character");
				break;
		}
	}

	channel.setMode(modes);
	broadcastModeChange(client, channel, msg);
}

void CommandHandler::handleModeKey(const Message& msg, Client& client, Channel& channel, ChannelModes& modes,
	                                   bool adding, size_t& paramIndex) {
	(void)channel;
	if (adding) {
		if (paramIndex >= msg.params.size())
			return sendMsg(client.getFd(), "461", client.getNickName(), "MODE +k :Missing parameter");
		modes.passKey = msg.params[paramIndex++];
	} else {
		modes.passKey.clear();
	}
}

void CommandHandler::handleModeOp(const Message& msg, Client& client, Channel& channel,
                                  bool adding, size_t& paramIndex) {
	if (paramIndex >= msg.params.size())
		return sendMsg(client.getFd(), "461", client.getNickName(), "MODE +o :Missing parameter");

	std::string nick = msg.params[paramIndex++];
	bool found = false;

	const std::map<int, Client*>& members = channel.getMembers();
	for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
		if (it->second->getNickName() == nick) {
			if (adding) channel.addOperator(it->first);
			else channel.removeOperator(it->first);
			found = true;
			break;
		}
	}
	if (!found)
		sendMsg(client.getFd(), "441", client.getNickName(), nick + " " + channel.getName() + " :They aren't on that channel");
}

void CommandHandler::handleModeLimit(const Message& msg, Client& client, Channel& channel,
	                                     ChannelModes& modes, bool adding, size_t& paramIndex) {
	(void)channel;
	if (adding) {
		if (paramIndex >= msg.params.size())
			return sendMsg(client.getFd(), "461", client.getNickName(), "MODE +l :Missing parameter");
		int limit = std::atoi(msg.params[paramIndex++].c_str());
		modes.userLimit = (limit > 0) ? limit : 1000;
	} else {
		modes.userLimit = 1000; // default
	}
}

void CommandHandler::broadcastModeChange(Client& client, Channel& channel, const Message& msg) {
	std::ostringstream modeMsg;
	modeMsg << ":" << client.makePrefix()
	        << " MODE " << msg.params[0] << " " << msg.params[1];

	for (size_t i = 2; i < msg.params.size(); ++i)
		modeMsg << " " << msg.params[i];
	modeMsg << "\r\n";

	channel.broadcast(modeMsg.str());
}
