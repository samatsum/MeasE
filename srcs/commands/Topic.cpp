#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sstream>

void CommandHandler::handleTopic(const Message& msg, Client& client)
{
	if (!requireAuth(client, "TOPIC"))
		return;
	if (msg.params.empty()) {
		sendError(client, "461", "TOPIC", "Not enough parameters");
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

	const ChannelModes& modes = channel->getModes();

	if (msg.params.size() == 1 && msg.trailingFlag == false) {
		if (channel->getTopic().empty())
			sendChanReply(client.getFd(), "331", client.getNickName(), channelName, "No topic is set");
		else
			sendChanReply(client.getFd(), "332", client.getNickName(), channelName, channel->getTopic());

		if (channel->getTopicSetTime() != 0) {
			std::ostringstream whoTime;
			whoTime << channelName << " "
			        << channel->getTopicSetBy() << " "
			        << channel->getTopicSetTime();

			std::vector<std::string> params;
			params.push_back(client.getNickName());
			sendMsg(client.getFd(), buildMessage("", "333", params, whoTime.str()));
		}
		return;
	}

	if (modes.topicProtected && !channel->isOperator(client.getFd())) {
		sendError(client, "NOTICE", channelName, "You are not channel operator (+t requires op)");	
		return;
	}

	std::string newTopic;
	if (!msg.trailingFlag)
		newTopic = "";
	else
		newTopic = msg.trailing;
	if (newTopic == channel->getTopic())
		return;

	channel->setTopic(newTopic, client.getNickName());

	std::ostringstream topicMsg;
	topicMsg << ":" << client.makePrefix()
	         << " TOPIC " << channelName << " :" << newTopic << "\r\n";
	channel->broadcast(topicMsg.str());
}

// const char* ERR_NOTREGISTERED = "451";
// const char* ERR_NEEDMOREPARAMS = "461";
// const char* ERR_NOSUCHCHANNEL = "403";
// const char* ERR_NOTONCHANNEL = "442";
// const char* ERR_NOCHANMODES = "477";モードをサポートしないチャンネルとは？
// const char* ERR_CHANOPRIVSNEEDED = "482";
// const char* RPL_NOTOPIC = "331";
// const char* RPL_TOPIC = "332";
