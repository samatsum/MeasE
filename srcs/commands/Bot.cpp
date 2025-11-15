#include "../../includes/CommandHandler.hpp"
#include "../../includes/IrcServer.hpp"
#include "../../includes/Channel.hpp"
#include <sys/socket.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <cctype>
//#include <random> C++98ではない
#include <sstream>
#include <cstdlib> // std::rand(), std::srand() のため
#include <ctime>   // std::time() のため

/*
基本的な考え

クライアントアプリは、独自のコマンドを送信してくれない。
そのため、PRIVMSGを利用してBOTコマンドを作成する。

irssiは、
/msg target trailing
の形式になっているため、たとえば、BOTであることを示すために、
param[0]をもたせようとすると、
PRIVMSG target :param[0] trailing
のようになり、ターゲット　のあとの　引数ではなくて、トレーリング扱いになる。

対処は2つあり、ターゲットを指定しないで、そこをBOTの識別子にする。
もしくは、トレーリングにフラグを入れる。

最初の、ターゲットを指定しないということは、チャンネルに関する情報や
ユーザーに関する情報が取れないため、BOTの応答が難しい。

トレーリングの最初を:<!flag trailing>のようにして、
トレーリングに、!で始まるフラグをつけて、BOTコマンドを指定する。
そうすることで、前者の問題を回避できる。

今回は、この後者を用いる。


簡易的な、BJの実装の構想
まず、ユーザーに、トランプを一枚引かせる。一番最初は、引数なしで。
で、結果をユーザーに保存して、返す。
BOT : you drew 7 of Hearts [point 7]
この場合は、ユーザーのニックネームと、カードを保存する、
データをクライアントに持たせるならば、メンバに
std::vector<std::string> m_botCards;
を持たせる。（ニックネームの変更にも対応しやすい予感。）
ここで、カードは、
文字列で、0〜9（1から10として扱う、つまりAは0で内部保存される。）、JQKの10は、JQKとして扱い保存する。一度に引いたカードは、1文字で保存する。
ここのデータリストを参考にすることで、サイズで、何回引いたかもわかる。
また、このように保存することで、同じカードを引かせないこともできるが（今回はまずは4回しか引かせないので無視）
また役の候補を保存していることで、AとJQKの組み合わせでブラックジャックを達成できるようにする。
また、AとJを引いた時に、確率で、表ブラック・ジャックや、裏ブラック・ジャックを達成させることもできる。（まだやらない。）

次に、もう一度、引数なしで、カードを引かせる。
このとき、以前の結果の文字列を参照する。例えば、0（これはエースの1）が保存された状況で、Kを引くと、21となり、ブラックジャック達成。
もし、なにかしらの役にならないのならば、
BOT : you drew K of Spades [point 11], total point 12
のように、合計点数を返す。
21を超えた場合は、バーストとして、
BOT : you drew Q of Diamonds [point 10], total point 22. BUST
のように返す。
21ちょうどならば、
BOT : you drew A of Clubs [point 1], total point 21. BLACKJACK!
のように返す。
引いた回数が5回を超えた場合は、強制的にバーストさせる。
BOT : you drew 3 of Hearts [point 3], total point 18. You have drawn 5 cards. BUST!
のように返す。 

スタンドは、!sのようにして、引数付きで送信する。
BOT : You chose to stand with total point 18.

例えば、5回引かせないようにすれば、ハートや、スペードのような違いを無視することができる。

もしくは、全種類のカードを保存して、同じカードを引かせないのも手。

*/

void CommandHandler::handleBot(const Message& msg, Client& client)
{
    const std::string& text = msg.trailing; // 例: "!h こんにちは"

    // if (text.size() >= 2 && text[0] == '!' && text[1] == 'h') {
    //     handleHello(msg, client);
    //     return;
    // }
	// !haみたいのを弾けたらいいなあと。
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

	//もし、引数なしなら、デフォルトのハローメッセージを使う。
    if (text.empty())
        text = "Hello! Everyone!";
    std::string botMessage = "BOT: " + text;

    const std::string& channelName = msg.params[0];
    Channel* channel = m_server.findChannel(channelName);
    if (!channel)
	{
		//irssiが、チャンネルを補うので大丈夫なはず。
		sendError(client, "403", channelName, "No such channel");
        return;
	}

    std::string prefix = m_serverName;
    std::vector<std::string> params;
    params.push_back(channelName);

    std::string out = buildMessage(prefix, "PRIVMSG", params, botMessage);

    // チャンネル全員へ配信
    channel->broadcast(out, -1);
}

