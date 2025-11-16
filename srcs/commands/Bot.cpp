#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sstream>//for std::istringstream, std::ostringstream
#include <cstdlib>//for std::rand, std::srand
#include <cstring>//for std::memset
#include <cerrno>//for errno
#include <ctime>//for time
#include <cctype>//for isalpha

void CommandHandler::handleBot(const Message& msg, Client& client)
{
    const std::string& text = msg.trailing; // 例: "!h こんにちは"

	if (text == "!h" || (text.size() > 2 && text.rfind("!h ", 0) == 0))
	{
		handleHello(msg, client);
		return;
	}
    else if (text == "!o" || (text.size() > 2 && text.rfind("!o ", 0) == 0))
    {
        handleOmikuji(msg, client);
        return;
    }
    else if (text == "!c" || (text.size() > 2 && text.rfind("!c ", 0) == 0))
    {
        handleChinchiro(msg, client);
        return;
    }
    else if (text == "!b" || text == "!s" || (text.size() > 2 && text.rfind("!b ", 0) == 0) || (text.size() > 2 && text.rfind("!s ", 0) == 0))
    {
        handleBlackjack(msg, client);
        return;
    }
    else if (text == "!r" || (text.size() > 2 && text.rfind("!r ", 0) == 0))
    {
        handleRandomSelect(msg, client);
        return;
    }
    else if (text == "!i" || (text.size() > 2 && text.rfind("!i ", 0) == 0))
    {
        handleBotInfo(msg, client);
        return;
    }

    sendError(client, "421", client.getNickName(), "Unknown BOT command");
}

void CommandHandler::handleHello(const Message& msg, Client& client)
{
	(void)client;
    std::string text = msg.trailing;

    //"!h"削除
    if (text.size() > 2)
        text = text.substr(2);
    else
        text.clear();

    while (!text.empty() && std::isspace(text[0]))
        text.erase(0, 1);
    while (!text.empty() && std::isspace(text[text.size() - 1]))
        text.erase(text.size() - 1, 1);

    if (text.empty())
        text = "Hello! Everyone!";
    std::string botMessage = "BOT: " + text;

    const std::string& channelName = msg.params[0];
    Channel* channel = m_server.findChannel(channelName);
    if (!channel)
	{

		sendError(client, "403", channelName, "No such channel");
        return;
	}

    std::string prefix = m_serverName;
    std::vector<std::string> params;
    params.push_back(channelName);

    std::string out = buildMessage(prefix, "PRIVMSG", params, botMessage);

    channel->broadcast(out, -1);
}

void CommandHandler::handleOmikuji(const Message& msg, Client& client)
{
    (void)client;
    std::string text = msg.trailing;

    if (text.size() > 2)
        text = text.substr(2);
    else
        text.clear();

    while (!text.empty() && std::isspace(text[0]))
        text.erase(0, 1);
    while (!text.empty() && std::isspace(text[text.size() - 1]))
        text.erase(text.size() - 1, 1);

    time_t now = time(NULL);
    struct tm* lt = localtime(&now);
    int y = lt->tm_year + 1900;
    int m = lt->tm_mon + 1;
    int d = lt->tm_mday;
    int dateSeed = y + m + d;

    int nickSeed = 0;
    const std::string& nick = client.getNickName();
    for (std::size_t i = 0; i < nick.size(); i++) {
        nickSeed += nick[i];
    }
    unsigned int finalSeed = dateSeed + nickSeed;

    std::vector<std::string> omikujiResults;
    omikujiResults.push_back("Super Lucky");
    omikujiResults.push_back("Great Lucky");
    omikujiResults.push_back("Lucky");
    omikujiResults.push_back("Medium Lucky");
    omikujiResults.push_back("Small Lucky");
    omikujiResults.push_back("Bad Luck, oh...");

    std::srand(finalSeed);
    int index = std::rand() % omikujiResults.size();
    std::string result = omikujiResults[index];

    std::string botMessage = "BOT: " + client.getNickName() + " omikuji result today is: " + result;

    const std::string& channelName = msg.params[0];
    Channel* channel = m_server.findChannel(channelName);
    if (!channel)
    {
        sendError(client, "403", channelName, "No such channel");
        return;
    }

    std::string prefix = m_serverName;
    std::vector<std::string> params;
    params.push_back(channelName);

    std::string out = buildMessage(prefix, "PRIVMSG", params, botMessage);
    channel->broadcast(out, -1);
}

