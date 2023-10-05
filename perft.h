#ifndef PERFT_H
#define PERFT_H_INCLUDED

#include "position.h"
#include <windows.h>


namespace Sloth {
	namespace Perft {
		static int getTimeMs() {
			return GetTickCount();
		}

		extern long nodes;

		extern inline void perft(int depth, Position& pos);

		void perftTest(int depth, Position& pos);
	}
}

#endif // !PERFT_H