void CommandHandler::handleOmikuji(const Message& msg, Client& client)
{
    (void)client;
    std::string text = msg.trailing;

    // "!o" を削除
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
    omikujiResults.push_back("Super Lucky");//超吉
    omikujiResults.push_back("Great Lucky");//大吉
    omikujiResults.push_back("Lucky");//吉
    omikujiResults.push_back("Medium Lucky");//中吉
    omikujiResults.push_back("Small Lucky");//末吉
    omikujiResults.push_back("Bad Luck, oh...");//凶

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

    if (nick == "hancho" || nick == "isawa" || nick == "numakawa" || nick == "isinuma" || nick == "otsuki")
    {
        //順番を変えてもいいかも。
        d1 = 4; d2 = 5; d3 = 6;
    }
    else if (nick == "kaiji" || nick == "oohba")
    {
        d1 = 1; d2 = 1; d3 = 1;
    }
    else
    {
        std::srand(time(NULL) + nick[0]);

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

        // 変更前:
        // role = "MEARI（" + std::to_string(eye) + "）";

        // 変更後:
        std::stringstream ss_eye;
        ss_eye << eye; // 数値をストリームに入れる
        role = "MEARI（" + ss_eye.str() + "）"; // .str() で string として取り出す
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
    const std::string& text = msg.trailing;
    const std::string& nick = client.getNickName();

    const std::string& channelName = msg.params[0];
    Channel* channel = m_server.findChannel(channelName);
    if (!channel)
    {
        sendError(client, "403", channelName, "No such channel");
        return;
    }

    //!s
    if (text.rfind("!s", 0) == 0)
    {
        if (client.getBotCardCount() == 0)
        {
            std::string botMessage = "BOT: You have not started Blackjack yet.";
            std::string out = buildMessage(m_serverName, "PRIVMSG",
                                           std::vector<std::string>(1, channelName),
                                           botMessage);
            channel->broadcast(out, -1);
            return;
        }

        int total = client.calcBotBJTotal();
        std::stringstream ss;
        ss << "BOT: You chose to stand with total point " << total << ".";
        std::string botMessage = ss.str();

        std::string out = buildMessage(m_serverName, "PRIVMSG",
                                       std::vector<std::string>(1, channelName),
                                       botMessage);
        channel->broadcast(out, -1);

        client.resetBotCards();
        return;
    }

    //!b が5回目　カードの柄を内部で保存してないので、5回目を超えたら強制バースト
    if (client.getBotCardCount() >= 5)
    {
        std::string botMessage = "BOT: You have drawn 5 cards. BUST!";
        std::string out = buildMessage(m_serverName, "PRIVMSG",
                                       std::vector<std::string>(1, channelName),
                                       botMessage);
        channel->broadcast(out, -1);

        client.resetBotCards();
        return;
    }

    if (client.getBotCardCount() == 0)
    {
        std::string botMessage = "BOT: Welcome to Simple Blackjack! There is no five cards.";
        std::string out = buildMessage(m_serverName, "PRIVMSG",
                                       std::vector<std::string>(1, channelName),
                                       botMessage);
        channel->broadcast(out, -1);
    }

    std::srand(std::time(NULL) + nick[0]);//nickはいらん気がする。なんかバイアスが10と5、6が多い。
    int r = std::rand() % 13;  
    std::string card;
    if (r == 0)
        card = "0";         // Ace
    else if (1 <= r && r <= 9)
    {
        // 変更前:
        // card = std::to_string(r); // 1～9

        // 変更後:
        std::stringstream ss_card;
        ss_card << r;
        card = ss_card.str(); // 1～9
    }else if (r == 10)
        card = "J";
    else if (r == 11)
        card = "Q";
    else if (r == 12)
        card = "K";

    client.addBotCard(card);

    int total = client.calcBotBJTotal();
    int point;
    if (card == "J" || card == "Q" || card == "K")
        point = 10;
    else
    {
        if (card == "0")
            point = 11; //計算と合致してないかも。
        else
            //    point = std::stoi(card);
            point = std::atoi(card.c_str());//std::stringstreamなら、"abc" のような変換できない文字列を 0 として返すことは無いけどめんどくさい。
    }

    std::stringstream ss;
    if (card == "0")
        card = "Ace";
    ss << "BOT: you drew " << card << " [point " << point << "]";

    if (client.getBotCardCount() > 1)
        ss << ", total point " << total;

    if (total == 21)
    {
        ss << ". BLACKJACK!";
        client.resetBotCards();
    }
    else if (total > 21)
    {
        ss << ". BUST";
        client.resetBotCards();
    }
    else if (client.getBotCardCount() >= 5)
    {
        ss << ". You have drawn 5 cards. BUST!";
        client.resetBotCards();
    }

    std::string botMessage = ss.str();

    std::string out = buildMessage(m_serverName, "PRIVMSG",
                                   std::vector<std::string>(1, channelName),
                                   botMessage);
    channel->broadcast(out, -1);
}



/*
    //seedを作る。
    time_t now = time(NULL);
    struct tm* lt = localtime(&now);
    int y = lt->tm_year + 1900;
    int m = lt->tm_mon + 1;
    int d = lt->tm_mday;

    int dateSeed = y * 10000 + m * 100 + d;  // 例：20240210

    //nickからもシードを作る
    long nickSeed = 0;
    const std::string& nick = client.getNickName();
    for (size_t i = 0; i < nick.size(); i++) {
        nickSeed = nickSeed * 131 + nick[i];  //131は適当な素数
    }

    unsigned long finalSeed = dateSeed ^ (nickSeed + 0x9e3779b97f4a7c15ULL);
    std::mt19937 gen(finalSeed);

    std::uniform_int_distribution<> dis(0, omikujiResults.size() - 1);
    std::string result = omikujiResults[dis(gen)];
*/