void CommandHandler::handleChinchiro(const Message& msg, Client& client)
{
    (void)msg;

    const std::string& nick = client.getNickName();
    int d1, d2, d3;
    std::srand(time(NULL) + nick[0]);

    if (nick == "hancho" || nick == "isawa" || nick == "numakawa" || nick == "isinuma" || nick == "otsuki")
    {
        d1 = (std::rand() % 3) + 4;
        d2 = (std::rand() % 3) + 4;
        d3 = (std::rand() % 3) + 4;
    }
    else if (nick == "kaiji" || nick == "kogino" || nick == "samatsum" || nick == "oohba")
    {
        int r = std::rand() % 10;
        if (r < 5)
        {
            d1 = 1;
            d2 = 1;
            d3 = 1;
        }
        else
        {
            d1 = (std::rand() % 3) + 4;
            d2 = (std::rand() % 3) + 4;
            d3 = (std::rand() % 3) + 4;
        }
    }
    else
    {
        d1 = (std::rand() % 6) + 1;
        d2 = (std::rand() % 6) + 1;
        d3 = (std::rand() % 6) + 1;
    }

    std::string role;
    int a = d1, b = d2, c = d3;

    if (a == b && b == c)
    {
        if (a == 1)
            role = "PINZORO!";
        else
            role = "ZOROME!";
    }
    else if ((a == 4 || b == 4 || c == 4) &&
             (a == 5 || b == 5 || c == 5) &&
             (a == 6 || b == 6 || c == 6))
    {
        role = "SHIGORO!";
    }
    else if ((a == 1 || b == 1 || c == 1) &&
             (a == 2 || b == 2 || c == 2) &&
             (a == 3 || b == 3 || c == 3))
    {
        role = "HIFUMI!";
    }
    else if (a == b || a == c || b == c)
    {
        int eye;
        if (a == b) eye = c;
        else if (a == c) eye = b;
        else eye = a;
        std::stringstream ss;
        ss << "MEARI（" << eye << "）";
        role = ss.str();
    }
    else
    {
        role = "YAKUNASHI";
    }

    std::stringstream ss;
    ss << "BOT: " << nick << " rolled [" << d1 << "," << d2 << "," << d3 << "] → " << role;
    std::string botMessage = ss.str();

    const std::string& channelName = msg.params[0];
    Channel* channel = m_server.findChannel(channelName);
    if (!channel)
    {
        sendError(client, "403", channelName, "No such channel");
        return;
    }

    std::string prefix = m_serverName;
    std::vector<std::string> params;
    params.push_back(channelName);

    std::string out = buildMessage(prefix, "PRIVMSG", params, botMessage);
    channel->broadcast(out, -1);
}


