/*
 * FileMemoryProvider.cpp
 *
 *  Created on: 2014. gada 7. jûl.
 *      Author: Bobby-Z (Robert L. Svarinskis)
 */

#include "FileMemoryProvider.h"

namespace jason
{

	FileMemoryProvider::FileMemoryProvider(const char * fileLocation) : MemoryProvider()
	{
		file.open(fileLocation, (std::ios::openmode) (std::ios::binary | std::ios::out | std::ios::in));
		if (!file.is_open())
		{
			std::cerr << "Could not open RAM file!\n";
		}
	}

	FileMemoryProvider::~FileMemoryProvider()
	{
		file.close();
	}

	void
	FileMemoryProvider::seek(pointer p)
	{
		file.seekg(p);
		file.seekp(p);
	}

	pointer
	FileMemoryProvider::getPointer()
	{
		return file.tellp();
	}

	pointer
	FileMemoryProvider::getLength()
	{
		pointer * p = new pointer;
		*p = file.tellg();
		file.seekg(0, std::ios::end);
		pointer len = file.tellg();
		file.seekg(*p);
		delete p;
		return len;
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
