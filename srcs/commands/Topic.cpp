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
// const char* ERR_NOCHANMODES = "477";モードをサポートしないチャンネルとは？
// const char* ERR_CHANOPRIVSNEEDED = "482";
// const char* RPL_NOTOPIC = "331";
// const char* RPL_TOPIC = "332";

/*
TOPIC channel [:topic]
もし、トピックが省略されたら、現在のトピックを返す。
もし、トピックが指定されたら、そのチャンネルのトピックを指定する（許可次第）
トピックが空文字列（:）だけなら、トピックを削除する（2814）

(+t)　オプションは、トピック変更をオペレーターのみに限定する。（これは以前samatsum氏に対して間違って説明してましたね。）
エラーは、ユーザーのチャンネル不参加、パラ不足、モードをサポートしてないチャンネル

2814では、トレーリングを空文字にすることで、トピックの削除をできる。（1459では削除の問題はない）
ただしirssiでは
/topic #gen :hogeでは、topicが、:hogeにされる。
/topic #gen hoge だと、irssi側で、topic #gen :hogeに変換さされる。
なので、空文字で送ると、/topic #genになるので、削除ができない。(スペースを入れるとスペースが入ってるかも)
優先順位が低いのと、ncでは、この仕様が実現できてるので、今回はこのままにする。

同じトピックを再設定しようとしたときに、ブロードキャストし直すのもめんどいから
もしトピックとトレーリングが同じならreturnを入れてもいいかも。
*/

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

		std::ostringstream whoTime;
		whoTime << channelName << " " << channel->getTopicSetBy()
		        << " " << channel->getTopicSetTime();

		std::vector<std::string> params;
		params.push_back(client.getNickName());
		sendMsg(client.getFd(), buildMessage("", "333", params, whoTime.str()));
		return;
	}

	if (modes.topicProtected && !channel->isOperator(client.getFd())) {
		sendChanReply(client.getFd(), "482", client.getNickName(), channelName, "You're not channel operator");
		return;
	}

	//同じトピックなら何もしない
	std::string newTopic;
	if (!msg.trailingFlag)
		newTopic = "";
	else
		newTopic = msg.trailing;
	if (newTopic == channel->getTopic())
		return;

	channel->setTopic(newTopic, client.getNickName());

	// --- ブロードキャスト ---
	std::ostringstream topicMsg;
	topicMsg << ":" << client.makePrefix()
	         << " TOPIC " << channelName << " :" << newTopic << "\r\n";
	channel->broadcast(topicMsg.str());
}

// void CommandHandler::handleTopic(const Message& msg, Client& client) {

// 	if (!client.isAuthenticated()) {
// 		sendMsg(client.getFd(), "451", client.getNickName(), "You have not registered");
// 		return;
// 	}
// 	if (msg.params.empty()) {
// 		sendMsg(client.getFd(), "461", client.getNickName(), "TOPIC :Not enough parameters");
// 		return;
// 	}

// 	std::string channelName = msg.params[0];
// 	Channel* channel = m_server.findChannel(channelName);
// 	if (!channel) {
// 		sendMsg(client.getFd(), "403", client.getNickName(), channelName + " :No such channel");
// 		return;
// 	}
// 	if (!channel->hasMember(client.getFd())) {
// 		sendMsg(client.getFd(), "442", client.getNickName(), channelName + " :You're not on that channel");
// 		return;
// 	}

// /*



// */

// 	//トピック何？への回答
// 	const ChannelModes& modes = channel->getModes();
// 	if (msg.params.size() == 1 && msg.trailingFlag == false) {
// 		if (channel->getTopic().empty()) {
// 			sendMsg(client.getFd(), "331", client.getNickName(), channelName + " :No topic is set");
// 		} else {
// 			sendMsg(client.getFd(), "332", client.getNickName(), channelName + " :" + channel->getTopic());
// 		}

// 		std::ostringstream whoTimeMsg;
// 		whoTimeMsg << channelName << " " << channel->getTopicSetBy() << " " << channel->getTopicSetTime();
// 		sendMsg(client.getFd(), "333", client.getNickName(), whoTimeMsg.str());
// 		return;
// 	}

// 	if (modes.topicProtected && !channel->isOperator(client.getFd())) {
// 		sendMsg(client.getFd(), "482", client.getNickName(), channelName + " :You're not channel operator");
// 		return;
// 	}

// 	std::string newTopic;
// 	if (msg.trailing.empty()) {
// 		newTopic = "";
// 	} else {
// 		newTopic = msg.trailing;
// 	}
// 	channel->setTopic(newTopic, client.getNickName());
// 	std::ostringstream topicMsg;
// 	topicMsg << ":" << client.makePrefix()
// 	         << " TOPIC " << channelName << " :" << newTopic << "\r\n";
// 	channel->broadcast(topicMsg.str());
// 	return;

// }
