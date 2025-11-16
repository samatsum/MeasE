#include "../includes/Channel.hpp"
#include "../includes/Client.hpp"
#include "../includes/IrcServer.hpp"
#include <cstdlib>
#include <ctime>

typedef std::map<int, Client*>::iterator MemberIt;

void Channel::broadcast(const std::string& message, int excludeFd) {

	for (MemberIt it = m_memberViews.begin(); it != m_memberViews.end(); ++it) {

		int memberFd = it->first;

		if (memberFd == excludeFd)
			continue;

		m_server->queueMessageForClient(memberFd, message);
	}
}

/*======================================================================*/

void Channel::addMember(Client* client) {
	if (client == NULL)
		return;
	
	m_memberViews[client->getFd()] = client;
}

bool Channel::hasMember(int fd) const {

	return m_memberViews.find(fd) != m_memberViews.end();
}

void Channel::removeMember(int fd) {

	MemberIt it = m_memberViews.find(fd);
	if (it != m_memberViews.end()) {
		m_memberViews.erase(it);
		m_operators.erase(fd);

		std::map<int, BJState>::iterator bjIt = m_bjStates.find(fd);
		if (bjIt != m_bjStates.end())
		{
			m_bjStates.erase(bjIt);
		}

		std::cout << "Removed member fd " << fd << " from channel " << m_name << std::endl;
	}
	else
		std::cerr << "Attempted to remove non-existent member fd " << fd << " from channel " << m_name << std::endl;
}

bool Channel::isOperator(int fd) const {
	return m_operators.find(fd) != m_operators.end();
}

void Channel::addOperator(int fd) {
	m_operators.insert(fd);
}

void Channel::removeOperator(int fd) {
	if (isOperator(fd))
		m_operators.erase(fd);
}

void Channel::addInvitedNick(const std::string& nick) {
	m_invitedNicks.insert(nick);
}

void Channel::removeInvitedNick(const std::string& nick) {
	m_invitedNicks.erase(nick);
}

bool Channel::isInvited(const std::string& nick) const {
	return m_invitedNicks.find(nick) != m_invitedNicks.end();
}

/*== getter ============================================================*/

const std::string& Channel::getName() const {
	return m_name;
}

const std::string& Channel::getTopic() const {
	return m_topic;
}

const ChannelModes& Channel::getModes() const {
	return m_channelModes;
}

const std::map<int, Client*>& Channel::getMembers() const {
	return m_memberViews; 
}

size_t Channel::getMemberCount() const {
	return m_memberViews.size();
}

/*== setter ============================================================*/

void Channel::setTopic(const std::string& topic, const std::string& setBy) {
	m_topic = topic;
	m_topicSetBy = setBy;
	m_topicSetTime = std::time(NULL);
}

const std::string& Channel::getTopicSetBy() const {
	return m_topicSetBy;
}

const time_t& Channel::getTopicSetTime() const {
	return m_topicSetTime;
}

void Channel::setMode(const ChannelModes& modes) {
	m_channelModes = modes;
}

/*== blackjack ===========================================================*/

typedef std::map<int, BJState>::iterator BJIt;
typedef std::map<int, BJState>::const_iterator BJConstIt;

void Channel::startBJFor(int fd)
{
	BJIt it = m_bjStates.find(fd);
	if (it == m_bjStates.end())
	{
		BJState st;
		m_bjStates.insert(std::make_pair(fd, st));
		it = m_bjStates.find(fd);
	}
	it->second.cards.clear();
	it->second.inGame = true;
}

void Channel::addBJCard(int fd, const std::string &card)
{
	BJIt it = m_bjStates.find(fd);
	if (it == m_bjStates.end())
	{
		BJState st;
		m_bjStates.insert(std::make_pair(fd, st));
		it = m_bjStates.find(fd);
	}
	it->second.cards.push_back(card);
}

int Channel::calcBJTotal(int fd) const
{
	BJConstIt it = m_bjStates.find(fd);
	if (it == m_bjStates.end())
		return 0;

	const BJState &st = it->second;

	int total = 0;
	int aces = 0;

	for (size_t i = 0; i < st.cards.size(); ++i)
	{
		const std::string &c = st.cards[i];

		if (c == "A")
		{
			total += 11;
			aces++;
		}
		else if (c == "K" || c == "Q" || c == "J")
		{
			total += 10;
		}
		else
		{

			total += std::atoi(c.c_str());
		}
	}

	while (total > 21 && aces > 0)
	{
		total -= 10;
		aces--;
	}

	return total;
}

bool Channel::isBJInGame(int fd) const
{
	BJConstIt it = m_bjStates.find(fd);
	if (it == m_bjStates.end())
		return false;

	return it->second.inGame;
}

void Channel::resetBJ(int fd)
{
	BJIt it = m_bjStates.find(fd);
	if (it != m_bjStates.end())
	{
		it->second.cards.clear();
		it->second.inGame = false;
	}
}

int Channel::getBJCardCount(int fd) const
{
	BJConstIt it = m_bjStates.find(fd);
	if (it == m_bjStates.end())
		return 0;

	return it->second.cards.size();
}

/*== constructor destructor ===========================================================*/
Channel::Channel(const std::string& name, IrcServer* server) : m_name(name), m_server(server) {
	m_topic.clear();
	m_topicSetBy.clear();
	m_topicSetTime = 0;
}

Channel::~Channel() {}
