#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <set>
#include <string>
#include <map>
#include <set>
#include <vector>

class Client;
class IrcServer;

struct ChannelModes {
	bool		inviteOnly;			// +i
	bool		topicProtected;		// +t
	std::string	passKey;			// +k
	int			userLimit;			// +l
	ChannelModes() : inviteOnly(false), topicProtected(false), passKey(""), userLimit(1000) {}
};

struct BJState {
    std::vector<std::string> cards; 
    bool inGame;

    BJState() : inGame(false) {}
};

class Channel {
private:

	std::string				m_name;
	ChannelModes			m_channelModes;

	IrcServer*				m_server;

	std::map<int, Client*>	m_memberViews;
	std::set<std::string>	m_invitedNicks;
	std::set<int>			m_operators;

	std::string				m_topic;
	std::string				m_topicSetBy;
	time_t					m_topicSetTime;

	std::map<int, BJState> m_bjStates;
	

public:
	explicit Channel(const std::string& name, IrcServer* server);
	~Channel();

	const std::string&				getName() const;

	const std::map<int, Client*>&	getMembers() const;
	bool							hasMember(int fd) const;
	void 							addMember(Client* client);
	void							removeMember(int fd);
	std::size_t						getMemberCount() const;

	void				broadcast(const std::string& message, int excludeFd = -1);

	void				setTopic(const std::string& topic, const std::string& setBy);
	const std::string&	getTopic() const;
	const std::string&	getTopicSetBy() const;
	const time_t&		getTopicSetTime() const;

	void				setMode(const ChannelModes& modes);
	const ChannelModes&	getModes() const;

	bool 				isOperator(int fd) const;
	void				addOperator(int fd);
	void				removeOperator(int fd);

	void				addInvitedNick(const std::string& nick);
	void				removeInvitedNick(const std::string& nick);
	bool				isInvited(const std::string& nick) const;

	void				startBJFor(int fd);  
	void				addBJCard(int fd, const std::string &card);
	int					calcBJTotal(int fd) const;
	bool				isBJInGame(int fd) const;
	void				resetBJ(int fd);
	int 				getBJCardCount(int fd) const;
};

#endif // CHANNEL_HPP
