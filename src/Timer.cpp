#include <time.h>

class Timer {
	private:
		unsigned long begTime;
	public:
        Timer(void) {
            begTime = 0;
        }
		void start() {
			begTime = clock();
		}

        void restart() {
            start();
        }

		int elapsedTime() {
			return (int)((((float) clock() - begTime) / (float)CLOCKS_PER_SEC) * 1000);
		}

		bool isTimeout(unsigned long seconds) {
			return seconds >= elapsedTime();
		}
};