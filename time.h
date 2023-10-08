/*
	Thanks to VICE by Bluefever Software
*/

#ifndef TIME_H_INCLUDED
#define TIME_H_INCLUDED
namespace Sloth {
	class Time {
	public:
		bool quit = false;
		bool stopped = false;

		int movesToGo = 30;
		int moveTime = -1;
		int time = -1;
		int inc = 0;
		int startTime = 0;
		int stopTime = 0;
		int timeSet = 0;

		int getTimeMs();
		int inputWaiting();
		
		void readInput();
		void communicate();
	};
}
#endif