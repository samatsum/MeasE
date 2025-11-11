#include "../includes/IrcServer.hpp"
#include "../includes/Client.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <csignal>
#include <netdb.h>
#include <sstream>

/*
【概要】
パケットを受け取り、クライアントクラスにバッファーする。
新しい接続済ソケットを、クライアントクラスと結びつける。
チャンネルとクライアントを所有して、生成削除のAPIを提供する。

【予定】
EINTRは、シグナル割り込みだが、ポール、アクセプト、レシーブで発生を考慮（ポール対応済み）
チャンネル実装時に、チャンネルから外す処理が必要。

POLLの待機は、IOのブロッキングと少し違う話かも。
（待機は0はCPU使いすぎ？）

sendが一回で送れなかったとき→sefesendを呼び出すことを検討。

512バイトを超えたときの処理があまり厳密じゃない。

*/

typedef std::map<int, Client>::iterator ClientIt;
typedef std::map<std::string, Channel>::iterator ChannelIt;

extern volatile sig_atomic_t g_signalCaught;

void IrcServer::setUpSocket() {

	struct addrinfo hints, *res, *p;//hintsは、インスタンス、resは、結果のリスト、pはループ用。
	const int optionOn = 1;
	const int optionOff = 0;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;// IPv4/IPv6 両対応
	hints.ai_socktype = SOCK_STREAM;//TCPソケット
	hints.ai_flags = AI_PASSIVE;//任意のアドレスでバインドできるように。

	std::ostringstream portStream;
	portStream << m_port;

	int status = getaddrinfo(NULL, portStream.str().c_str(), &hints, &res);//ipv6に対応できるかのチェックを開始
	if (status != 0)
		throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(status));//gai_strerrorで、失敗原因を文字列で取得

	struct addrinfo *boundAddr = NULL;
	for (int pass = 0; pass < 2 && boundAddr == NULL; ++pass) {
		for (p = res; p != NULL; p = p->ai_next) {
			if (pass == 0 && p->ai_family != AF_INET6)
				continue;// 1周目はIPv6を優先
			if (pass == 1 && p->ai_family == AF_INET6)
				continue;// 2周目ではIPv4やその他のみを試す

			m_listenFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (m_listenFd < 0)
				continue; // 次候補を試す

			// IPv6の「IPv4マッピング無効」を有効に設定。リナックスだとデフォルトで無効らし。
			if (p->ai_family == AF_INET6) {
				setsockopt(m_listenFd, IPPROTO_IPV6, IPV6_V6ONLY, &optionOff, sizeof(optionOff));
			}

			// 再起動直後でもすぐにバインドできるようにする
			if (setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &optionOn, sizeof(optionOn)) < 0) {
				close(m_listenFd);
				continue;
			}

			if (bind(m_listenFd, p->ai_addr, p->ai_addrlen) == 0) {
				boundAddr = p;
				break; // バインド成功が目的
			}
			close(m_listenFd);
		}
	}
	freeaddrinfo(res);//アドレス情報リストはマロックしてるっぽい

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

//
void	IrcServer::run() {

	while (g_signalCaught == 0)
	{
		int numFds = poll(&m_pollFds[0], m_pollFds.size(), 0);
		if (numFds < 0) {
			if (errno == EINTR)
				continue;//これがないとシグナルのときpoll failed判定になる
			throw std::runtime_error("Poll failed");
		}

		for (std::size_t i = m_pollFds.size(); i > 0; --i)
		{
			//複数のイベントが発生することもあるので、ビット演算でPOLLINが含まれるかを見てる。
			if (m_pollFds[i - 1].revents & POLLIN) {

				if (m_pollFds[i - 1].fd == m_listenFd)
					acceptNewClient();//リスニングソケットにイベントが起きたら
				else
					receiveFromClient(m_pollFds[i - 1].fd);

			}
			if (m_pollFds[i - 1].revents & POLLHUP)
				closeClient(i - 1);//接続が切れたとき
		}
	}
	
}

