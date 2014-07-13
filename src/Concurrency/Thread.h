/*
 * Thread.h
 *
 *  Created on: 2014. gada 13. jûl.
 *      Author: Bobby-Z (Robert L. Svarinskis)
 */

#ifndef THREAD_H_
#define THREAD_H_

namespace jason
{

	class Thread
	{
		public:
			Thread();
			~Thread();
			void start();
			void pause();
			void resume();
			void join();
			void interrupt();
			void stop();
			boolean isRunning();
			boolean hasStopped();
		private:
			unsigned char status_mask;
	};

} /* namespace jason */

#endif /* THREAD_H_ */