void CommandHandler::handleBlackjack(const Message& msg, Client& client)
{
	const std::string &text = msg.trailing;
	const std::string nick = client.getNickName();
	const int fd = client.getFd();

	const std::string &channelName = msg.params[0];
	Channel *channel = m_server.findChannel(channelName);
	if (!channel)
	{
		sendError(client, "403", channelName, "No such channel");
		return;
	}

	//STAND
	if (text.rfind("!s", 0) == 0)
	{
		if (!channel->isBJInGame(fd))
		{
			std::string botMessage = "BOT: " + nick + " has not started Blackjack yet.";
			std::string out = buildMessage(m_serverName, "PRIVMSG",
				std::vector<std::string>(1, channelName), botMessage);
			m_server.queueMessageForClient(client.getFd(), out);
			return;
		}

		int total = channel->calcBJTotal(fd);

		std::stringstream ss;
		ss << "BOT: " << nick << " chose to stand with total point " << total << ".";
		std::string out = buildMessage(m_serverName, "PRIVMSG",
			std::vector<std::string>(1, channelName), ss.str());
		channel->broadcast(out, -1);

		channel->resetBJ(fd);
		return;
	}

	if (!channel->isBJInGame(fd))
	{
		channel->startBJFor(fd);
		std::string botMessage = "BOT: [Simple Black Jack] Welcome " + nick + "!";
		std::string out = buildMessage(m_serverName, "PRIVMSG",
			std::vector<std::string>(1, channelName), botMessage);
		m_server.queueMessageForClient(client.getFd(), out);
	}

	std::srand(std::time(NULL));
	int r = std::rand() % 13;

	std::string card;
	if (r == 0)
		card = "A";
	else if (r >= 1 && r <= 9){
        std::stringstream num;
        num << r + 1;
        card = num.str();
    }
	else if (r == 10)
		card = "J";
	else if (r == 11)
		card = "Q";
	else
		card = "K";

	channel->addBJCard(fd, card);

	int total = channel->calcBJTotal(fd);
	int count = channel->getBJCardCount(fd);

	std::stringstream ss;
	ss << "BOT: " << nick << " drew [" << card << "]";

	if (count > 1)
	{
		ss << ", total point " << total;
	}

    //judge the result
	if (total == 21)
	{
		ss << ". BLACKJACK!";
		channel->resetBJ(fd);
	}
	else if (total > 21)
	{
		ss << ". BUST";
		channel->resetBJ(fd);
	}
	else if (count >= 5)
	{
		ss << ". " << nick << " has drawn 5 cards. BUST!";
		channel->resetBJ(fd);
	}

	std::string out = buildMessage(m_serverName, "PRIVMSG",
		std::vector<std::string>(1, channelName), ss.str());
	channel->broadcast(out, -1);
}

void CommandHandler::handleRandomSelect(const Message& msg, Client& client)
{
    const std::string &channelName = msg.params[0];
	Channel *channel = m_server.findChannel(channelName);
	if (!channel)
	{
		sendError(client, "403", channelName, "No such channel");
		return;
	}

	const std::map<int, Client*> &members = channel->getMembers();
	if (members.empty())
		return;

	std::vector<Client*> list;
	for (std::map<int, Client*>::const_iterator it = members.begin();
		 it != members.end(); ++it)
	{
		Client *cl = it->second;
		list.push_back(cl);
	}

    time_t now = time(NULL);
    struct tm* lt = localtime(&now);
    int y = lt->tm_year + 1900;
    int m = lt->tm_mon + 1;
    int d = lt->tm_mday;
    int dateSeed = y + m + d + members.size();

	std::srand(dateSeed);

	int idx = std::rand() % list.size();
	Client *selected = list[idx];
    std::string botMessage;
    if (list.size() == 1)
        botMessage = "BOT: HEY! THERE IS NO ONE ELSE BUT YOU " + selected->getNickName();
    else
	    botMessage = "BOT: ATTENTION PLEASE! TODAY'S PERSON IS " + selected->getNickName() + "!";
	std::string out = buildMessage(m_serverName, "PRIVMSG",
		std::vector<std::string>(1, channelName), botMessage);
	channel->broadcast(out, -1);
}

void    CommandHandler::handleBotInfo(const Message& msg, Client& client){

    const std::string &channelName = msg.params[0];
    Channel *channel = m_server.findChannel(channelName);
    if (!channel)
    {
        sendError(client, "403", channelName, "No such channel");
        return;
    }

    std::string botMessage = "BOT: Available commands - !h (hello), !o (omikuji), !c (chinchiro), !b (blackjack), !s (stand in blackjack), !r (random select). Enjoy!";
    std::string out = buildMessage(m_serverName, "PRIVMSG",
        std::vector<std::string>(1, channelName), botMessage);
    m_server.queueMessageForClient(client.getFd(), out);
}
