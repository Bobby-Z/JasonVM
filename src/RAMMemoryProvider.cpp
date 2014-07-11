/*
 * RAMMemoryProvider.cpp
 *
 *  Created on: 2014. gada 7. jûl.
 *      Author: Bobby-Z (Robert L. Svarinskis)
 */

#include "RAMMemoryProvider.h"

namespace jason
{

	RAMMemoryProvider::RAMMemoryProvider(pointer size) : MemoryProvider()
	{
		reserved = new byte[8192];
		currentPointer = 0;
		length = size;
		memory = new byte[size];
		for (pointer i = 0; i < size; i++)
			memory[i] = 0xf8;
		printf("Created an array of %i bytes", size);
	}

	RAMMemoryProvider::~RAMMemoryProvider()
	{
		delete[] memory;
		delete[] reserved;
	}

	void
	RAMMemoryProvider::seek(pointer p)
	{
		currentPointer = p;
	}

	pointer
	RAMMemoryProvider::getPointer()
	{
		return currentPointer;
	}

	pointer
	RAMMemoryProvider::getLength()
	{
		return length;
	}

	boolean
	RAMMemoryProvider::expand(pointer newSize)
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
	RAMMemoryProvider::read(byte & buff, unsigned long off, unsigned long len)
	{
		for (unsigned int i = 0; i < len; i++)
		{
			(&buff)[i + off] = memory[currentPointer++];
			if (currentPointer >= length) return i;
		}
		return len;
	}

	unsigned long
	RAMMemoryProvider::write(byte & buff, unsigned long off, unsigned long len)
	{
		for (unsigned int i = 0; i < len; i++)
		{
			memory[currentPointer++] = (&buff)[i + off];
			if (currentPointer >= length) return i;
		}
		return len;
	}
}
