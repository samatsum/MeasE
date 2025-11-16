#include "../includes/IrcServer.hpp"
#include "../includes/Client.hpp"
#include <sys/socket.h>//for socket, bind, listen, accept
#include <poll.h>//for poll
#include <fcntl.h>//for fcntl, O_NONBLOCK
#include <unistd.h>//for close
#include <errno.h>//for errno
#include <stdexcept>//for std::runtime_error
#include <cstring>//for std::memset
#include <csignal>//for sigatomic_t
#include <netdb.h>//for getaddrinfo, freeaddrinfo
#include <sstream>//for std::ostringstream

typedef std::map<int, Client>::iterator ClientIt;
typedef std::map<std::string, Channel>::iterator ChannelIt;

extern volatile sig_atomic_t g_signalCaught;

void IrcServer::setUpSocket() {

	struct addrinfo hints, *res, *p;
	const int optionOn = 1;
	const int optionOff = 0;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	std::ostringstream portStream;
	portStream << m_port;

	int status = getaddrinfo(NULL, portStream.str().c_str(), &hints, &res);
	if (status != 0)
		throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(status));

	struct addrinfo *boundAddr = NULL;
	for (int pass = 0; pass < 2 && boundAddr == NULL; ++pass) {
		for (p = res; p != NULL; p = p->ai_next) {
			if (pass == 0 && p->ai_family != AF_INET6)
				continue;
			if (pass == 1 && p->ai_family == AF_INET6)
				continue;

			m_listenFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (m_listenFd < 0)
				continue;

			if (p->ai_family == AF_INET6) {
				setsockopt(m_listenFd, IPPROTO_IPV6, IPV6_V6ONLY, &optionOff, sizeof(optionOff));
			}

			if (setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &optionOn, sizeof(optionOn)) < 0) {
				close(m_listenFd);
				continue;
			}

			if (bind(m_listenFd, p->ai_addr, p->ai_addrlen) == 0) {
				boundAddr = p;
				break;
			}
			close(m_listenFd);
		}
	}
	freeaddrinfo(res);

	if (boundAddr == NULL)
		throw std::runtime_error("Bind failed for both IPv4 and IPv6");

	if (fcntl(m_listenFd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl failed");

	if (listen(m_listenFd, SOMAXCONN) < 0)
		throw std::runtime_error("Listen failed");

	struct pollfd pollFd;
	pollFd.fd = m_listenFd;
	pollFd.events = POLLIN;
	m_pollFds.push_back(pollFd);
}

void	IrcServer::run() {

	while (g_signalCaught == 0)
	{
		if (m_pollFds.empty())
			continue;

		int numFds = poll(&m_pollFds[0], m_pollFds.size(), -1);
		if (numFds < 0) {
			if (errno == EINTR)
				continue;
			throw std::runtime_error("Poll failed");
		}

		for (std::size_t i = m_pollFds.size(); i > 0; --i)
		{
			struct pollfd &pfd = m_pollFds[i - 1];
			short revents = pfd.revents;

			if (revents == 0)
				continue;

			if (revents & POLLIN) {

				if (pfd.fd == m_listenFd)
					acceptNewClient();
				else
					receiveFromClient(pfd.fd);
			}

			if (revents & POLLOUT) {

				ClientIt it = m_clients.find(pfd.fd);
				if (it == m_clients.end()) {
					continue;
				}

				Client &client = it->second;
				if (!client.hasPendingSend()) {
					pfd.events = POLLIN;
					continue;
				}

				std::string &buf = client.getSendBuffer();
				std::cout << "[ Sending to fd=" << pfd.fd << " ]: " << buf;
				ssize_t sent = send(pfd.fd, buf.c_str(), buf.size(), 0);

				if (sent > 0) {

					buf.erase(0, sent);
					if (!client.hasPendingSend()) {
						pfd.events = POLLIN;
					}
				}
				else if (sent < 0) {

					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						continue;
					}
					closeClientByFd(pfd.fd);
					continue;
				}
				else {
					closeClientByFd(pfd.fd);
					continue;
				}
			}

			if (revents & POLLHUP) {
				closeClient(i - 1);
			}
		}
	}
}

void	IrcServer::enablePollout(int fd) {

	for (std::size_t i = 0; i < m_pollFds.size(); ++i) {

		if (m_pollFds[i].fd == fd) {

			short events = m_pollFds[i].events;

			if ((events & POLLOUT) == 0) {
				m_pollFds[i].events = events | POLLOUT;
			}
			break;
		}
	}
}


