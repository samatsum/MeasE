#include "../includes/CommandHandler.hpp"
#include "../includes/IrcServer.hpp"
#include <iostream>//for std::cout, std::cerr

void CommandHandler::parseCommand(const std::string& line, Client& client)
{
	Message msg = parse(line);
	if (!msg.isValid) {

		std::string context;
		if (msg.command.empty())
			context = line;
		else
			context = msg.command;
		sendError(client, "421", context, "Invalid command: " + msg.errorDetail);
		std::cerr << "[fd " << client.getFd() << "] Invalid IRC line: " << msg.errorDetail << std::endl;
		return;
	}

	printMessageDebug(msg, line, client);
	std::string cmd = makeUppercase(msg.command);

	std::map<std::string, CommandFunc>::iterator it = m_cmdMap.find(cmd);
	if (it != m_cmdMap.end()) {

		(this->*it->second)(msg, client);
	} else {

		sendError(client, "421", cmd, "Unknown command");
		std::cout << "[fd " << client.getFd() << "] Unknown command: " << cmd << std::endl;
	}
}

/*=====================================================*/

CommandHandler::CommandHandler(IrcServer& server)
	: m_server(server), m_serverName(server.getServerName())
{

	m_cmdMap["CAP"]     = &CommandHandler::handleCAP;
	m_cmdMap["PING"]    = &CommandHandler::handlePing;
	m_cmdMap["PASS"]    = &CommandHandler::handlePass;
	m_cmdMap["NICK"]    = &CommandHandler::handleNick;
	m_cmdMap["USER"]    = &CommandHandler::handleUser;
	m_cmdMap["PRIVMSG"] = &CommandHandler::handlePrivmsg;
	m_cmdMap["JOIN"]    = &CommandHandler::handleJoin;
	m_cmdMap["QUIT"]    = &CommandHandler::handleQuit;
	m_cmdMap["PART"]    = &CommandHandler::handlePart;
	m_cmdMap["TOPIC"]   = &CommandHandler::handleTopic;
	m_cmdMap["MODE"]    = &CommandHandler::handleMode;
	m_cmdMap["KICK"]    = &CommandHandler::handleKick;
	m_cmdMap["INVITE"]  = &CommandHandler::handleInvite;
	m_cmdMap["WHO"]     = &CommandHandler::handleWho;
	m_cmdMap["WHOIS"]   = &CommandHandler::handleWhois;
	m_cmdMap["NAMES"]   = &CommandHandler::handleNames;
}

CommandHandler::~CommandHandler() {}

/*=====================================================*/

void CommandHandler::printMessageDebug(const Message& msg, const std::string& rawLine, const Client& client)
{
	std::cout << "===== Parsed IRC Message (fd " << client.getFd() << ") =====" << std::endl; 
	std::cout << "Prefix   : ";
	if (msg.prefix.empty()) {
		std::cout << "(none)";
	} else {
		std::cout << msg.prefix;
	}
	std::cout << std::endl;

	std::cout << "Command  : " << msg.command << std::endl;

	std::cout << "Params   : ";
	if (msg.params.empty()) {
		std::cout << "(none)";
	} else {
		for (std::size_t i = 0; i < msg.params.size(); ++i) {
			std::cout << "[" << i << "]=" << msg.params[i];
			if (i + 1 < msg.params.size())
				std::cout << ", ";
		}
	}
	std::cout << std::endl;

	std::cout << "Trailing : ";
	if (msg.trailing.empty()) {
		std::cout << "(none)";
	} else {
		std::cout << msg.trailing;
	}
	std::cout << std::endl;

	std::cout << "Raw line : " << rawLine << std::endl;
	std::cout << "==================================================" << std::endl;
}
