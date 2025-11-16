#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <set>

class Client {

private:
	int						m_fd;
	std::string				m_host;
	std::string				m_buffer;
	std::string				m_sendBuffer;
	std::string				m_nickName;
	std::string				m_realName;
	std::string				m_userName;
	bool					m_isAuthenticated;
	bool					m_isRegistered;
	std::set<std::string>	m_joinedChannels;

public:
	Client(int fd, const std::string& host);
	~Client();

	bool				extractNextCommand(std::string &containCmd);

	void				appendBuffer(const std::string& data);
	void				clearBuffer();

	bool				isAuthenticated() const;
	bool				isRegistered() const;

	int					getFd() const;
	std::string&		getBuffer();
	std::string&		getNickName();
	const std::string&	getNickName() const;
	std::string&		getRealName();
	std::string&		getUserName();
	std::string&		getHost();

	void				setNickName(const std::string& nick);
	void				setRealName(const std::string& real);
	void				setUserName(const std::string& user);
	void				setAuthenticated(bool state);
	void				setRegistered(bool state);

	std::string makePrefix() const;
	void		joinChannel(const std::string& name);
	void		leaveChannel(const std::string& name);
	bool		isInChannel(const std::string& name) const;

	void appendSendBuffer(const std::string &msg);
	bool hasPendingSend() const;
	std::string &getSendBuffer();
};

#endif
