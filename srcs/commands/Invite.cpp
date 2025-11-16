#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"

void CommandHandler::handleInvite(const Message& msg, Client& client)
{
	if (!requireAuth(client, "INVITE"))
		return;
	if (msg.params.size() < 2) {
		sendError(client, "461", "INVITE", "Not enough parameters");
		return;
	}

	const std::string& targetNick  = msg.params[0];
	const std::string& channelName = msg.params[1];

	Channel* ch = m_server.findChannel(channelName);
	if (!ch) {
		sendError(client, "403", channelName, "No such channel");
		return;
	}
	if (!ch->hasMember(client.getFd())) {
		sendError(client, "442", channelName, "You're not on that channel");
		return;
	}

	const ChannelModes& modes = ch->getModes();
	if (modes.inviteOnly && !ch->isOperator(client.getFd())) {
		sendError(client, "NOTICE", targetNick, "You are not channel operator (+" + channelName + " requires op)");	
		return;
	}

	Client* target = m_server.findClientByNick(targetNick);
	if (!target) {
		sendError(client, "401", targetNick, "No such nick");
		return;
	}

	if (target->isInChannel(channelName)) {

		std::vector<std::string> p;
		p.push_back(client.getNickName());
		p.push_back(targetNick);
		p.push_back(channelName);
		sendMsg(client.getFd(), "", "443", p, "is already on channel");
		return;
	}

	ch->addInvitedNick(targetNick);

	//invite notification
	{
		std::vector<std::string> p;
		p.push_back(targetNick);

		std::string prefix = client.makePrefix();
		std::string msgOut = buildMessage(prefix, "INVITE", p, channelName);
		sendMsg(target->getFd(), msgOut);
	}

	// 341 RPL_INVITING <nick> <channel>
	{
		std::vector<std::string> p;
		p.push_back(client.getNickName());
		p.push_back(targetNick);
		p.push_back(channelName);
		sendMsg(client.getFd(), "", "341", p, "");
	}
}
