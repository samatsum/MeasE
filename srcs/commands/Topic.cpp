#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sys/socket.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cerrno>

// const char* ERR_NOTREGISTERED = "451";
// const char* ERR_NEEDMOREPARAMS = "461";
// const char* ERR_NOSUCHCHANNEL = "403";
// const char* ERR_NOTONCHANNEL = "442";
// const char* ERR_CHANOPRIVSNEEDED = "482";
// const char* RPL_NOTOPIC = "331";
// const char* RPL_TOPIC = "332";

void CommandHandler::handleTopic(const Message& msg, Client& client) {

	if (!client.isAuthenticated()) {
		sendMsg(client.getFd(), "451", client.getNickName(), "You have not registered");
		return;
	}
	if (msg.params.empty()) {
		sendMsg(client.getFd(), "461", client.getNickName(), "TOPIC :Not enough parameters");
		return;
	}

	std::string channelName = msg.params[0];
	Channel* channel = m_server.findChannel(channelName);
	if (!channel) {
		sendMsg(client.getFd(), "403", client.getNickName(), channelName + " :No such channel");
		return;
	}
	if (!channel->hasMember(client.getFd())) {
		sendMsg(client.getFd(), "442", client.getNickName(), channelName + " :You're not on that channel");
		return;
	}

	//トピックがちゃんと引数としてあるか。
	//TOPIC　#channel :new topic なので、トレーリングがないとチャンネル名だけになる、パラムスが複数なら2つ目以降は無視。
	const ChannelModes& modes = channel->getModes();
	if (msg.params.size() == 1 && msg.trailing.empty()) {
		if (channel->getTopic().empty()) {
			sendMsg(client.getFd(), "331", client.getNickName(), channelName + " :No topic is set");
		} else {
			sendMsg(client.getFd(), "332", client.getNickName(), channelName + " :" + channel->getTopic());
		}
		return;
	}

	if (modes.topicProtected && !channel->isOperator(client.getFd())) {
		sendMsg(client.getFd(), "482", client.getNickName(), channelName + " :You're not channel operator");
		return;
	}

	//すでに同じトピックが設定されてるときは？
	std::string newTopic = msg.trailing;

	channel->setTopic(newTopic);

	std::ostringstream topicMsg;
	topicMsg << ":" << client.makePrefix()
	         << " TOPIC " << channelName
	         << " :" << newTopic << "\r\n";

	channel->broadcast(topicMsg.str());
}
