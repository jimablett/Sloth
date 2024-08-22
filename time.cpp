/*
	Thanks to VICE by Bluefever Software
*/
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <windows.h>

#include "time.h"

namespace Sloth {
	int Time::getTimeMs() {
		return GetTickCount64();
	}

	int Time::inputWaiting() {
		#ifndef WIN32
			fd_set readfds;
			struct timeval_tv;
			FD_ZERO(&readfds);
			FD_SET(fileno(stdin), &readfds);
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			select(16, &readfds, 0, 0, &tv);
		#else
		static int init = 0, pipe;
		static HANDLE inh;
		DWORD dw;

		if (!init) {
			init = 1;
			inh = GetStdHandle(STD_INPUT_HANDLE);
			pipe = !GetConsoleMode(inh, &dw);
			if (!pipe) {
				SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
				FlushConsoleInputBuffer(inh);
			}
		}

		if (pipe) {
			if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
			return dw;
		}
		else {
			GetNumberOfConsoleInputEvents(inh, &dw);
			return dw <= 1 ? 0 : dw;
		}
		#endif
	}

	void Time::readInput() {
		int bytes;

		char input[256] = "";

		if (inputWaiting()) {
			stopped = true;

			do {
				std::cin.getline(input, 256);
				bytes = std::cin.gcount();
			} while (bytes < 0);

			if (bytes > 0) {
				char* endc = strchr(input, '\n');

				if (endc) *endc = 0;

				if (std::strncmp(input, "quit", 4) == 0) {
					quit = true;
				}
				else if (std::strncmp(input, "stop", 4) == 0) {
					quit = true;
				}
			}
		}
	}

	void Time::communicate() {
		if (timeSet == 1 && getTimeMs() > stopTime) {
			stopped = true;
		}

		readInput();
	}
}