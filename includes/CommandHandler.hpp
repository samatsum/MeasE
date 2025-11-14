#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include <string>
#include <vector>
#include "Client.hpp"
#include "Channel.hpp"
#include <map>

class IrcServer;

class CommandHandler {

public:
	CommandHandler(IrcServer& server);
	~CommandHandler();

	void	parseCommand(const std::string& line, Client& client);

private:
	IrcServer&	m_server;
	std::string	m_serverName;//サーバーが持ってるから、メソッドで取るべきかも。

	struct Message {

		std::string prefix;
		std::string command;
		std::vector<std::string> params;
		std::string trailing;
		bool		trailingFlag;//:が存在したか。：だけの空呼び出しに対応。
	
		bool isValid;
		std::string errorDetail;
		Message() : trailingFlag(false), isValid(false) {}
	};

	typedef void (CommandHandler::*CommandFunc)(const struct Message& msg, Client& client);
	std::map<std::string, CommandFunc>	m_cmdMap;


	Message parse(const std::string& rawLine);
	std::string buildMessage(
		const std::string& prefix,
		const std::string& command,
		const std::vector<std::string>& params,
		const std::string& trailing);
	void	sendMsg(int fd, const std::string& prefix,
						const std::string& command,
						const std::vector<std::string>& params,
						const std::string& trailing);
	void sendMsg(int fd, const std::string& msg);
	void sendChanReply(int fd, const std::string& code,
					const std::string& nick, const std::string& chan, const std::string& text);
	void sendError(Client& client, const std::string& code,
                               const std::string& context, const std::string& text);

	bool requireAuth(Client& client, const std::string& command);
	Channel* findValidChannel(const std::string& name, Client& client);

	// ====== 通信系 ======
	void	handleCAP(const Message& msg, Client& client);
	void	handlePing(const Message& msg, Client& client);
 
	// ====== 登録系 ======
	void	handlePass(const Message& msg, Client& client);
	void	handleNick(const Message& msg, Client& client);	
	bool	isValidNick(const std::string& nick);
	void	registerNick(Client& client, const std::string& newNick);
	void	changeNick(Client& client, const std::string& newNick);

	void	handleUser(const Message& msg, Client& client);
	void	handleQuit(const Message& msg, Client& client);

	void	tryRegister(Client& client);

	void	handleWho(const Message& msg, Client& client);
	void	handleWhois(const Message& msg, Client& client);
	void	handleNames(const Message& msg, Client& client);

	// ====== チャンネル系 ======
	void	handleJoin(const Message& msg, Client& client);
	void	handlePart(const Message& msg, Client& client);
	void	handleMode(const Message& msg, Client& client);
	void	handleKick(const Message& msg, Client& client);
	void	handleInvite(const Message& msg, Client& client);

	// ====== メッセージ系 ======
	void	handlePrivmsg(const Message& msg, Client& client);
	void	handleTopic(const Message& msg, Client& client);

	// ====== モード系 ======
	void	handleUserMode(const Message& msg, Client& client);
	void	handleChannelMode(const Message& msg, Client& client);
	void	replyChannelModes(Client& client, Channel& channel, const std::string& target);
	void	applyChannelMode(const Message& msg, Client& client, Channel& channel);
	void	handleModeKey(const Message& msg, Client& client, Channel& channel, ChannelModes& modes,
	                   bool adding, size_t& paramIndex);
	void	handleModeOp(const Message& msg, Client& client, Channel& channel,
	                  bool adding, size_t& paramIndex);
	void	handleModeLimit(const Message& msg, Client& client, Channel& channel,
	                     ChannelModes& modes, bool adding, size_t& paramIndex);
	void	handleBan(Client& client, Channel& channel, const std::string& chanName);
	void	broadcastModeChange(Client& client, Channel& channel, const Message& msg);

	// ====== その他 ======
	
	void	handleBot(const Message& msg, Client& client);
	void	handleHello(const Message& msg, Client& client);
	void	handleOmikuji(const Message& msg, Client& client);
	void	handleChinchiro(const Message& msg, Client& client);
	void	handleBlackjack(const Message& msg, Client& client);

	// ====== ユーティリティ系 ======
	std::string	makeUppercase(const std::string& argStr);
	//デバッグ用
	void		printMessageDebug(const Message& msg,
	                              const std::string& rawLine,
	                              const Client& client);
};

#endif
