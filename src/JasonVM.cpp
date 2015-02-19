//============================================================================
// Name        : JasonVM.cpp
// Author      : Bobby-Z (Robert L. Svarinskis)
// Version     :
// Copyright   : (c) BobShizzler Inc.
// Description : Jason virtual machine
//============================================================================

#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <string.h>

#include "Compiler.h"
#include <fstream>
#include <sstream>
#include <vector>

namespace jason
{
	MemoryProvider * RAM;

	struct MachineArgs
	{
			std::string ramamount;
			int argc;
			const char** arguments;
	};

	unsigned long long
	MemoryUnitToBytes(const char* mem, signed int len)
	{
		if (len < 0)
		{
			printf("\"%s\" is not a valid memory size\n", mem);
			std::exit(2);
		}
		unsigned char c;
		char memtype = 0;
		unsigned long long value = 0;
		c = mem[len--];
		if (c == 'B' || c == 'b')
		{
			if (len < 1)
			{
				printf("\"%s\" is not a valid memory size\n", mem);
				std::exit(2);
			}
			c = mem[len--];
			switch (c)
			{
				case 'K':
				case 'k':
					memtype = 'K';
					break;
				case 'M':
				case 'm':
					memtype = 'M';
					break;
				case 'G':
				case 'g':
					memtype = 'G';
					break;
				case 'T':
				case 't':
					memtype = 'T';
					break;
				case 'P':
				case 'p':
					memtype = 'P';
					break;
				case 'E':
				case 'e':
					memtype = 'E';
					break;
				default:
					printf("\"%s\" is not a valid memory size: \"%cB\" is not a memory unit\n", mem, c);
					std::exit(2);
			}
		} else
		{
			memtype = 'B';
			len++;
		}
		unsigned int multiplier = 1;
		while (len >= 0)
		{
			c = mem[len--];
			unsigned char i = 0;
			if (c > 47 && c < 58)
			{
				i = c - 48;
				value += i * multiplier;
				multiplier *= 10;
			} else
			{
				printf("%s is not a valid memory size: '%c' is not a digit\n", mem, c);
				std::exit(2);
			}
		}
		switch (memtype)
		{
			case 'E':
				value *= 1024;
				/* no break */
			case 'P':
				value *= 1024;
				/* no break */
			case 'T':
				value *= 1024;
				/* no break */
			case 'G':
				value *= 1024;
				/* no break */
			case 'M':
				value *= 1024;
				/* no break */
			case 'K':
				value *= 1024;
				/* no break */
			default:
				break;
		}
		if (value == 0)
		{
			printf("%s is not a valid memory size: either too large, or too small\n", mem);
			std::exit(2);
		}
		return value;
	}

	int
	run(MachineArgs arg)
	{
		RAM = new MemoryProvider(MemoryUnitToBytes(arg.ramamount.c_str(), arg.ramamount.length() - 1));
		std::string * str = new std::string(" JasonVM");
		LoadScript("D:/JasonVM/Library/System.jll", str);

		//while (RAM->getPointer() < RAM->getLength()) //Memory stats
		//{
		//	pointer p = RAM->getPointer();
		//	pointer l = RAM->readLong();
		//	byte gc = l >> 62 & 0x03;
		//	printf("Variable\n\tPointer: %d\n\tGCStatus: %d\n\tLength: %d\n", p, gc, l & 0x3FFFFFFFFFFFFFFF);
		//}

		//LoadScript("", str);
		delete str;
		return 0;
	}
}

byte
GetStringArgument(const char* arg)
{
	if (arg == std::string("--ram")) return 0x00;
	else if (arg == std::string("--version") || arg == std::string("-v")) return 0xFD;
	else if (arg == std::string("--help") || arg == std::string("-h")) return 0xFE;
	else return 0xFF;
}

int
PrintHelp()
{
	std::cout <<
	"JasonVM version 0.0.1_2\n\
Jason specification #1 (partially supported)\n\
\n\
Usage: JasonVM [options] file [arguments]\n\
\n\
The available options are:\n\
	--ram			<size>				Select the size of RAM memory to use\n\
\n\
	--version -v						Display the current version of JasonVM\n\
\n\
	--help -h						Display this message\n\
\n\
The options must be provided in lowercase.\n";
	return 0;
}

int
main(const int argc, const char** argv)
{
	if (argc == 1)
		return PrintHelp();
	jason::MachineArgs machine = { std::string("1MB"), argc, argv };
	for (int i = 1; i < argc; i++)
	{
		switch (GetStringArgument(argv[i]))
		{
			case 0x00:
				if (i + 1 < argc)
				{
					machine.ramamount = argv[++i];
				} else
				{
					std::cout << "Correct usage:" << std::endl;
					std::cout << "	--ram <size>" << std::endl;
					return 2;
				}
				break;
			case 0xFD:
				std::cout << "JasonVM version 0.0.1_2" << std::endl;
				std::cout << "Jason specification #1 (partially supported)" << std::endl;
				break;
			case 0xFE:
				PrintHelp();
				break;
			default:
				//TODO end the for loop, and then parse the file name and arguments
				break;
		}
	}
	return jason::run(machine);
}
