/*
 * FileMemoryProvider.cpp
 *
 *  Created on: 2014. gada 7. jûl.
 *      Author: Bobby-Z (Robert L. Svarinskis)
 */

#include <iostream>
#include "FileMemoryProvider.h"

namespace jason
{

	FileMemoryProvider::FileMemoryProvider(const char * fileLocation) : MemoryProvider()
	{
		file.open(fileLocation, (std::ios::openmode) (std::ios::binary | std::ios::out | std::ios::in));
		if (!file.is_open())
		{
			std::cerr << "Could not open RAM file!\n";
		} else
		{
			//file.write((byte) ((size >> 56) & 0x3F));
			//memory[1] = (byte) ((size >> 48) & 0xFF);
			//memory[2] = (byte) ((size >> 40) & 0xFF);
			//memory[3] = (byte) ((size >> 32) & 0xFF);
			//memory[4] = (byte) ((size >> 24) & 0xFF);
			//memory[5] = (byte) ((size >> 16) & 0xFF);
			//memory[6] = (byte) ((size >> 8) & 0xFF);
			//memory[7] = (byte) ((size) & 0xFF);
		}
	}

	FileMemoryProvider::~FileMemoryProvider()
	{
		file.close();
	}

	void
	FileMemoryProvider::seek(pointer p)
	{
		file.seekg(p, std::ios::beg);
		file.seekp(p, std::ios::beg);
	}

	pointer
	FileMemoryProvider::getPointer()
	{
		return file.tellp();
	}

	pointer
	FileMemoryProvider::getLength()
	{
		pointer p = file.tellg();
		file.seekg(0, std::ios::end);
		pointer len = file.tellg();
		file.seekg(p);
		return len + 1;
	}

	boolean
	FileMemoryProvider::expand(pointer newSize)
	{
		pointer p = file.tellp();
		file.seekp(newSize);
		const char * c = new char[1];
		file.write(c, 1);
		delete c;
		file.seekp(p);
		return true;
	}

	unsigned long
	FileMemoryProvider::read(byte & buff, unsigned long off, unsigned long len)
	{
		file.read((char*) &buff, len);
		return len;
	}

	unsigned long
	FileMemoryProvider::write(byte & buff, unsigned long off, unsigned long len)
	{
		file.write((char*) &buff, len);
		return len;
	}

}
