#include "../includes/Client.hpp"
#include <unistd.h>
#include <cstdlib>

/*
【概要】
クライアントごとの情報を管理するクラス。

ホストネームは固定値にしておく。

そのため、リナックス側での対応を考慮する必要がある。
*/

//リナックス環境では、LFだけで改行される？netcatだとどうか？これを考慮して実装を考える。
bool Client::extractNextCommand(std::string &containCmd) {

	std::size_t pos = m_buffer.find("\r\n");

	//改行がないと、nposは-1になる。つまりまだ改行がなかったね。//ここでリナックスだとこの処理が必要になるんじゃないかで入れてる。
	if (pos == std::string::npos){

		pos = m_buffer.find('\n');
		if (pos == std::string::npos)
			return false;
		containCmd = m_buffer.substr(0, pos);
		m_buffer.erase(0, pos + 1);
		return true;
	}
	// 改行までの文字を切り抜いて
	containCmd = m_buffer.substr(0, pos);

	// 抽出した分を削除（CRLF 2文字分を含めて）
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
	return m_nickName + "!" + m_userName + "@" + m_host; // m_host を持っている前提
}

/*=========================================================*/

void Client::addBotCard(const std::string& card) {

	m_botCards.push_back(card);
}

void Client::resetBotCards() {

	m_botCards.clear();
}

int Client::calcBotBJTotal() const
{
    int total = 0;
    int aceCount = 0;

    for (size_t i = 0; i < m_botCards.size(); i++)
    {
        const std::string& card = m_botCards[i];

        if (card == "J" || card == "Q" || card == "K")
        {
            total += 10;
        }
        else if (card == "0") // Ace
        {
            total += 1;
            aceCount++;
        }
        else
        {
			//total += std::stoi(card);
			total += std::atoi(card.c_str());//std::stringstreamなら、"abc" のような変換できない文字列を 0 として返すことは無いけどめんどくさい。
        }
    }

	//aは、1か11で扱い、有利な方を適用
    for (int i = 0; i < aceCount; i++)
    {
        if (total + 10 <= 21)
            total += 10;
    }

    return total;
}

size_t Client::getBotCardCount() const {

	return m_botCards.size();
}

/*=== コンストラクタ、デストラクタ ============================*/

//Client::Client(int fd) : m_fd(fd), m_host("localhost"), m_isAuthenticated(false), m_isRegistered(false) {}

Client::Client(int fd, const std::string& host)
    : m_fd(fd), m_host(host), m_isAuthenticated(false), m_isRegistered(false) {}

Client::~Client() {}