//リスニングソケットに通信が来たら、クライアントソケットを作成して、ポールの監視と、自前のマップに追加する。
void	IrcServer::acceptNewClient() {

	struct sockaddr_storage	clientAddr;//socketaddr_inもsockaddr_in6も入るように。
	socklen_t				addrLen = sizeof(clientAddr);
	char					hostStr[NI_MAXHOST];
	char					serviceStr[NI_MAXSERV];

	//歴史的経緯で、EAGAINとEWOULDBRLOCKは、使い分けられてるが、どちらもソケットが読み書きできない状況を示す。
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

	//数値のIP文字列を取得、ホスト名（DNS）を取得したいときは、NI_NAMEREQDを指定する。
	int status = getnameinfo((struct sockaddr *)&clientAddr, addrLen,
	                           hostStr, sizeof(hostStr),
	                           serviceStr, sizeof(serviceStr),
	                           NI_NUMERICHOST | NI_NUMERICSERV);
	if (status == 0)
		std::cout << "Accepted new connection from " << hostStr << ":" << serviceStr << " (fd=" << clientFd << ")" << std::endl;
	else
		std::cout << "Accepted new connection (fd=" << clientFd << ")" << std::endl;

	std::pair<int, Client> p(clientFd, Client(clientFd, std::string(hostStr)));
	m_clients.insert(p);

	std::cout << "New client connected (fd=" << clientFd << ")" << std::endl;
}

/*
m_clients.insert(std::make_pair(clientFd, Client(clientFd)));

std::pair<int, Client> p(clientFd, Client(clientFd));
m_clients.insert(p);

m_clients[clientFd] = Client(clientFd);
//引数なしコンストラクタを設定してないからだめ。insertかemplace
*/


void IrcServer::receiveFromClient(int fd) {
	char buffer[512];//512ちょうどじゃなくて、512以上で確保して、それをオーバーしたらのほうがいいかも。
	int bytes = recv(fd, buffer, sizeof(buffer), 0);

	if (bytes <= 0) {
		//これは、クライアントが接続を閉じたか（eof）、一時的に読み書きできない状況以外
		if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
			closeClientByFd(fd);
		return;
	}

	//nc で分割送信したときの確認使える。
	std::cout << "Received " << bytes << " bytes from client (fd=" << fd << ")" << std::endl;

	ClientIt it = m_clients.find(fd);
	if (it == m_clients.end())
		return;
	Client &client = it->second;

	client.appendBuffer(std::string(buffer, bytes));

	//少なくとも蓄積を含めて512を超えるようなものは拒否する
	//CRLFも含めて512バイトなので、厳密には512バイトを超えたら拒否する。
	if (client.getBuffer().length() >= 512) {

		closeClientByFd(fd);
		std::cerr << "Client (fd=" << fd << ") exceeded buffer limit. Connection closed." << std::endl;
		return;
	}

	std::string containCmd;
	//１つのバッファーで複数のコマンドが来たときようにワイルで回している。
	while (client.extractNextCommand(containCmd))
	{
		m_commandHandler.parseCommand(containCmd, client);
	}
}

/*== resource cleanup ==========================================*/

void	IrcServer::closeClient(std::size_t i) {

	if (i >= m_pollFds.size())
		return;

	int	fd = m_pollFds[i].fd;
	closeClientByFd(fd);
}

//データは、ポール、クライアント、接続済ソケットの3種類がある
//逆順なのは、eraseのときに、整列したときにずれが起きないようにするため。
void	IrcServer::closeClientByFd(int fd) {

	removeClientFromAllChannels(fd);//すでにチャンネルに参加しているときのために。
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

		std::pair<std::string, Channel> newChannel(name, Channel(name));
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

void IrcServer::broadcastAll(const std::string& message) {
	for (ClientIt it = m_clients.begin(); it != m_clients.end(); ++it) {

		if (send(it->first, message.c_str(), message.length(), 0) < 0) {
			std::cerr << "Failed to broadcast message to client (fd=" << it->first << ")" << std::endl;
		}
	}
}

void IrcServer::removeClientFromAllChannels(int fd) {

	ChannelIt it = m_channels.begin();

	//チャンネルからユーザーを探して、いたら、削除する。
	while (it != m_channels.end()) {

		Channel& channel = it->second;
		bool erased = false;

		if (channel.hasMember(fd)) {

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
