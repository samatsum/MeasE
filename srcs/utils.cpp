#include "../includes/Utils.hpp"

bool isAcceptablePassword(const std::string &password)
{
	if (password.length() < 4 || password.length() > 16)
		return false;

	int consecutiveCount = 1;
	for (std::size_t i = 1; i < password.length(); ++i) {
		if (password[i] == password[i - 1]) {
			++consecutiveCount;
			if (consecutiveCount >= 3)
				return false;
		} else {
			consecutiveCount = 1;
		}
	}

	return true;
}
