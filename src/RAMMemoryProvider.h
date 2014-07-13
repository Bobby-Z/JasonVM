/*
 * RAMMemoryProvider.h
 *
 *  Created on: 2014. gada 7. jûl.
 *      Author: Bobby-Z (Robert L. Svarinskis)
 */

#ifndef RAMMEMORYPROVIDER_H_
#define RAMMEMORYPROVIDER_H_

#include "MemoryProvider.h"
#include <stdio.h>
#include <cstdlib>
#include <iostream>

namespace jason
{

	class RAMMemoryProvider : public MemoryProvider
	{
		public:
			RAMMemoryProvider(unsigned long long size);
			~RAMMemoryProvider();
			void seek(pointer p);
			boolean expand(pointer newSize);
			pointer getPointer();
			pointer getLength();
			unsigned long read(byte& buff, unsigned long off, unsigned long len);
			unsigned long write(byte& buff, unsigned long off, unsigned long len);
		private:
			byte * memory;
			byte * stack;
			byte * reserved;
			pointer length;
			pointer currentPointer;
	};

}

#endif /* RAMMEMORYPROVIDER_H_ */
