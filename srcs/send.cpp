#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

void safeSend(int fd, const char* buffer, size_t length) {

	size_t totalSent = 0;
	size_t retryCount = 0;

	while (totalSent < length) {

		ssize_t bytesSent = send(fd, buffer + totalSent, length - totalSent, 0);
		if (bytesSent < 0) {
			if (errno == EINTR) {
				continue; // シグナル割り込みの場合は再試行
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// ソケットが一時的に書き込み不可の場合、少し待ってから再試行
				usleep(1000);
				++retryCount;
				if (retryCount > 3) {
					std::cerr << "Send failed after multiple retries: " << std::strerror(errno) << std::endl;
					return; // 複数回のリトライ後に失敗した場合は終了
				}
				continue;
			} else {
				std::cerr << "Send failed: " << std::strerror(errno) << std::endl;
				return; // エラー発生時は終了
			}
		}
		totalSent += bytesSent;
	}
}
