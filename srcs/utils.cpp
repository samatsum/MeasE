#include "../includes/Utils.hpp"

bool isAcceptablePassword(const std::string &password)
{
	if (password.length() < 4 || password.length() > 16)
		return false;

	int count[256] = {0};

	for (size_t i = 0; i < password.length(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(password[i]);
		count[c]++;

		if (count[c] >= 3)
			return false;
	}
	return true;
}
