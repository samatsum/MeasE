#include "../includes/Client.hpp"

bool Client::extractNextCommand(std::string &containCmd) {

	std::size_t pos = m_buffer.find("\r\n");

	if (pos == std::string::npos){

		pos = m_buffer.find('\n');
		if (pos == std::string::npos)
			return false;
		containCmd = m_buffer.substr(0, pos);
		m_buffer.erase(0, pos + 1);
		return true;
	}
	containCmd = m_buffer.substr(0, pos);

	m_buffer.erase(0, pos + 2);

	return true;
}


void Client::appendBuffer(const std::string& data) {

	m_buffer += data;

}

void Client::clearBuffer() {

	m_buffer.clear();

}

bool Client::isAuthenticated() const {

	return m_isAuthenticated;

}

bool Client::isRegistered() const {

	return m_isRegistered;

}

/*=== getter =======================================*/
int	Client::getFd() const {

	return m_fd;

}

std::string& Client::getBuffer() {

	return m_buffer;

}

std::string& Client::getNickName() {

	return m_nickName;

}

const std::string& Client::getNickName() const {

	return m_nickName;

}

std::string& Client::getRealName() {
	
	return m_realName;

}

std::string& Client::getUserName() {
	
	return m_userName;

}

std::string& Client::getHost() {
	
	return m_host;

}


/*=== setter ========================================*/

void Client::setNickName(const std::string& nick) {

	m_nickName = nick;

}

void Client::setRealName(const std::string& real) {

	m_realName = real;

}

void Client::setUserName(const std::string& user) {

	m_userName = user;

}

void Client::setAuthenticated(bool state) {

	m_isAuthenticated = state;

}

void Client::setRegistered(bool state) {

	m_isRegistered = state;

}

void Client::joinChannel(const std::string& name) {

	m_joinedChannels.insert(name);

}

void Client::leaveChannel(const std::string& name) {

	m_joinedChannels.erase(name);

}

bool Client::isInChannel(const std::string& name) const {

	return m_joinedChannels.find(name) != m_joinedChannels.end();

}

/*============================================*/

std::string Client::makePrefix() const {
	return m_nickName + "!" + m_userName + "@" + m_host;
}

/*=========================================================*/

void Client::appendSendBuffer(const std::string &msg) {
	m_sendBuffer += msg;
}

bool Client::hasPendingSend() const {
	return m_sendBuffer.size() > 0;
}

std::string &Client::getSendBuffer() {
	return m_sendBuffer;
}

/*=== constructor、destructor ============================*/

Client::Client(int fd, const std::string& host)
    : m_fd(fd), m_host(host), m_isAuthenticated(false), m_isRegistered(false) {}

Client::~Client() {}
