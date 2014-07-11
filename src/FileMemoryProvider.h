/*
 * FileMemoryProvider.h
 *
 *  Created on: 2014. gada 7. jûl.
 *      Author: Bobby-Z (Robert L. Svarinskis)
 */

#ifndef FILEMEMORYPROVIDER_H_
#define FILEMEMORYPROVIDER_H_

#include "MemoryProvider.h"
#include <fstream>

namespace jason
{

	class FileMemoryProvider : public MemoryProvider
	{
		public:
			FileMemoryProvider(const char * file);
			~FileMemoryProvider();
			void seek(pointer p);
			boolean expand(pointer newSize);
			pointer getPointer();
			pointer getLength();
			unsigned long read(byte& buff, unsigned long off, unsigned long len);
			unsigned long write(byte& buff, unsigned long off, unsigned long len);
		private:
			std::fstream file;
	};

}

#endif /* FILEMEMORYPROVIDER_H_ */
