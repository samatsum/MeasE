#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sstream>

static bool checkValidChannelName(const std::string &name);

void CommandHandler::handleJoin(const Message& msg, Client& client)
{
	if (!requireAuth(client, "JOIN"))
		return;
	if (msg.params.empty()) {
		sendError(client, "461", "JOIN", "Not enough parameters");
		return;
	}

	std::string channelName = msg.params[0];
	if (!checkValidChannelName(channelName)) {
		sendError(client, "476", channelName, "Invalid channel name");
		return;
	}

	Channel* channelPtr = m_server.findChannel(channelName);
	if (!channelPtr) {
		channelPtr = &m_server.getCreateChannel(channelName);
	}
	else {
		//+i（invite-only）
		const ChannelModes& modes = channelPtr->getModes();
		if (modes.inviteOnly && !channelPtr->isInvited(client.getNickName())) {
			sendError(client, "473", channelName, "Cannot join channel (+i)");
			return;
		}
		channelPtr->removeInvitedNick(client.getNickName());
	}

	Channel& channel = *channelPtr;
	const ChannelModes& modes = channel.getModes();
	if (static_cast<int>(channel.getMemberCount()) >= modes.userLimit) {
		sendError(client, "471", channelName, "Channel is full");
		return;
	}
	if (channel.hasMember(client.getFd())) {
		sendError(client, "443", channelName, "is already on channel");
		return;
	}

	if (!modes.passKey.empty()) {
		std::string suppliedKey;
		if (msg.params.size() >= 2)
			suppliedKey = msg.params[1];
		else if (!msg.trailing.empty())
			suppliedKey = msg.trailing;

		if (suppliedKey != modes.passKey) {
			sendError(client, "475", channelName, "Cannot join channel (+k)");
			return;
		}
	}

	channel.addMember(&client);
	client.joinChannel(channelName);

	if (channel.getMemberCount() == 1)
		channel.addOperator(client.getFd());

	//JOIN notice
	{
		std::vector<std::string> params;
		params.push_back(channelName);

		std::string prefix = client.makePrefix(); // nick!user@host
		std::string out = buildMessage(prefix, "JOIN", params, "");
		channel.broadcast(out, -1);
	}

	//RPL_TOPIC
	{
		std::vector<std::string> p;
		p.push_back(client.getNickName());
		p.push_back(channelName);
		if (channel.getTopic().empty())
			sendMsg(client.getFd(), "", "331", p, "No topic is set");
		else
			sendMsg(client.getFd(), "", "332", p, channel.getTopic());
	}

	// RPL_NAMREPLY
	{
		std::ostringstream oss;
		const std::map<int, Client*>& members = channel.getMembers();
		for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
			if (it != members.begin())
				oss << " ";
			if (channel.isOperator(it->first))
				oss << "@";
			oss << it->second->getNickName();
		}

		std::vector<std::string> params353;
		params353.push_back(client.getNickName());
		params353.push_back("=");
		params353.push_back(channelName);
		sendMsg(client.getFd(), "", "353", params353, oss.str());
	}

	// RPL_ENDOFNAMES
	{
		std::vector<std::string> params366;
		params366.push_back(client.getNickName());
		params366.push_back(channelName);
		sendMsg(client.getFd(), "", "366", params366, "End of /NAMES list");
	}
}

static bool checkValidChannelName(const std::string &name) {

	if (name.empty() || name.size() > 50)
		return false;

	if (name[0] != '#')
		return false;

	for (std::size_t i = 1; i < name.size(); ++i) {
		unsigned char c = name[i];

		if (c == ',' || c <= ' ' || c == 127)
			return false;

		if (!std::isalnum(c) &&
			c != '-' && c != '_' && c != '[' && c != ']' &&
			c != '\\' && c != '`' && c != '^' &&
			c != '{' && c != '}' && c != '|')
			return false;
	}
	return true;
}

/*
rfc2812
443 ERR_USERONCHANNEL
451	ERR_NOTREGISTERED
471 ERR_CHANNELISFULL
476 ERR_BADCHANMASK
461 ERR_NEEDMOREPARAMS

332　RPL_TOPIC　"<channel> :<topic>"

353　RPL_NAMREPLY
366　RPL_ENDOFNAMES　"<channel> :End of /NAMES list"

*/