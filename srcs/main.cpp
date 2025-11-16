#include <cstdlib> //for std::strtol
#include <cerrno>  //for errno
#include <iostream> //for std::cout, std::cerr
#include <csignal> //for sigaction, sigemptyset, sigaddset, sigprocmask
#include <cstring> // for std::memset
#include <stdexcept> //for std::runtime_error
#include "../includes/IrcServer.hpp"
#include "../includes/Utils.hpp"

volatile sig_atomic_t g_signalCaught = 0;

static bool checkArgs(int argc, char **argv, int &port, std::string &passWord);
static void	SetUpSignalHandlers(struct sigaction &sa);

int	main(int argc, char **argv)
{
	int					port;
	std::string			passWord;
	struct sigaction	sa;
	
	std::memset(&sa, 0, sizeof(sa));
	if (!checkArgs(argc, argv, port, passWord))
		return 1;

	try 
	{
		SetUpSignalHandlers(sa);

		IrcServer			server(port, passWord);

		server.setUpSocket();

		std::cout << "Listening on port " << port << std::endl;
		server.run();

	}
	catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	
	std::cout << "Server closed normally!" << std::endl;
	return 0;
}


static bool checkArgs(int argc, char **argv, int &port, std::string &passWord)
{
	char	*endptr;
	long	val;

	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return (false);
	}

	errno = 0;
	val = std::strtol(argv[1], &endptr, 10);
	if (errno != 0 || *endptr != '\0' || val < 1024 || val > 49151)
	{
		std::cerr << "Error: A port number between 1024 and 49151 is required." << std::endl;
		return (false);
	}

	port = static_cast<int>(val);
	passWord = argv[2];
	if (!isAcceptablePassword(passWord)) {
		std::cerr << "Error: Password must be 4-16 characters long with no character repeated 3 or more times." << std::endl;
		return (false);
	}
	return (true);
}

static void sigHandler(int signo)
{
    g_signalCaught = signo;
}

static void SetUpSignalHandlers(struct sigaction &sa)
{

	sa.sa_handler = sigHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sa, NULL))
		throw(std::runtime_error("Error: Failed to set signal handler for SIGINT"));
	if (sigaction(SIGQUIT, &sa, NULL))
		throw(std::runtime_error("Error: Failed to set signal handler for SIGQUIT"));
}

