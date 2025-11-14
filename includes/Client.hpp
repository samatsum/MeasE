#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <netinet/in.h>
#include <set>
#include <vector>


class Client {

private:
	int						m_fd;
	std::string				m_host;
	std::string				m_buffer;
	std::string				m_nickName;//NICKコマンドで決めるチャット名
	std::string				m_realName;//unix的なシステムユーザーID(login名)
	std::string				m_userName;//人間の名前
	bool					m_isAuthenticated;//pass認証用
	bool					m_isRegistered;//情報の登録完了用
	std::set<std::string>	m_joinedChannels;
	std::vector<std::string> m_botCards;//ブラックジャック用の引いたカード保存用

public:
	//Client(int fd);
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
	void				setAuthenticated(bool state);//pass用
	void				setRegistered(bool state);//register用

	std::string makePrefix() const;
	void		joinChannel(const std::string& name);
	void		leaveChannel(const std::string& name);
	bool		isInChannel(const std::string& name) const;

	void		addBotCard(const std::string& card);
	void		resetBotCards();
	int			calcBotBJTotal() const;
	size_t		getBotCardCount() const;
};

#endif
