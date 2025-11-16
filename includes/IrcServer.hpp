#ifndef IRCSERVER_HPP
#define IRCSERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "CommandHandler.hpp"

class Channel;
class Client;

class IrcServer{
	private:

		std::string						m_serverName;
		int								m_port;
		std::string						m_passWord;
		int								m_listenFd;
		std::vector<struct pollfd>		m_pollFds;

		std::map<int, Client>			m_clients;
		CommandHandler					m_commandHandler;
		std::map<std::string, Channel>	m_channels;
		
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

		Channel&	getCreateChannel(const std::string& name);
		Channel*	findChannel(const std::string& name);
		void		removeChannel(const std::string& name);

		void broadcastAll(const std::string& message);
		void removeClientFromAllChannels(int fd);
		void requestClientClose(int fd);
		std::map<std::string, Channel>& getChannels();
		Channel* findChannelByMember(int fd);

		void	enablePollout(int fd);
		Client*	findClientByFd(int fd);
		void queueMessageForClient(int fd, const std::string& msg);
};

#endif