void	IrcServer::acceptNewClient() {

	struct sockaddr_storage	clientAddr;
	socklen_t				addrLen = sizeof(clientAddr);
	char					hostStr[NI_MAXHOST];
	char					serviceStr[NI_MAXSERV];

	std::memset(hostStr, 0, sizeof(hostStr));
	std::memset(serviceStr, 0, sizeof(serviceStr));

	int	clientFd = accept(m_listenFd, (struct sockaddr *)&clientAddr, &addrLen);
	if (clientFd < 0)
	{
		if (errno == EINTR)
			return;

		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		throw std::runtime_error("Accept failed");
	}

	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl failed");

	struct pollfd	newPollFd;
	newPollFd.fd = clientFd;
	newPollFd.events = POLLIN;
	m_pollFds.push_back(newPollFd);

	int status = getnameinfo((struct sockaddr *)&clientAddr, addrLen,
	                           hostStr, sizeof(hostStr),
	                           serviceStr, sizeof(serviceStr),
	                           NI_NUMERICHOST | NI_NUMERICSERV);
	if (status == 0) {
		std::cout << "Accepted new connection from " << hostStr << ":" << serviceStr << " (fd=" << clientFd << ")" << std::endl;
	} else {
		std::strncpy(hostStr, "unknown", sizeof(hostStr) - 1);
		std::cout << "Accepted new connection (fd=" << clientFd << ")" << std::endl;
	}

	std::pair<int, Client> p(clientFd, Client(clientFd, std::string(hostStr)));
	m_clients.insert(p);

	std::cout << "New client connected (fd=" << clientFd << ")" << std::endl;
}

void IrcServer::receiveFromClient(int fd) {
	char buffer[512];
	int bytes = recv(fd, buffer, sizeof(buffer), 0);

	if (bytes <= 0) {

		if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
			closeClientByFd(fd);
		return;
	}

	ClientIt it = m_clients.find(fd);
	if (it == m_clients.end())
		return;
	Client* client = &it->second;

	client->appendBuffer(std::string(buffer, bytes));

	std::string containCmd;

	// Commands may trigger closeClientByFd, so re-check the iterator each loop.
	while (true)
	{
		if (!client->extractNextCommand(containCmd))
			break;
		if (containCmd.length() > 512) {
			closeClientByFd(fd);
			std::cerr << "Client (fd=" << fd << ") sent command exceeding 512 bytes. Connection closed." << std::endl;
			return;
		}
		m_commandHandler.parseCommand(containCmd, *client);

		it = m_clients.find(fd);
		if (it == m_clients.end())
			return;
		client = &it->second;
	}
}

/*== resource cleanup ==========================================*/

void	IrcServer::closeClient(std::size_t i) {

	if (i >= m_pollFds.size())
		return;

	int	fd = m_pollFds[i].fd;
	closeClientByFd(fd);
}

void	IrcServer::closeClientByFd(int fd) {

	removeClientFromAllChannels(fd);
	for (std::size_t i = m_pollFds.size(); i > 0; --i) {

		if (m_pollFds[i - 1].fd == fd) {
			m_pollFds.erase(m_pollFds.begin() + (i - 1));
			break;
		}
	}

	m_clients.erase(fd);

	close(fd);

	std::cout << "Closed client (fd=" << fd << ")" << std::endl;
}

void IrcServer::requestClientClose(int fd) {
    closeClientByFd(fd);
}


/*==  Channel API ==========================================*/

Channel& IrcServer::getCreateChannel(const std::string& name) {

	ChannelIt it = m_channels.find(name);
	if (it == m_channels.end()) {

		std::pair<std::string, Channel> newChannel(name, Channel(name, this));
		it = m_channels.insert(newChannel).first;
	}
	return it->second;
}

Channel* IrcServer::findChannel(const std::string& name) {

	ChannelIt it = m_channels.find(name);
	if (it != m_channels.end()) {
		return &it->second;
	}
	return NULL;
}

Channel* IrcServer::findChannelByMember(int fd) {

	for (ChannelIt it = m_channels.begin(); it != m_channels.end(); ++it) {

		Channel& channel = it->second;
		if (channel.hasMember(fd)) {
			return &channel;
		}
	}
	return NULL;
}

void IrcServer::removeChannel(const std::string& name) {
	m_channels.erase(name);
}

void IrcServer::broadcastAll(const std::string& msg)
{
	for (ClientIt it = m_clients.begin(); it != m_clients.end(); ++it) {
		if (!it->second.isAuthenticated())
			continue;
		queueMessageForClient(it->first, msg);
	}
}

