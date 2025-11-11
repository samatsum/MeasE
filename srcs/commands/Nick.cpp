#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include <iostream>
#include <sstream>
#include <cctype>

/*
NICK（1459）
431 ERR_NONICKNAMEGIVEN ニックネームが指定されていない
432 ERR_ERRONEUSNICKNAME 不正なニックネーム
433 ERR_NICKNAMEINUSE ニックネームがすでに使用されている（違い？）
436 ERR_NICKCOLLISION ニックネームの衝突（違い？）
437 ERR_UNAVAILRESOURCE 利用できないリソース（エラー？）
484 ERR_RESTRICTED コネクションが制限されている？（エラー）

ニックネームの置換　name1 NICK name2　はやらない。
ニックネームの衝突時、古いほうのインスタンスが削除？

ニックネームをチャンネル内で変更したら面倒そう、
インバイトリスト、オペーレータリストの2つ帰る必要がありそう。

//ニック

*/


void CommandHandler::handleNick(const Message& msg, Client& client)
{
	if (msg.params.empty())
	{
		sendMsg(client.getFd(), "431", client.getNickName(), ":No nickname given");
		return;
	}

	std::string newNick = msg.params[0];

	if (!isValidNick(newNick))
	{
		sendMsg(client.getFd(), "432", client.getNickName(), newNick + " :Erroneous nickname");
		return;
	}

	if (m_server.isNickInUse(newNick))
	{
		// irssi互換のレスポンス形式（RFC 1459）: 433 <target> <nick> :Nickname is already in use
		std::vector<std::string> params;
		std::string currentNick;
		if (client.getNickName().empty())
			currentNick = "*";
		else
			currentNick = client.getNickName();
		params.push_back(currentNick);
		params.push_back(newNick);
		sendMsg(client.getFd(), "", "433", params, "Nickname is already in use");
		return;
	}

	if (!client.isRegistered())
	{
		registerNick(client, newNick);
	}
	else
	{
		changeNick(client, newNick);
	}
}

void CommandHandler::registerNick(Client& client, const std::string& newNick)
{
	client.setNickName(newNick);
	tryRegister(client);
}

void CommandHandler::changeNick(Client& client, const std::string& newNick)
{
	std::string oldNick = client.getNickName();
	client.setNickName(newNick);

	// :oldNick!user@host NICK :newNick
	std::ostringstream noticeMsg;
	noticeMsg << ":" << oldNick << "!" << client.getUserName()
	          << "@" << client.getHost()
	          << " NICK :" << newNick << "\r\n";

	m_server.broadcastAll(noticeMsg.str());

	std::cout << "[fd " << client.getFd() << "] NICK changed from "
	          << oldNick << " to " << newNick << std::endl;
}

bool CommandHandler::isValidNick(const std::string& nick)
{
	if (nick.empty() || nick.size() > 9)
		return false;

	// 先頭文字：A–Z, a–z, および特定記号
	char first = nick[0];
	if (!std::isalpha(first) &&
	    first != '[' && first != ']' &&
	    first != '\\' && first != '^' &&
	    first != '{' && first != '}' &&
	    first != '_')
	{
		return false;
	}

	for (std::size_t i = 1; i < nick.size(); ++i)
	{
		char c = nick[i];
		if (!std::isalnum(c) &&
		    c != '-' && c != '[' && c != ']' &&
		    c != '\\' && c != '^' && c != '{' &&
		    c != '}' && c != '_')
		{
			return false;
		}
	}
	return true;
}
