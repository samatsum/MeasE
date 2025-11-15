/\*
DCC は、IRC メッセージではなくて、
CTCP（CLIENT-TO-CLIENT PROTOCOL）を利用する。

クライアントは、PRIVMSG コマンドで、
PRIVMSG 相手 :<CTCP メッセージ>

CTPT では、DCC の基本的フォーマットにして。
\x01DCC <種類> <引数 1> <引数 2> ...\x01
のような形で並ぶ。

【DCC 　 CHAT 　会話】
\x01DCC CHAT chat 3232235776 5000\x01
この場合、3232235776 は IP アドレス（32 ビット整数表現）で、
5000 はポート番号。
慣習的に、Chat chat はチャットセッションの識別子として使われる。
空白区切りで並ぶ。

これは、相手に、TCP で直接つながるための IP とポートを知らせる。

【DCC 　 SEND 　送信】
\x01DCC SEND filename ip port filesize\x01

【実際の操作　 irssi】
上記を考える必要はなく、実際問題これだけである。
PRIVMSG が適切に送信できてればクライアントが対応してくれるので OK。

irssi を立ち上げてログインする。
【送信側の操作】
/dcc send 相手の nick /path/to/file

【受信側の操作】
/dcc get 相手の nick 　これだけで OK

<!-- /dcc list
/dcc close セッション番号 -->

ファイルは、~/.irssi/files か
ルートとか、カレントディレクトあたりに保存させる。

===== Parsed IRC Message (fd 6) =====
Prefix : (none)
Command : PRIVMSG
Params : [0]=one
Trailing : DCC SEND DCC.cpp ::1 49495 783
Raw line : PRIVMSG one :DCC SEND DCC.cpp ::1 49495 783
==================================================
[fd 6] -> [fd 5] PRIVMSG: DCC SEND DCC.cpp ::1 49495 783

\*/
