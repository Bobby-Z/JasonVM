/*
 * MemoryProvider.h
 *
 *  Created on: 2014. gada 7. jûl.
 *      Author: Bobby-Z (Robert L. Svarinskis)
 */

#ifndef MEMORYPROVIDER_H_
#define MEMORYPROVIDER_H_

typedef unsigned char byte;
typedef unsigned long long int pointer;
typedef bool boolean;

#include <string>

namespace jason
{

	class MemoryProvider
	{
		public:
			MemoryProvider(){gc = 1;};
			virtual ~MemoryProvider();
			virtual void seek(pointer p) = 0;
			virtual pointer getPointer() = 0;
			virtual pointer getLength() = 0;
			virtual boolean expand(pointer newSize) = 0;
			virtual unsigned long read(byte * buff, unsigned long off, unsigned long len) = 0;
			virtual unsigned long write(byte * buff, unsigned long off, unsigned long len) = 0;
			unsigned long read(byte * buff, unsigned long len);
			unsigned long write(byte * buff, unsigned long len);
			byte read();
			void write(byte b);
			unsigned short readShort();
			void writeShort(unsigned short s);
			unsigned long int readInt();
			void writeInt(unsigned long int i);
			unsigned long long readLong();
			void writeLong(unsigned long long l);
			std::string readUTF8();
			void writeUTF8(std::string str);
			std::string readUTF16();
			void writeUTF16(std::string str);

			pointer createVariable(pointer size);
			void deleteVariable(pointer p);
			void GC();
		private:
			byte gc;
			void doGC(pointer loc);
			byte buff[8];
	};

}

#endif /* MEMORYPROVIDER_H_ */
