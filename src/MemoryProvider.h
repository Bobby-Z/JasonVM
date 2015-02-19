/*
 * MemoryProvider.h
 *
 *  Created on: 2014. gada 7. jûl.
 *      Author: Bobby-Z (Robert L. Svarinskis)
 */

#ifndef MEMORYPROVIDER_H_
#define MEMORYPROVIDER_H_

typedef unsigned char byte;

typedef int8_t byte8;
typedef int16_t short16;
typedef int32_t int32;
typedef int64_t long64;

typedef float float32;
typedef double double64;

typedef uint64_t pointer;

#include <string>

namespace jason
{

	struct Pointer
	{
			char * name; //TODO implement hash algorithm
			byte * mask;
			byte * type;
			pointer * loc;
	};

	class MemoryProvider
	{
		public:
			MemoryProvider(pointer size);
			~MemoryProvider();

			pointer getLength();
			bool expand(pointer newSize);

			unsigned long read(pointer loc, byte * buff, unsigned long off, unsigned long len);
			unsigned long write(pointer loc, byte * buff, unsigned long off, unsigned long len);
			unsigned long read(pointer loc, byte * buff, unsigned long len);
			unsigned long write(pointer loc, byte * buff, unsigned long len);

			byte8 read(pointer loc);
			void write(pointer loc, byte8 b);

			short16 readShort(pointer loc);
			void writeShort(pointer loc, short16 s);

			int32 readInt(pointer loc);
			void writeInt(pointer loc, int32 i);

			long64 readLong(pointer loc);
			void writeLong(pointer loc, long64 l);

			std::string readUTF8(pointer loc);
			void writeUTF8(pointer loc, std::string str);

			std::string readUTF16(pointer loc);
			void writeUTF16(pointer loc, std::string str);

			pointer createVariable(pointer size);
			void deleteVariable(pointer p);

			bool searchInVariable(pointer context, const char * c, Pointer & var);
			bool searchForVariable(pointer obj, const char * c, Pointer & var);

			void GC();

			byte * memory;
			pointer length;
		private:
			byte gc = 0;
			void doGC(pointer loc);
			byte * reserved;
	};

}

#endif /* MEMORYPROVIDER_H_ */
