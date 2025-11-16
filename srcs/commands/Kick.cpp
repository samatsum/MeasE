#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"

void CommandHandler::handleKick(const Message& msg, Client& client)
{
	if (!requireAuth(client, "KICK"))
		return;
	if (msg.params.size() < 2) {
		sendError(client, "461", "KICK", "Not enough parameters");
		return;
	}

	const std::string& channelName = msg.params[0];
	const std::string& targetNick  = msg.params[1];
	std::string reason;

	if (msg.trailingFlag && !msg.trailing.empty())
		reason = msg.trailing;
	else
		reason = targetNick;

	Channel* ch = m_server.findChannel(channelName);
	if (!ch) {
		sendError(client, "403", channelName, "No such channel");
		return;
	}
	if (!ch->hasMember(client.getFd())) {
		sendError(client, "442", channelName, "You're not on that channel");
		return;
	}
	if (!ch->isOperator(client.getFd())) {
		sendError(client, "NOTICE", targetNick, "You are not channel operator (+" + channelName + " requires op)");	
		return;
	}

	Client* target = m_server.findClientByNick(targetNick);
	if (!target || !ch->hasMember(target->getFd())) {
		sendError(client, "441", channelName, "They aren't on that channel");
		return;
	}

	std::vector<std::string> params;
	params.push_back(channelName);
	params.push_back(targetNick);

	std::string prefix = client.getNickName() + "!" +
						client.getUserName() + "@" +
						client.getHost();

	std::string out = buildMessage(prefix, "KICK", params, reason);

	ch->broadcast(out);
	ch->removeOperator(target->getFd());
	ch->removeMember(target->getFd());
	target->leaveChannel(channelName);
}
