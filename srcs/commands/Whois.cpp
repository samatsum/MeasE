#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <vector>
#include <string>

/*
      Command: WHOIS
   Parameters: [ <target> ] <mask> *( "," <mask> )

   This command is used to query information about particular user.
   The server will answer this command with several numeric messages
   indicating different statuses of each user which matches the mask (if
   you are entitled to see them).  If no wildcard is present in the
   <mask>, any information about that nick which you are allowed to see
   is presented.

   If the <target> parameter is specified, it sends the query to a
   specific server.  It is useful if you want to know how long the user
   in question has been idle as only local server (i.e., the server the
   user is directly connected to) knows that information, while
   everything else is globally known.

   Wildcards are allowed in the <target> parameter.

   Numeric Replies:

           ERR_NOSUCHSERVER              ERR_NONICKNAMEGIVEN
           RPL_WHOISUSER                 RPL_WHOISCHANNELS
           RPL_WHOISCHANNELS             RPL_WHOISSERVER
           RPL_AWAY                      RPL_WHOISOPERATOR
           RPL_WHOISIDLE                 ERR_NOSUCHNICK
           RPL_ENDOFWHOIS

エラーは、NONICKNAMEGIVEN、NOSUCHNICK、NOSUCHSERVERのみのエラーハンドリングで行ける。
AWAYは対応しない、オペレーターと、ワイルドカードは未実装。

/conectをirssiで多重呼び出しをすると、whoisが飛んでくるのと一緒に、ニックのチェックも山のようにとんでくることが
samatsum氏が発見されたので、安定性向上のために対応しました。
*/

void	CommandHandler::handleWhois(const Message& msg, Client& client)
{
	if (!client.isAuthenticated())
	{
		std::vector<std::string> params;
		params.push_back(client.getNickName());
		sendMsg(client.getFd(), "", "451", params, "You have not registered");
		return;
	}

	// パラメータが無い場合 -> 431
	if (msg.params.empty())
	{
		std::vector<std::string> params;
		params.push_back(client.getNickName());
		sendMsg(client.getFd(), "", "431", params, "No nickname given");
		return;
	}

	std::string targetNick = msg.params[0];

	// 対象ユーザー検索
	Client* target = NULL;
	std::map<int, Client>& clients = m_server.getClients();
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if (it->second.getNickName() == targetNick)
		{
			target = &(it->second);
			break;
		}
	}

	// 存在しない場合 -> 401
	if (!target)
	{
		std::vector<std::string> params;
		params.push_back(client.getNickName());
		params.push_back(targetNick);
		sendMsg(client.getFd(), "", "401", params, "No such nick");
		return;
	}

	// 311 RPL_WHOISUSER
	{
		std::vector<std::string> params;
		params.push_back(client.getNickName());
		params.push_back(target->getNickName());
		params.push_back(target->getUserName());
		params.push_back(target->getHost());
		params.push_back("*");
		sendMsg(client.getFd(), "", "311", params, target->getRealName());
	}

	// 312 RPL_WHOISSERVER
	{
		std::vector<std::string> params;
		params.push_back(client.getNickName());
		params.push_back(target->getNickName());
		params.push_back(m_server.getServerName());
		sendMsg(client.getFd(), "", "312", params, "ft_irc server");
	}

	// 318 RPL_ENDOFWHOIS
	{
		std::vector<std::string> params;
		params.push_back(client.getNickName());
		params.push_back(target->getNickName());
		sendMsg(client.getFd(), "", "318", params, "End of WHOIS list");
	}
}

// void	CommandHandler::handleWhois(const Message& msg, Client& client)
// {
// 	// if (!client.isAuthenticated())
// 	// {
// 	// 	std::vector<std::string> params;
// 	// 	params.push_back(client.getNickName());
// 	// 	sendMsg(client.getFd(), "", "451", params, "You have not registered");
// 	// 	return;
// 	// }

