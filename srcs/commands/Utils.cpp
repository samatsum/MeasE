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

mode +b　はとりま未対応

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

//あちこちで使うので、シンプルな呼び出しに。
bool CommandHandler::requireAuth(Client& client, const std::string& command)
{
	if (client.isAuthenticated())
		return true;
	sendError(client, "451", command, "You have not registered");
	return false;
}

//これもよく使うやつ。
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

/*
コマンド生成メソッド、プレフィックスをサーバーとクライアントで共通できる。
prefixを指定すれば、　nickname!username@host（呼び出しで作る。）
指定しなければ、サーバーネームをprefixにする。

つまり
:nickname!username@host COMMAND param1 param2 :trailing message\r\n
たとえば、JOIN（コマンド通知）ならば
:nickname!username@host JOIN #channel\r\n

もしくは
:hostname COMMAND param1 param2 :trailing message\r\n
たとえば、サーバー返答（ニューメリック）ならば
:nickname!username@host 001 nickname :Welcome to the ft_IRC server nickname\r\n

のように生成する。
*/
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




/*
エラー通知は、ユーザー名、コードのニューメリックタイプ
ただ、ユーザー名取得前は、*を使う。
:server <number> <nick> <あればcontext>　:message
の形式。
サーバー名は、空で用いて、sendMsg内で補完する。
*/
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

/*
組み立て済みの場合
*/
void CommandHandler::sendMsg(int fd, const std::string& msg)
{
	std::cout << "[send]" << msg << std::endl;
	if (send(fd, msg.c_str(), msg.size(), 0) == -1)
		std::cerr << "Send failed: " << std::strerror(errno) << std::endl;
}

/*
コマンド通知、ニューメリック返信にも使えるバージョン
ニューメリックなら、prefixを空にしておけば、サーバー名が
コマンド通知ならば、クライアントのプレフィックスでクライアントに指定する。
*/
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
	std::cout  << "[send]" << out << std::endl;
	if (send(fd, out.c_str(), out.size(), 0) == -1) {
		std::cerr << "Send message failed: " << std::strerror(errno) << std::endl;
	}
}

/*
サーバー返答（ニューメリック）のうち、チャンネル関連のニューメリック返信用。
:server numeric　nick #channel :<trailing> の形式にするための。
以前だと、チャンネルがトレーリングにされてしまったので
paramにnick, chanを入れる用。
呼び出し元でベクター作る面倒を省くためのヘルパー。
*/
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
// void CommandHandler::sendMsg(int fd, const std::string& code,
// 							const std::string& target,
// 							const std::string& text) {
// 	std::vector<std::string> params;
// 	if (!text.empty())
// 		params.push_back(target);
// 	sendMsg(fd, "", code, params, text);
// }
*/