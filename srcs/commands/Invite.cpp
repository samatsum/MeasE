#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sys/socket.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>

/*
INVITEは存在しないチャンネルへの招待も可能。（401は、ノーサッチニック、オアチャンネルだが。）
チャンネルの、インバイトリストが既存のものしかないから、存在しないチャンネルの設計はしたくない。
（実在するチャンネルのインバイトリストに追加するだけのメソッドだし。）

ただし、存在するチャンネルには、所属メンバーだけが可能。
インバイトオンリーフラグが設定されてるときは、オペレーターのみ可能。
招待の通知は、正体されたものだけが受け取る。

AWAY設定はそもそもその状態の定義がないので不要。

*/

void CommandHandler::handleInvite(const Message& msg, Client& client) {
	if (!client.isAuthenticated()) {
		sendMsg(client.getFd(), "451", client.getNickName(), "You have not registered");
		return;
	}
	if (msg.params.size() < 2) {
		sendMsg(client.getFd(), "461", client.getNickName(), "INVITE :Not enough parameters");
		return;
	}

	std::string targetNick = msg.params[0];
	std::string channelName = msg.params[1];

	Channel* channel = m_server.findChannel(channelName);
	if (!channel) {
		sendMsg(client.getFd(), "403", client.getNickName(), channelName + " :No such channel");
		return;
	}
	if (!channel->hasMember(client.getFd())) {
		sendMsg(client.getFd(), "442", client.getNickName(), channelName + " :You're not on that channel");
		return;
	}

	// +i のときはオペレーターであることを確認
	const ChannelModes& modes = channel->getModes();
	if (modes.inviteOnly && !channel->isOperator(client.getFd())) {
		sendMsg(client.getFd(), "482", client.getNickName(), channelName + " :You're not channel operator");
		return;
	}

	// 招待対象のクライアントを探す
	std::map<int, Client>& clients = m_server.getClients();
	Client* targetClient = NULL;
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
		if (it->second.getNickName() == targetNick) {
			targetClient = &it->second;
			break;
		}
	}
	if (!targetClient) {
		sendMsg(client.getFd(), "401", client.getNickName(), targetNick + " :No such nick");
		return;
	}

	if (targetClient->isInChannel(channelName)) {
		sendMsg(client.getFd(), "443", client.getNickName(), targetNick + " " + channelName + " :is already on channel");
		return;
	}

	channel->addInvitedNick(targetNick);

	std::ostringstream msgStream;
	msgStream << ":" << client.makePrefix()
	          << " INVITE " << targetNick << " :" << channelName << "\r\n";
	send(targetClient->getFd(), msgStream.str().c_str(), msgStream.str().size(), 0);

	sendMsg(client.getFd(), "341", client.getNickName(), targetNick + " " + channelName);
}
