#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <cerrno>

/*
【概要】
コマンドハンドラークラスのうち、プロトコルに従った、解析と送信を行うメソッド群。

sendErrorやsendReplyは内実同じだが、呼び出し時に意図を伝えるために分けている。
 
君たち、コマンドハンドラー直下でもよくない？


登録完了ように、001, 002, 003, 004を返す。RFC2812参照。
001　RPL_WELCOME　クライアントに対する最初の挨拶メッセージ
002　RPL_YOURHOST　サーバのホスト名とバージョン情報
003　RPL_CREATED　サーバが作成された日時情報
004　RPL_MYINFO　サーバ情報（モードや対応機能など）

*/

CommandHandler::Message CommandHandler::parse(const std::string& rawLine) {

	Message msg;
	if (rawLine.empty() || rawLine == "\n" || rawLine == "\r\n") {
		msg.errorDetail = "empty or newline only";
		return msg;
	}

	std::string line = rawLine;
	if (!line.empty() && line[line.size() - 1] == '\n')
		line.erase(line.size() - 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	std::istringstream iss(line);

	if (line[0] == ':') {

		std::string token;
	
		iss >> token;
		if (token.empty()) {
			msg.errorDetail = "prefix symbol found but empty";
			return msg;
		}
		msg.prefix = token.substr(1);
	}

	std::string commandToken;

	iss >> commandToken;
	if (commandToken.empty()) {
		msg.errorDetail = "missing command token";
		return msg;
	}
	for (std::size_t i = 0; i < commandToken.size(); ++i) {
		if (!isalpha(commandToken[i])) {
			msg.errorDetail = "invalid character in command";
			return msg;
		}
	}
	msg.command = commandToken;

	std::string part;
	while (iss >> part) {

		if (part[0] == ':') {
			msg.trailingFlag = true;
			msg.trailing = part.substr(1);
			std::string rest;
			std::getline(iss, rest);
			msg.trailing += rest;
			break;
		}
		msg.params.push_back(part);
	}

	msg.isValid = true;
	return msg;
}

void CommandHandler::sendMsg(int fd, const std::string& prefix,
							const std::string& command,
							const std::vector<std::string>& params,
							const std::string& trailing) {
	std::ostringstream oss;

	oss << ":";
	if (!prefix.empty())
		oss << prefix;
	else
		oss << m_serverName;
	oss << " " << command;

	for (std::size_t i = 0; i < params.size(); ++i)
		oss << " " << params[i];
	if (!trailing.empty())
		oss << " :" << trailing;
	std::string out = oss.str() + "\r\n";
	std::cout  << out << std::endl;
	if (send(fd, out.c_str(), out.size(), 0) == -1) {
		std::cerr << "Send message failed: " << std::strerror(errno) << std::endl;
	}
}

void CommandHandler::sendMsg(int fd, const std::string& code,
							const std::string& target,
							const std::string& text) {
	std::vector<std::string> params;
	if (!text.empty())
		params.push_back(target);
	sendMsg(fd, "", code, params, text);
}

void CommandHandler::sendNumeric(int fd,
	const std::string& code,
	const std::vector<std::string>& params,
	const std::string& trailing)
{
	std::ostringstream oss;
	oss << ":" << m_serverName << " " << code;

	for (size_t i = 0; i < params.size(); ++i)
		oss << " " << params[i];

	if (!trailing.empty())
		oss << " :" << trailing;

	std::string out = oss.str() + "\r\n";
	send(fd, out.c_str(), out.size(), 0);
}

void CommandHandler::tryRegister(Client& client)
{
	std::string serverName = m_server.getServerName();

	if (client.isAuthenticated()
		&& !client.getNickName().empty()
		&& !client.getUserName().empty())
	{
		if (client.isRegistered())
			return;

		client.setRegistered(true);
		std::cout << "[fd " << client.getFd() << "] REGISTER DONE" << std::endl;

		std::vector<std::string> params(1, client.getNickName());

		sendMsg(client.getFd(), "", "001", params,
			"Welcome to the ft_IRC server " + client.getNickName());
		sendMsg(client.getFd(), "", "002", params,
			"Your host is ircserv, running version 0.008");
		sendMsg(client.getFd(), "", "003", params,
			"This server was created just now");
		sendMsg(client.getFd(), "", "004", params,
			serverName + " 0.008 i o");
	}
}

std::string CommandHandler::makeUppercase(const std::string& argStr)
{
	std::string		resultText;
	unsigned char	ch;
	char			upperCh;

	for (std::string::size_type i = 0; i < argStr.length(); i++)
	{
		ch = static_cast<unsigned char>(argStr[i]);
		upperCh = static_cast<char>(std::toupper(ch));
		resultText += upperCh;
	}

	return resultText;
}