// 	if (msg.params.empty())
// 	{
// 		std::vector<std::string> params;
// 		params.push_back(client.getNickName());
// 		sendMsg(client.getFd(), "", "431", params, "No nickname given");
// 		return;
// 	}

// 	std::string maskList;
// 	if (msg.params.size() == 1)
// 	{
// 		maskList = msg.params[0];
// 	}
// 	else
// 	{
// 		const std::string& targetServer = msg.params[0];
// 		if (!targetServer.empty() && targetServer != m_server.getServerName())
// 		{
// 			std::vector<std::string> params;
// 			params.push_back(client.getNickName());
// 			params.push_back(targetServer);
// 			sendMsg(client.getFd(), "", "402", params, "No such server");
// 			return;
// 		}
// 		maskList = msg.params[msg.params.size() - 1];
// 	}

// 	std::vector<std::string> targets;
// 	std::size_t start = 0;
// 	while (start <= maskList.size())
// 	{
// 		std::size_t comma = maskList.find(',', start);
// 		std::string token = maskList.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
// 		if (!token.empty())
// 			targets.push_back(token);
// 		if (comma == std::string::npos)
// 			break;
// 		start = comma + 1;
// 	}

// 	if (targets.empty())
// 	{
// 		std::vector<std::string> params;
// 		params.push_back(client.getNickName());
// 		sendMsg(client.getFd(), "", "431", params, "No nickname given");
// 		return;
// 	}

// 	std::map<int, Client>& clients = m_server.getClients();
// 	std::map<std::string, Channel>& channels = m_server.getChannels();

// 	for (std::size_t i = 0; i < targets.size(); ++i)
// 	{
// 		const std::string& nick = targets[i];
// 		Client* targetClient = NULL;

// 		for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
// 		{
// 			if (it->second.getNickName() == nick)
// 			{
// 				targetClient = &(it->second);
// 				break;
// 			}
// 		}

// 		if (!targetClient)
// 		{
// 			std::vector<std::string> params;
// 			params.push_back(client.getNickName());
// 			params.push_back(nick);
// 			sendMsg(client.getFd(), "", "401", params, "No such nick");
// 			continue;
// 		}

// 		// RPL_WHOISUSER (311)
// 		{
// 			std::vector<std::string> params;
// 			params.push_back(client.getNickName());
// 			params.push_back(targetClient->getNickName());
// 			params.push_back(targetClient->getUserName());
// 			params.push_back(targetClient->getHost());
// 			params.push_back("*");
// 			sendMsg(client.getFd(), "", "311", params, targetClient->getRealName());
// 		}

// 		// RPL_WHOISSERVER (312)
// 		{
// 			std::vector<std::string> params;
// 			params.push_back(client.getNickName());
// 			params.push_back(targetClient->getNickName());
// 			params.push_back(m_server.getServerName());
// 			sendMsg(client.getFd(), "", "312", params, "ft_irc server");
// 		}

// 		// RPL_WHOISCHANNELS (319)
// 		std::string channelList;
// 		for (std::map<std::string, Channel>::iterator chIt = channels.begin(); chIt != channels.end(); ++chIt)
// 		{
// 			Channel& ch = chIt->second;
// 			if (!ch.hasMember(targetClient->getFd()))
// 				continue;
// 			if (!channelList.empty())
// 				channelList += " ";
// 			if (ch.isOperator(targetClient->getFd()))
// 				channelList += "@";
// 			channelList += ch.getName();
// 		}
// 		if (!channelList.empty())
// 		{
// 			std::vector<std::string> params;
// 			params.push_back(client.getNickName());
// 			params.push_back(targetClient->getNickName());
// 			sendMsg(client.getFd(), "", "319", params, channelList);
// 		}

// 		// RPL_ENDOFWHOIS (318)
// 		{
// 			std::vector<std::string> params;
// 			params.push_back(client.getNickName());
// 			params.push_back(targetClient->getNickName());
// 			sendMsg(client.getFd(), "", "318", params, "End of WHOIS list");
// 		}
// 	}
// }
