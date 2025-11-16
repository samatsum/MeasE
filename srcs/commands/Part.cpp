#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"

void CommandHandler::handlePart(const Message& msg, Client& client)
{
	if (!requireAuth(client, "PART"))
		return;
	if (msg.params.empty()) {
		sendError(client, "461", "PART", "Not enough parameters");
		return;
	}

	std::string channelName = msg.params[0];
	Channel* channel = m_server.findChannel(channelName);
	if (!channel) {
		sendError(client, "403", channelName, "No such channel");
		return;
	}
	if (!channel->hasMember(client.getFd())) {
		sendError(client, "442", channelName, "You're not on that channel");
		return;
	}

	std::string reason;
	if (msg.trailing.empty())
		reason = client.getNickName();
	else
		reason = msg.trailing;

	std::vector<std::string> params;
	params.push_back(channelName);
	std::string partMsg = buildMessage(client.makePrefix(), "PART", params, reason);

	channel->broadcast(partMsg);

	channel->removeOperator(client.getFd());
	client.leaveChannel(channelName);
	channel->removeMember(client.getFd());

	if (channel->getMemberCount() == 0)
		m_server.removeChannel(channelName);

	std::cout << "[fd " << client.getFd() << "] PART " << channelName
	          << " (" << reason << ")" << std::endl;
}
