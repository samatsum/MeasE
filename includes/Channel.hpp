#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <set>
#include <string>
#include <map>
#include <set>

class Client;

// i t k o l
//limitsは、大きい数字にして、それを制限するようにしたら実装が楽になるのでは？メンバー数の制限を既存メソッドに反映。
struct ChannelModes {
	bool		inviteOnly;			// +i
	bool		topicProtected;		// +t
	std::string	passKey;			// +k
	int			userLimit;			// +l
	ChannelModes() : inviteOnly(false), topicProtected(false), passKey(""), userLimit(1000) {}
};

class Channel {
private:
	//基本情報
	std::string				m_name;
	ChannelModes			m_channelModes;
	//メンバ情報
	std::map<int, Client*>	m_memberViews; // fdをキーにメンバー管理(メンバーの削除はサーバー経由で行う、ここは所有しない意図が命名にある)
	std::set<std::string>	m_invitedNicks;//inviteOnlyがチャンネル設定時に入れる人を保存　JOINの時に削除する以外やることない？
	std::set<int>			m_operators; // チャンネルオペレーターのfdを管理,先頭参加者をオペーレーターに、モードに持たせたほうがいい？
	//トピック管理
	std::string				m_topic;
	std::string				m_topicSetBy;
	time_t					m_topicSetTime;
	

public:
	explicit Channel(const std::string& name);
	~Channel();

	const std::string&				getName() const;

	const std::map<int, Client*>&	getMembers() const;
	bool							hasMember(int fd) const;
	void 							addMember(Client* client);
	void							removeMember(int fd);
	std::size_t						getMemberCount() const;

	void				broadcast(const std::string& message, int excludeFd = -1);
	void				broadcastAll(const std::string& message);

	void				setTopic(const std::string& topic, const std::string& setBy);
	const std::string&	getTopic() const;
	const std::string&	getTopicSetBy() const;
	const time_t&		getTopicSetTime() const;

	void				setMode(const ChannelModes& modes);
	const ChannelModes&	getModes() const;

	bool 				isOperator(int fd) const;
	void				addOperator(int fd);
	void				removeOperator(int fd);

	//招待関係
	void				addInvitedNick(const std::string& nick);
	void				removeInvitedNick(const std::string& nick);
	bool				isInvited(const std::string& nick) const;
};

#endif // CHANNEL_HPP