void IrcServer::removeClientFromAllChannels(int fd) {

	ChannelIt it = m_channels.begin();

	while (it != m_channels.end()) {

		Channel& channel = it->second;
		bool erased = false;

		if (channel.hasMember(fd)) {

			channel.removeOperator(fd);
			channel.removeMember(fd);
			ClientIt clientIt = m_clients.find(fd);
			if (clientIt != m_clients.end()) {

				Client& client = clientIt->second;
				client.leaveChannel(channel.getName());
			}

			if (channel.getMemberCount() == 0) {

				ChannelIt eraseIt = it;
				++it;
				m_channels.erase(eraseIt);
				erased = true;
			}
		}
		if (!erased)
		++it;
	}
}

/*==  date getters ======================================================*/

bool	IrcServer::isNickInUse(const std::string& nick) const {

	std::map<int, Client>::const_iterator it = m_clients.begin();
	for (;it != m_clients.end(); ++it) {

		const Client& client = it->second;
		const std::string clientNick = client.getNickName();
		if (clientNick == nick) {
			return true;
		}
	}
	return false;
}

Client*	IrcServer::findClientByNick(const std::string& nick) {

	std::map<int, Client>::iterator it = m_clients.begin();
	for (;it != m_clients.end(); ++it) {

		Client& client = it->second;
		const std::string clientNick = client.getNickName();
		if (clientNick == nick) {
			return &client;
		}
	}
	return NULL;
}

Client*	IrcServer::findClientByFd(int fd) {

	std::map<int, Client>::iterator it = m_clients.find(fd);
	if (it != m_clients.end()) {
		return &it->second;
	}
	return NULL;
}

std::string	IrcServer::getServerName() const {

	return m_serverName;

}

std::string	IrcServer::getPassword() const {

	return m_passWord;

}

std::map<int, Client>& IrcServer::getClients() {

	return m_clients;

}

std::map<std::string, Channel>& IrcServer::getChannels() {

	return m_channels;

}

void IrcServer::queueMessageForClient(int fd, const std::string& msg) {
	Client* c = findClientByFd(fd);
	if (!c)
		return;
	c->appendSendBuffer(msg);
	enablePollout(fd);
}

/*==  Constructor & destructor  ======================================*/

IrcServer::IrcServer(int port, const std::string &password)
	: m_serverName("ircserver"), m_port(port), m_passWord(password), m_listenFd(-1), m_commandHandler(*this) {}

IrcServer::~IrcServer() {

	for (ClientIt it = m_clients.begin(); it != m_clients.end(); ++it) {
		close(it->first);
	}
	m_clients.clear();

	if (m_listenFd >= 0)
		close(m_listenFd);

	m_pollFds.clear();

	std::cout << "Server resources cleaned up." << std::endl;
}


// void	IrcServer::run() {

// 	while (g_signalCaught == 0)
// 	{
// 		int numFds = poll(&m_pollFds[0], m_pollFds.size(), 0);
// 		if (numFds < 0) {
// 			if (errno == EINTR)
// 				continue;//これがないとシグナルのときpoll failed判定になる
// 			throw std::runtime_error("Poll failed");
// 		}

// 		for (std::size_t i = m_pollFds.size(); i > 0; --i)
// 		{
// 			//POLL_INは読み込み可能、POLL_OUTは書き込み可能、POLL_HUPは接続が切れたとき。
// 			//複数のイベントが発生することもあるので、ビット演算でPOLLINが含まれるかを見てる。
// 			if (m_pollFds[i - 1].revents & POLLIN) {

// 				if (m_pollFds[i - 1].fd == m_listenFd)
// 					acceptNewClient();//リスニングソケットにイベントが起きたら
// 				else
// 					receiveFromClient(m_pollFds[i - 1].fd);

// 			}
// 			if (m_pollFds[i - 1].revents & POLLHUP)
// 				closeClient(i - 1);//接続が切れたとき
// 		}
// 	}
	
// }

/*
void	IrcServer::setUpSocket() {

	const int optionOn = 1;
	m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_listenFd < 0)
		throw std::runtime_error("Socket creation failed");

	//再起動のあとすぐそのソケットが使えるようにする。
	int opt = optionOn;
	if (setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt failed");

	if (fcntl(m_listenFd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl failed");

	struct sockaddr_in	serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(m_port);

	if (bind(m_listenFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
		throw std::runtime_error("Bind failed");
	if (listen(m_listenFd, SOMAXCONN) < 0)
		throw std::runtime_error("Listen failed");

	struct pollfd	pollFd;
	pollFd.fd = m_listenFd;
	pollFd.events = POLLIN;
	m_pollFds.push_back(pollFd);

}
*/
