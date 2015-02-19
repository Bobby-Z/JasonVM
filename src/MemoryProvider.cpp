#include "MemoryProvider.h"

namespace jason
{
	MemoryProvider::MemoryProvider(pointer size)
	{
		size &= 0x3FFFFFFFFFFFFFFF;
		reserved = new byte[16384];
		length = size;
		memory = new byte[size];
		*((pointer *) memory) = size - 8;
	}

	MemoryProvider::~MemoryProvider()
	{
		delete[] memory;
		delete[] reserved;
	}

	pointer
	MemoryProvider::getLength()
	{
		return length;
	}

	bool
	MemoryProvider::expand(pointer newSize)
	{
		byte * tmpdata;
		try
		{
			tmpdata = new byte[newSize];
		} catch (std::bad_alloc & er)
		{
			delete[] reserved;
			delete[] tmpdata;
			std::cout << er.what() << std::endl;
			return false;
		}
		std::copy(memory + 0, memory + length, tmpdata);
		delete[] memory;
		memory = tmpdata;
		return true;
	}

	unsigned long
	MemoryProvider::read(pointer loc, byte * buff, unsigned long off, unsigned long len)
	{
		for (unsigned int i = 0; i < len; i++)
		{
			buff[i + off] = memory[loc++];
			if (loc >= length) return i;
		}
		return len;
	}

	unsigned long
	MemoryProvider::write(pointer loc, byte * buff, unsigned long off, unsigned long len)
	{
		for (unsigned int i = 0; i < len; i++)
		{
			memory[loc++] = buff[i + off];
			if (loc >= length) return i;
		}
		return len;
	}

	unsigned long
	MemoryProvider::read(pointer loc, byte * buf, unsigned long len)
	{
		return read(loc, buf, 0, len);
	}

	byte8
	MemoryProvider::read(pointer loc)
	{
		return memory[loc];
	}

	short16
	MemoryProvider::readShort(pointer loc)
	{
		short16 s = *((short16 *) &memory[loc]);
		return s;
	}

	int32
	MemoryProvider::readInt(pointer loc)
	{
		int32 i = *((int32 *) &memory[loc]);
		return i;
	}

	long64
	MemoryProvider::readLong(pointer loc)
	{
		long64 l = *((long64 *) &memory[loc]);
		return l;
	}

	std::string
	MemoryProvider::readUTF8(pointer loc)
	{
		unsigned short len = readShort(loc);
		byte * retval = new byte[len];
		read(loc + 2, retval, len);
		std::string ret((const char*)retval);
		delete [] retval;
		return ret;
	}

	std::string
	MemoryProvider::readUTF16(pointer loc)
	{
		return readUTF8(loc);
	}

	unsigned long
	MemoryProvider::write(pointer loc, byte * buff, unsigned long len)
	{
		return write(loc, buff, 0, len);
	}

	void
	MemoryProvider::write(pointer loc, byte8 b)
	{
		memory[loc] = b;
	}

	void
	MemoryProvider::writeShort(pointer loc, short16 s)
	{
		*((short16 *) &memory[loc]) = s;
	}

	void
	MemoryProvider::writeInt(pointer loc, int32 i)
	{
		*((int32 *) &memory[loc]) = i;
	}

	void
	MemoryProvider::writeLong(pointer loc, long64 l)
	{
		*((long64 *) &memory[loc]) = l;
	}

	void
	MemoryProvider::writeUTF8(pointer loc, std::string s)
	{
		writeShort(loc, (unsigned short int) s.length());
		byte * bytes = (byte*) s.data();
		write(loc + 2, bytes, (unsigned long int) s.length());
	}

	void
	MemoryProvider::writeUTF16(pointer loc, std::string s)
	{
		writeUTF8(loc, s);
	}

	pointer
	MemoryProvider::createVariable(pointer size)
	{
		pointer location = 0;
		while (location < getLength())
		{
			printf("Scanning variable %d\n", location);
			pointer l = readLong(location);
			location += 8;
			if (l & 0xC000000000000000 != 0)
			{
				printf("Variable is used...\n");
				location += (l & 0x3FFFFFFFFFFFFFFF);
				continue;
			}
			l &= 0x3FFFFFFFFFFFFFFF;
			printf("Variable isn't used\n");
			if (size <= l - 8)
				writeLong(location + l, (long64) (l - size - 8));
			else
			{
				location += l;
				continue;
			}
			writeLong(location - 8, ((pointer) gc << 62) | size);
			return location - 8;
		}
		return getLength();
	}

	void
	MemoryProvider::deleteVariable(pointer p)
	{
		pointer length = readLong(p) & 0x3FFFFFFFFFFFFFFF;
		writeLong(p, length);
	}

	bool
	MemoryProvider::searchInVariable(pointer context, const char * name, Pointer & var)
	{
		pointer length = readLong(context) & 0x3FFFFFFFFFFFFFFF;
		pointer supertype = readLong(context + 8) & 0x3FFFFFFFFFFFFFFF;

		char c;
		unsigned long int parsed = 24;

		a: while (parsed < length)
		{
			int i = 0;
			while ((c = memory[context + parsed++]) != 0)
			{
				if (name[i] == 0)
					a: continue;
				if (c != name[i++])
					a: continue;
			}
			if (name[i] != 0)
				a: continue;
			var.mask = memory[context + parsed++];
			if ((var.mask & 3) != 2)
				a: continue;
			pointer p = readLong(context + parsed);
			parsed += 8;
			var.type = (byte) (p >> 59 & 0xFF);
			var.loc = p & 0x7FFFFFFFFFFFFFF;
			var.name = name;
			return true;
		}

		if (supertype != context)
			return searchInVariable(supertype, name, var);
		else
		{
			var.type = 1;
			return false;
		}
	}

	bool
	MemoryProvider::searchForVariable(pointer context, const char * name, Pointer & var)
	{
		pointer parent = readLong(context + 16) & 0x7FFFFFFFFFFFFFFF;

		if (searchInVariable(context, name, var))
			return true;
		else if (parent != context)
			return searchForVariable(parent, name, var);
		else
			return false;
	}

	void
	MemoryProvider::doGC(pointer loc)
	{
		gc += 0x40;
		if (gc == 0)
			gc = 0x40;
		while (loc < getLength())
		{
			long64 l = readLong(loc);
			loc += 8;
			//GC
		}
	}

	void
	MemoryProvider::GC()
	{
		doGC(0);
	}
}
