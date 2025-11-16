#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include "../../includes/Utils.hpp"
#include <sstream>
#include <cerrno>
#include <cstdlib>

void CommandHandler::handleMode(const Message& msg, Client& client) {
	if (!requireAuth(client, "MODE"))
		return;

	if (msg.params.empty()) {
		sendError(client, "461", "MODE", "Not enough parameters");
		return;
	}

	if (msg.params[0][0] != '#') {
		handleUserMode(msg, client);
		return;
	}

	handleChannelMode(msg, client);
}

void CommandHandler::handleUserMode(const Message& msg, Client& client) {
	(void)msg;
	sendError(client, "501", "MODE", "User modes are not supported on this server");
}

void CommandHandler::handleChannelMode(const Message& msg, Client& client) {
	const std::string& target = msg.params[0];
	Channel* channel = m_server.findChannel(target);

	if (!channel) {
		sendError(client, "403", target, "No such channel");
		return;
	}
	if (!channel->hasMember(client.getFd())) {
		sendError(client, "442", target, "You're not on that channel");
		return;
	}

	if (msg.params.size() == 1 || (msg.params.size() >= 2 && msg.params[1].empty())) {
		replyChannelModes(client, *channel, target);
		return;
	}

	if (msg.params[1].find('b') != std::string::npos) {
		handleBan(client, *channel, target);

		bool onlyB = true;
		for (size_t i = 0; i < msg.params[1].size(); ++i) {
			char c = msg.params[1][i];
			if (c != '+' && c != '-' && c != 'b') {
				onlyB = false;
				break;
			}
		}
		if (onlyB) {
			return;
		}
	}

	std::string cleaned;
	for (size_t i = 0; i < msg.params[1].size(); ++i) {
		if (msg.params[1][i] != 'b') {
			cleaned += msg.params[1][i];
		}
	}
	
	Message newMsg = msg;
	newMsg.params[1] = cleaned;

	if (!channel->isOperator(client.getFd())) {

		sendError(client, "NOTICE", target, "You are not channel operator (+" + cleaned + " requires op)");		
		return;
	}

	applyChannelMode(newMsg, client, *channel);
}

void CommandHandler::replyChannelModes(Client& client, Channel& channel, const std::string& target) {
	const ChannelModes& modes = channel.getModes();
	std::ostringstream modeLine;

	modeLine << "+";
	if (modes.inviteOnly) modeLine << "i";
	if (modes.topicProtected) modeLine << "t";
	if (!modes.passKey.empty()) modeLine << "k";
	if (modes.userLimit > 0 && modes.userLimit != 1000) modeLine << "l";

	sendChanReply(client.getFd(), "324", client.getNickName(), target, modeLine.str());
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
				sendError(client, "472", std::string(1, m), "is unknown mode character");
				break;
		}
	}

	channel.setMode(modes);
	broadcastModeChange(client, channel, msg);
}

void CommandHandler::handleModeKey(const Message& msg, Client& client, Channel& channel,
                                   ChannelModes& modes, bool adding, size_t& paramIndex) {
	(void)channel;
	if (adding) {
		if (paramIndex >= msg.params.size()) {
			sendError(client, "461", "MODE", "Missing parameter for +k");
			return;
		}
		modes.passKey = msg.params[paramIndex++];
		if (!isAcceptablePassword(modes.passKey)) {
			sendError(client, "467", "MODE", "Invalid channel key");
			modes.passKey.clear();
			return;
		}
	} else {
		modes.passKey.clear();
	}
}

void CommandHandler::handleModeOp(const Message& msg, Client& client, Channel& channel,
                                  bool adding, size_t& paramIndex) {
	if (paramIndex >= msg.params.size()) {
		sendError(client, "461", "MODE", "Missing parameter for +o");
		return;
	}

	std::string nick = msg.params[paramIndex++];
	bool found = false;
	const std::map<int, Client*>& members = channel.getMembers();

	for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
		if (it->second->getNickName() == nick) {
			if (adding)
				channel.addOperator(it->first);
			else
				channel.removeOperator(it->first);
			found = true;
			break;
		}
	}

	if (!found)
		sendError(client, "441", nick, "They aren't on that channel");
}

void CommandHandler::handleModeLimit(const Message& msg, Client& client,
                                     Channel& channel, ChannelModes& modes,
                                     bool adding, size_t& paramIndex)
{
	(void)modes;
	(void)channel;

	if (adding) {
		if (paramIndex >= msg.params.size()) {
			sendError(client, "461", "MODE", "Missing parameter for +l");
			return;
		}

		char* endptr;
		errno = 0;
		int limit = std::strtol(msg.params[paramIndex++].c_str(), &endptr, 10);
		if (errno != 0 || *endptr != '\0' || limit <= 0 || limit > 1000) {
			sendError(client, "461", "MODE", "Invalid limit parameter");
			return;
		}

		modes.userLimit = limit;
	} else {
		modes.userLimit = 1000;
	}
}

void CommandHandler::handleBan(Client& client, Channel& channel, const std::string& chanName) {

	(void)channel;
    sendChanReply(client.getFd(), "367", client.getNickName(), chanName, "*!*@*");
    sendChanReply(client.getFd(), "368", client.getNickName(), chanName, "End of channel ban list");
}

void CommandHandler::broadcastModeChange(Client& client, Channel& channel, const Message& msg) {
	std::string prefix = client.makePrefix();
	std::string out = buildMessage(prefix, "MODE", msg.params, "");
	channel.broadcast(out);
}
