#include "MemoryProvider.h"

namespace jason
{
	MemoryProvider::~MemoryProvider()
	{
	}

	void
	MemoryProvider::seek(pointer p)
	{
	}

	pointer
	MemoryProvider::getPointer()
	{
		return 0;
	}

	pointer
	MemoryProvider::getLength()
	{
		return 0;
	}

	boolean
	MemoryProvider::expand(pointer newSize)
	{
		return false;
	}

	unsigned long
	MemoryProvider::read(byte & buff, unsigned long off, unsigned long len)
	{
		return 0;
	}

	unsigned long
	MemoryProvider::write(byte & buff, unsigned long off, unsigned long len)
	{
		return 0;
	}

	unsigned long
	MemoryProvider::read(byte& buf, unsigned long len)
	{
		return read(buf, 0, len);
	}

	byte
	MemoryProvider::read()
	{
		read(*buff, 1);
		return buff[0];
	}

	unsigned short
	MemoryProvider::readShort()
	{
		read(*buff, 2);
		return buff[0] << 8 | buff[1];
	}

	unsigned long int
	MemoryProvider::readInt()
	{
		read(*buff, 4);
		return buff[0] << 24 | buff[1] << 16 | buff[2] << 8 | buff[3];
	}

	unsigned long long
	MemoryProvider::readLong()
	{
		read(*buff, 8);
		return ((unsigned long long) buff[0]) << 56 | ((unsigned long long) buff[1]) << 48 | ((unsigned long long) buff[2]) << 40 | ((unsigned long long) buff[3]) << 32 | buff[4] << 24 | buff[5] << 16 | buff[6] << 8 | buff[7];
	}

	std::string
	MemoryProvider::readUTF8()
	{
		unsigned short len = readShort();
		byte * retval = new byte[len];
		read(*retval, len);
		std::string ret((const char*)retval);
		return ret;
	}

	std::string
	MemoryProvider::readUTF16()
	{
		return readUTF8();
	}

	unsigned long
	MemoryProvider::write(byte& buff, unsigned long len)
	{
		return MemoryProvider::write(buff, 0, len);
	}

	void
	MemoryProvider::write(byte b)
	{
		buff[0] = b;
		write(*buff, 1);
	}

	void
	MemoryProvider::writeShort(unsigned short int s)
	{
		buff[0] = (s >> 8) & 0xFF;
		buff[1] = s & 0xFF;
		write(*buff, 2);
	}

	void
	MemoryProvider::writeInt(unsigned long int i)
	{
		buff[0] = (i >> 24) & 0xFF;
		buff[1] = (i >> 16) & 0xFF;
		buff[2] = (i >> 8) & 0xFF;
		buff[3] = i & 0xFF;
		write(*buff, 4);
	}

	void
	MemoryProvider::writeLong(unsigned long long l)
	{
		buff[0] = (l >> 56) & 0xFF;
		buff[1] = (l >> 48) & 0xFF;
		buff[2] = (l >> 40) & 0xFF;
		buff[3] = (l >> 32) & 0xFF;
		buff[4] = (l >> 24) & 0xFF;
		buff[5] = (l >> 16) & 0xFF;
		buff[6] = (l >> 8) & 0xFF;
		buff[7] = l & 0xFF;
		write(*buff, 8);
	}

	void
	MemoryProvider::writeUTF8(std::string s)
	{
		writeShort((unsigned short int) s.length());
		byte * bytes = (byte*) s.data();
		write(*bytes, (unsigned long int) s.length());
	}

	void
	MemoryProvider::writeUTF16(std::string s)
	{
		writeUTF8(s);
	}

	pointer
	MemoryProvider::createVariable(pointer size)
	{
		pointer b4 = getPointer();
		while (getPointer() < getLength())
		{
			if (read() == 0)
			{
				seek(getPointer() - 1);
				continue;
			}
			seek(getPointer() - 1);
			pointer l = readLong();
			if (l & 0xC000000000000000 != 0)
			{
				seek(getPointer() + (l & 0x3FFFFFFFFFFFFFFF));
				continue;
			}
			if (l & 0x3FFFFFFFFFFFFFFF > size)
			{
				seek(getPointer() + (l & 0x3FFFFFFFFFFFFFFF));
				continue;
			}
			pointer p = getPointer() - 1;
			if (size < l)
			{
				seek(getPointer() + size);
				byte * b = new byte[l - size];
				write(*b, l - size);
			}
			seek(p);
			writeLong(((pointer) gc << 56) | size);
			return p;
		}
		seek(b4);
		return getLength();
	}

	void
	MemoryProvider::deleteVariable(pointer p)
	{
		pointer b4 = getPointer();
		seek(p);
		unsigned long long int length = readLong() & 0x3FFFFFFFFFFFFFFF;
		seek(p);
		writeLong(length);
		seek(b4);
	}

	void
	MemoryProvider::doGC(pointer loc)
	{
		gc += 0x40;
		if (gc == 0)
			gc = 0x40;
		seek(loc);
		while (getPointer() < getLength())
		{
			unsigned long long l = readLong();
			//GC
		}
	}

	void
	MemoryProvider::GC()
	{
		pointer p = getPointer();
		doGC(0);
		seek(p);
	}
}
