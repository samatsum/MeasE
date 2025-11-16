#include "../includes/CommandHandler.hpp"
#include "../includes/IrcServer.hpp"
#include <iostream>//for std::cout, std::cerr
#include <sstream>//for std::istringstream, std::ostringstream



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

bool CommandHandler::requireAuth(Client& client, const std::string& command)
{
	if (client.isRegistered())
		return true;
	sendError(client, "451", command, "You have not registered");
	return false;
}

Channel* CommandHandler::findValidChannel(const std::string& name, Client& client)
{
	Channel* ch = m_server.findChannel(name);
	if (!ch) {
		sendError(client, "403", name, "No such channel");
		return NULL;
	}
	if (!ch->hasMember(client.getFd())) {
		sendError(client, "442", name, "You're not on that channel");
		return NULL;
	}
	return ch;
}

std::string CommandHandler::buildMessage(
	const std::string& prefix,
	const std::string& command,
	const std::vector<std::string>& params,
	const std::string& trailing)
{
	std::ostringstream oss;

	oss << ":";
	if (!prefix.empty())
		oss << prefix;
	else
		oss << m_serverName;
	oss << " " << command;

	for (size_t i = 0; i < params.size(); ++i)
		oss << " " << params[i];
	if (!trailing.empty())
		oss << " :" << trailing;

	return oss.str() + "\r\n";
}

void CommandHandler::sendError(Client& client, const std::string& code,
                               const std::string& context, const std::string& text)
{
	std::vector<std::string> params;
	std::string nick;
	if (client.getNickName().empty())
		nick = "*";
	else
		nick = client.getNickName();

	if (!context.empty())
		params.push_back(nick), params.push_back(context);
	else
		params.push_back(nick);
	sendMsg(client.getFd(), "", code, params, text);
}

void CommandHandler::sendMsg(int fd, const std::string& msg)
{
	Client* c = m_server.findClientByFd(fd);
	if (!c)
		return;

	c->appendSendBuffer(msg);
	m_server.enablePollout(fd);
}

void CommandHandler::sendMsg(int fd,
	const std::string& prefix,
	const std::string& command,
	const std::vector<std::string>& params,
	const std::string& trailing)
{
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

	Client* c = m_server.findClientByFd(fd);
	if (!c)
		return;

	c->appendSendBuffer(out);
	m_server.enablePollout(fd);
}

void CommandHandler::sendChanReply(int fd, const std::string& code,
	const std::string& nick, const std::string& chan, const std::string& text)
{
	std::vector<std::string> params;
	params.push_back(nick);
	params.push_back(chan);
	sendMsg(fd, "", code, params, text);
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

/*
【概要】
コマンドハンドラークラスのうち、プロトコルに従った、解析と送信を行うメソッド群。
登録完了ように、001, 002, 003, 004を返す。RFC2812参照。
001　RPL_WELCOME　クライアントに対する最初の挨拶メッセージ
002　RPL_YOURHOST　サーバのホスト名とバージョン情報
003　RPL_CREATED　サーバが作成された日時情報
004　RPL_MYINFO　サーバ情報（モードや対応機能など）
*/