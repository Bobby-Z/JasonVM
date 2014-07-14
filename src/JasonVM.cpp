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
#include <string>

#include "RAMMemoryProvider.h"
#include "FileMemoryProvider.h"
#include <fstream>

namespace jason
{
	struct MachineArgs
	{
			byte memprovider;
			std::string memproviderargs;
			int argc;
			const char** arguments;
	};

	unsigned long long
	MemToBytes(const char* mem, signed int len)
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

	MemoryProvider * RAM;

	void LoadScript(const char * file, const char * includedFrom, const unsigned long long * argumentPointers);
	void ParseHeader(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column);
	void ParseBrackets(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column);
	void ParseVariable(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column);

	void
	LoadScript(const char * file, const char * includedFrom, const unsigned long long * argumentPointers)
	{
		std::string * str = new std::string("");
		std::ifstream * i = new std::ifstream(file);
		if (i->is_open())
		{
			std::string line;
			while (i->good())
			{
				getline(*i, line);
				str->append(line);
				str->append("\n");
			}
			i->close();
			unsigned long int * p = new unsigned long int(0);
			unsigned int * line_num = new unsigned int(1);
			unsigned int * column = new unsigned int(0);
			ParseHeader(p, *str, line_num, column);
			ParseVariable(p, *str, line_num, column);
		} else
			printf("Could not open file %s included from %s\n", file, includedFrom);
		i->close();
		delete str;
		delete i;
	}

	void
	ParseHeader(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string includeloc)
	{
		while (*p < str.length())
		{
			char c = str.data()[*p++];
			*column++;
			if (c == ' ' || c == '	')
				continue;
			else if (c == '\n')
			{
				*line++;
				*column = 0;
				continue;
			} else if (c == '#')
			{
				std::string * hashtype = new std::string("");
				while (*p < str.length())
				{
					char c2 = str.data()[*p++];
					*column++;
					if (c == ' ' || c == '	')
						break;
					else if (c == '\n')
					{
						*column = 0;
						*line++;
						break;
					} else if (c == '<' || c == '"')
					{
						*column--;
						*p--;
						break;
					}
					hashtype->append(&c2);
				}
				if (*hashtype == std::string("include"))
				{
					char closingType = ' ';
					std::string * includefile = new std::string("");
					while (*p < str.length())
					{
						char c2 = str.data()[*p++];
						*column++;
						if ((c2 == ' ' || c2 == '	') && closingType != ' ')
							continue;
						else if (c2 == '\n' && closingType != ' ')
						{
							*column = 0;
							*line++;
							continue;
						} else if (c2 == '<' && closingType == ' ')
							closingType = '>';
						else if (c2 == '"' && closingType == ' ')
							closingType = '"';
						else if (c2 == closingType && closingType != ' ')
							break;
						else
							includefile->append(&c2);
					}
					//LoadScript(includefile->data());
					delete includefile;
				} else
					printf("Note: unknown token '#%s' at line %i column %i\n", hashtype->data(), *line, *column - hashtype->length() - 1);
				delete hashtype;
			} else if (c == '{')
			{
				*p--;
				*column--;
				return;
			} else
				printf("Note: invalid token '%c' at line %i column %i\n", c, *line, *column);
		}
	}

	void
	ParseVariable(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column)
	{
		std::string * variablename = new std::string("");
		unsigned char varmask = 0;
		while (*p < str.length())
		{
			char c = str.data()[*p++];
			*column++;
			if (c == ' ' || c == '	')
				continue;
			else if (c == '\n')
			{
				*line++;
				*column = 0;
				continue;
			} else if (c == '"')
			{
				while (*p < str.length())
				{
					char c2 = str.data()[*p++];
					*column++;
					if (c == '\n')
					{
						*line++;
						*column = 0;
						continue;
					} else if (c == '"')
					{

					}
				}
			}
		}// else if (c == '{')
		{
			*column--;
			*p--;
		}
	}

	void
	ParseBrackets(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column)
	{

	}

	int
	Run(MachineArgs arg)
	{
		switch (arg.memprovider)
		{
			case 0:
				RAM = new RAMMemoryProvider(MemToBytes(arg.memproviderargs.data(), arg.memproviderargs.length() - 1));
				break;
			case 1:
				RAM = new FileMemoryProvider(arg.memproviderargs.data());
				break;
		}
		unsigned long long int args[5];
		LoadScript("Library/System.jll", "JasonVM (Native program)", args);
		for (int i = 0; i < 65536; i++)
			printf("File length: %d\n", RAM->getLength());
		return 0;
	}
}

byte
GetStringArgument(const char* arg)
{
	if (arg == std::string("-memprovider")) return 0x00;
	else if (arg == std::string("-version")) return 0x01;
	else return 0xFF;
}

int
main(const int argc, const char** argv)
{
	if (argc == 1)
	{
		std::cout << "JasonVM version 1.0.0_1" << std::endl;
		std::cout << "Jason specification 1" << std::endl;
		std::cout << std::endl;
		std::cout << "Usage: JasonVM [options] file [arguments]" << std::endl;
		std::cout << std::endl;
		std::cout << "The available options are:" << std::endl;
		std::cout
				<< "	-memprovider <file/ram> <location/size>:		choose the VRAM provider, or where the VM stores its RAM"
				<< std::endl;
		std::cout
				<< "								if the provider is chosen to be file, then you must specify the file location"
				<< std::endl;
		std::cout << "								else you must specify the size of the VRAM"
				<< std::endl;
		std::cout << std::endl;
		std::cout << "The options must be in lowercase." << std::endl;
		std::cout << std::endl;
		return 0;
	}
	jason::MachineArgs machine = { 0, std::string("1MB"), argc, argv };
	for (int i = 1; i < argc; i++)
	{
		switch (GetStringArgument(argv[i]))
		{
			case 0x00:
				if (i + 2 < argc)
				{
					i++;
					if (argv[i] == std::string("file"))
					{
						machine.memprovider = 1;
						machine.memproviderargs = argv[++i];
					} else if (argv[i] == std::string("ram"))
					{
						machine.memprovider = 0;
						machine.memproviderargs = argv[++i];
					} else
					{
						std::cout << "Correct usage:" << std::endl;
						std::cout << "	-memprovider <file/ram> <location/size>"
								<< std::endl;
						return 2;
					}
				} else
				{
					std::cout << "Correct usage:" << std::endl;
					std::cout << "	-memprovider <file/ram> <location/size>"
							<< std::endl;
					return 2;
				}
				break;
			case 0x01:
				std::cout << "JasonVM version 1.0.0_1" << std::endl;
				std::cout << "Jason specification 1" << std::endl;
				break;
		}
	}
	return jason::Run(machine);
}
