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

チャンネルの場所がおかしい？
*/

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
		// sendError(client, "482", channelName, "You're not channel operator");
		return;
	}

	Client* target = m_server.findClientByNick(targetNick);
	if (!target) {
		sendError(client, "401", targetNick, "No such nick");
		return;
	}

	if (target->isInChannel(channelName)) {
		// ERR_USERONCHANNEL 443 <nick> <chan> :is already on channel
		std::vector<std::string> p;
		p.push_back(client.getNickName());
		p.push_back(targetNick);
		p.push_back(channelName);
		sendMsg(client.getFd(), "", "443", p, "is already on channel");
		return;
	}

	ch->addInvitedNick(targetNick);
	//招待通知
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
