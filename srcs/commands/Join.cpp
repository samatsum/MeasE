#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sys/socket.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cerrno>

/*
チャンネル名の妥当性ルール
rfc2812
443 ERR_USERONCHANNEL
451	ERR_NOTREGISTERED
471 ERR_CHANNELISFULL
476 ERR_BADCHANMASK
461 ERR_NEEDMOREPARAMS

332　RPL_TOPIC　"<channel> :<topic>"

353　RPL_NAMREPLY
366　RPL_ENDOFNAMES　"<channel> :End of /NAMES list"

インバイトモード、ユーザーリミットモードの実装が必要。

//チャンネル名は大文字、小文字を区別しない。
チャンネル作成者が、オペレーターになるが、そいつが抜けた場合はどうする？(RFC1459)

インバイトモードでも、招待されてないユーザーがすでにインバイトモードに指定されたチャンネルを
開こうとするとチャンネル画面は開けるが、メッセージの送受信はできない。

/joinの中でジョインすると、また別のチャンネルに参加できるねぇ。

*/

static bool checkValidChannelName(const std::string &name);

void CommandHandler::handleJoin(const Message& msg, Client& client) {

	//認証前のクライアントは拒否
	if (!client.isAuthenticated()) {
		//一番最初のJOINが入る　別に悪さはしない。
		sendMsg(client.getFd(), "451", client.getNickName(), "You have not registered");
		return;
	}

	if (msg.params.empty()) {
		sendMsg(client.getFd(), "461", client.getNickName(), "JOIN :Not enough parameters");
		return;
	}
	std::string channelName = msg.params[0];
	if (!checkValidChannelName(channelName)) {
		sendMsg(client.getFd(), "476", client.getNickName(), "Invalid channel name");
		return;
	}

	//チャンネルを取得または生成 チャンネル名の生成ルールが正しいならば、ここで取得しとけばよいはず。
	Channel* channelPtr = m_server.findChannel(channelName);
	if (channelPtr == NULL) {
		// チャンネルが存在しない場合のみ新規作成
		channelPtr = &m_server.getCreateChannel(channelName);
	} else {
		// 既存チャンネル → invite-only の場合チェック
		const ChannelModes& modes = channelPtr->getModes();
		if (modes.inviteOnly && !channelPtr->isInvited(client.getNickName())) {
			sendMsg(client.getFd(), "473", client.getNickName(), channelName + " :Cannot join channel (+i)");
			return;
		}
		channelPtr->removeInvitedNick(client.getNickName());
	}

	Channel& channel = *channelPtr;

	//人数制限の確認、立ち上げるときに、オプションはつけるんか？
	const ChannelModes& modes = channel.getModes();
	if (static_cast<int>(channel.getMemberCount()) >= modes.userLimit) {
		sendMsg(client.getFd(), "471", client.getNickName(), channelName + " :Channel is full");
		return;
	}
	if (channel.hasMember(client.getFd())) {
		sendMsg(client.getFd(), "443", client.getNickName(), channelName + " :is already on channel");
		return;
	}

	// +k のときはキー引数が必須。params.size() や trailing を確認してから比較する。
	if (!modes.passKey.empty()) {
		std::string suppliedKey;
		if (msg.params.size() >= 2)
			suppliedKey = msg.params[1];
		else if (!msg.trailing.empty())
			suppliedKey = msg.trailing;
		if (suppliedKey != modes.passKey) {
			sendMsg(client.getFd(), "475", client.getNickName(), channelName + " :Cannot join channel (+k)");
			return;
		}
	}

	//クライアントとチャンネルでメンバーに追加
	channel.addMember(&client);
	client.joinChannel(channelName);

	// 最初の参加者をオペレーターに設定（とりま）
	if (channel.getMemberCount() == 1)
		channel.addOperator(client.getFd());

	//チャンネル参加通知（全員へ）
	std::string prefix = client.makePrefix();
	std::string joinLine = ":" + prefix + " JOIN " + channelName;
	std::string joinMsg  = joinLine + "\r\n";
	channel.broadcast(joinMsg, client.getFd());
	if (send(client.getFd(), joinMsg.c_str(), joinMsg.size(), 0) == -1)
		std::cerr << "Send JOIN reply failed: " << std::strerror(errno) << std::endl;


	//トピック表示
	if (!channel.getTopic().empty()) {

		std::vector<std::string> params332;

		params332.push_back(client.getNickName());
		params332.push_back(channelName);
		sendMsg(client.getFd(), "", "332", params332, channel.getTopic());
	}


	//NAMESリスト送信
	std::ostringstream names;
	const std::map<int, Client*>& members = channel.getMembers();
	for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
		if (it != members.begin())
			names << " ";
		names << it->second->getNickName();
	}
	std::vector<std::string> params353;
	params353.push_back(client.getNickName());
	params353.push_back("=");
	params353.push_back(channelName);
	sendMsg(client.getFd(), "", "353", params353, names.str());

	std::vector<std::string> params366;
	params366.push_back(client.getNickName());
	params366.push_back(channelName);
	sendMsg(client.getFd(), "", "366", params366, "End of /NAMES list");

}

//&は、ローカルなので、使わん　紛らわしいから弾いても良いか
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
