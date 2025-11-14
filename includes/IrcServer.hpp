#ifndef IRCSERVER_HPP
#define IRCSERVER_HPP

#include "CommandHandler.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <poll.h>
#include <map>

class Client;

// struct AnsiColor {
// 	static const char* const RED;
// 	static const char* const GREEN;
// 	static const char* const BLUE;
// 	static const char* const YELLOW;
// 	static const char* const END;
// };

// const char* const AnsiColor::RED = "\033[31m";
// const char* const AnsiColor::GREEN = "\033[32m";
// const char* const AnsiColor::BLUE = "\033[34m";
// const char* const AnsiColor::YELLOW = "\033[33m";
// const char* const AnsiColor::END = "\033[0m";

class IrcServer{
	private:
		//サーバー昨日関連
		std::string						m_serverName;
		int								m_port;
		std::string						m_passWord;
		int								m_listenFd;
		std::vector<struct pollfd>		m_pollFds;

		//親機能
		std::map<int, Client>			m_clients;
		CommandHandler					m_commandHandler;
		std::map<std::string, Channel>	m_channels;
		
		//クライアント管理系
		void	acceptNewClient();
		void	receiveFromClient(int fd);
		void	closeClient(std::size_t i);
		void	closeClientByFd(int fd);

	public:
		IrcServer(int port, const std::string &password);
		~IrcServer();

		void	setUpSocket();
		void	run();

		std::string				getServerName() const;
		std::string				getPassword() const;
		std::map<int, Client>&	getClients();

		bool	isNickInUse(const std::string& nick) const;
		Client*	findClientByNick(const std::string& nick);

		//チャンネルAPI
		Channel&	getCreateChannel(const std::string& name);
		Channel*	findChannel(const std::string& name);
		void		removeChannel(const std::string& name);

		//チャンネルを横断するためのAPIも作る。
		void broadcastAll(const std::string& message);
		void removeClientFromAllChannels(int fd);
		void requestClientClose(int fd);
		std::map<std::string, Channel>& getChannels();
		Channel* findChannelByMember(int fd);
};

#endif
