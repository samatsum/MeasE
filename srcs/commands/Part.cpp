#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sys/socket.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cerrno>

/*
/partでirssiから抜けれてない。チャンネル指定されてないとだめ？
→irssiの挙動で、 /window close　が必要、勝手に抜けるわけとちゃうよ


*/

void CommandHandler::handlePart(const Message& msg, Client& client) {

	if (!client.isAuthenticated()) {
		sendMsg(client.getFd(), "451", client.getNickName(), "You have not registered");
		return;
	}

	if (msg.params.empty()) {
		sendMsg(client.getFd(), "461", client.getNickName(), "PART :Not enough parameters");
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

	//トレーリングが空なら、ニックネームで埋める。
	std::string reason;
	if (msg.trailing.empty()) {
		reason = client.getNickName();
	} else {
		reason = msg.trailing;
	}

	std::ostringstream partMsg;
	partMsg << ":" << client.makePrefix()
	        << " PART " << channelName << " :" << reason << "\r\n";

	channel->broadcast(partMsg.str());

	client.leaveChannel(channelName);
	channel->removeMember(client.getFd());

	if (channel->getMemberCount() == 0) {
		m_server.removeChannel(channelName);
	}
}
