#include "../includes/Channel.hpp"
#include "../includes/Client.hpp"

#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <ctime>

/*
【概要】
チャンネルごとの情報を管理するクラス。
メンバーの追加、削除、ブロードキャストを行う。


ブロードキャストは、自分にもしないといけない場合が、ある
JOINのときとか。

リムーブから、オペレーターも一緒に削除した。

*/

typedef std::map<int, Client*>::iterator MemberIt;

//キャリッジリターンは呼び出し元で。
void	Channel::broadcast(const std::string& message, int excludeFd) {

	for (MemberIt it = m_memberViews.begin(); it != m_memberViews.end(); ++it) {

		int memberFd = it->first;

		if (memberFd != excludeFd) {
			std::string msgWithCRLF = message;
			if (send(memberFd, msgWithCRLF.c_str(), msgWithCRLF.length(), 0) == -1) {
				std::cerr << "Failed to send message to fd " << memberFd << std::endl;
			}
		}
	}
}

//こっちは使ってないかも。
void	Channel::broadcastAll(const std::string& message) {

	for (MemberIt it = m_memberViews.begin(); it != m_memberViews.end(); ++it) {

		int memberFd = it->first;

		std::string msgWithCRLF = message;
		if (send(memberFd, msgWithCRLF.c_str(), msgWithCRLF.length(), 0) == -1) {
			std::cerr << "Failed to send message to fd " << memberFd << std::endl;
		}
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
		m_operators.erase(fd); //オペレーター権限も削除
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

/*== constructor destructor ===========================================================*/
Channel::Channel(const std::string& name) : m_name(name) {}

Channel::~Channel() {}
