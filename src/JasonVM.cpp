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
#include <sstream>

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

	std::string
	replaceAllInString(std::string& str, const std::string& from, const std::string& to)
	{
		if(from.empty())
			return str;
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos)
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
		return str;
	}

	MemoryProvider * RAM;

	void LoadScript(const char * file, std::string * includedFrom);
	void ParseHeader(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom);
	bool ParseName(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::stringstream * variablename, byte * mask, byte * type);
	pointer ParseValue(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte * type);

	void
	LoadScript(const char * file, std::string * includedFrom)
	{
		std::string * str = new std::string;
		std::ifstream * i = new std::ifstream(file);
		if (i->is_open())
		{
			std::string line;
			while (i->good())
			{
				getline(*i, line);
				*str += line;
				*str += "\n";
			}
			i->close();
			delete i;
			unsigned long int * p = new unsigned long int(0);
			unsigned int * line_num = new unsigned int(1);
			unsigned int * column = new unsigned int(0);

			std::string * includedFromNew = new std::string("\"");
			*includedFromNew += file;
			*includedFromNew += "\"\n<[\\spaces/]>included from ";
			*includedFromNew += includedFrom->data();

			byte type = 17;
			ParseHeader(p, *str, line_num, column, includedFromNew);
			ParseValue(p, *str, line_num, column, includedFromNew, &type);

			delete str;
			delete p;
			delete line_num;
			delete column;
			delete includedFromNew;
		} else
		{
			std::cout << std::endl;
			std::string * msg = new std::string();
			*msg = "Error: Could not open file '";
			*msg += file;
			*msg += "' ";
			std::cout << msg->data();
			std::cout << "included from ";
			char * c = new char[msg->length() + 1];
			for (int charPos = 0; charPos < msg->length(); charPos++)
				c[charPos] = ' ';
			c[msg->length()] = 0;
			std::string * includedFromFormatted = new std::string(*includedFrom);
			replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
			std::cout << *includedFromFormatted << std::endl;
			delete msg;
			delete c;
			delete includedFromFormatted;
			delete i;
			delete str;
		}
	}

	void
	ParseHeader(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom)
	{
		bool skipErrors = false;
		while (true)
		{
			if (*p >= str.length())
			{
				std::cout << std::endl;
				std::string * msg = new std::string();
				*msg = "Error: Unexpected EOF in ";
				std::cout << msg->data();
				char * c = new char[msg->length() - 13];
				for (int charPos = 0; charPos < msg->length() - 13; charPos++)
					c[charPos] = ' ';
				c[msg->length() - 14] = 0;
				std::string * includedFromFormatted = new std::string(*includedFrom);
				replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
				std::cout << *includedFromFormatted << std::endl;
				delete msg;
				delete [] c;
				delete includedFromFormatted;
				return;
			}
			char c = str.data()[(*p)++];
			(*column)++;
			if (c == ' ' || c == '	')
				continue;
			else if (c == '\n')
			{
				(*line)++;
				(*column) = 0;
				continue;
			} else if (c == '#')
			{
				skipErrors = false;
				std::string * hashtype = new std::string();
				while (true)
				{
					if (*p >= str.length())
					{
						std::cout << std::endl;
						std::string * msg = new std::string();
						*msg = "Error: Unexpected EOF in ";
						std::cout << msg->data();
						char * c = new char[msg->length() - 13];
						for (int charPos = 0; charPos < msg->length() - 13; charPos++)
							c[charPos] = ' ';
						c[msg->length() - 14] = 0;
						std::string * includedFromFormatted = new std::string(*includedFrom);
						replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
						std::cout << *includedFromFormatted << std::endl;
						delete msg;
						delete [] c;
						delete includedFromFormatted;
						return;
					}
					char c2 = str.data()[(*p)++];
					(*column)++;
					if (c2 == ' ' || c2 == '	')
						break;
					else if (c2 == '\n')
					{
						(*column) = 0;
						(*line)++;
						break;
					} else if (c2 == '<' || c2 == '"')
					{
						(*column)--;
						(*p)--;
						break;
					}
					*hashtype += c2;
				}
				if (*hashtype == "include")
				{
					char closingType = ' ';
					std::string * includefile = new std::string();
					while (true)
					{
						if (*p >= str.length())
						{
							std::cout << std::endl;
							std::string * msg = new std::string();
							*msg = "Error: Unexpected EOF in ";
							std::cout << msg->data();
							char * c = new char[msg->length() - 13];
							for (int charPos = 0; charPos < msg->length() - 13; charPos++)
								c[charPos] = ' ';
							c[msg->length() - 14] = 0;
							std::string * includedFromFormatted = new std::string(*includedFrom);
							replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
							std::cout << *includedFromFormatted << std::endl;
							delete msg;
							delete [] c;
							delete includedFromFormatted;
							return;
						}
						char c2 = str.data()[(*p)++];
						(*column)++;
						if ((c2 == ' ' || c2 == '	') && closingType != ' ')
							continue;
						else if (c2 == '\n' && closingType != ' ')
						{
							(*column) = 0;
							(*line)++;
							continue;
						} else if (c2 == '<' && closingType == ' ')
							closingType = '>';
						else if (c2 == '"' && closingType == ' ')
							closingType = '"';
						else if (c2 == closingType && closingType != ' ')
							break;
						else
							*includefile += c2;
					}
					LoadScript(includefile->data(), includedFrom);
					delete includefile;
				} else
				{
					std::cout << std::endl;
					std::string * msg = new std::string();
					std::stringstream * numconv = new std::stringstream();
					std::string * numbers = new std::string();
					*msg = "Warning: Unexpected token '#";
					*msg += *hashtype;
					*msg += "' at line ";
					*numconv << *line;
					*numconv << " column ";
					*numconv << *column;
					*numconv >> *numbers;
					*msg += *numbers;
					delete numconv;
					delete numbers;
					*msg += " in ";
					std::cout << msg->data();
					char * c = new char[msg->length() - 13];
					for (int charPos = 0; charPos < msg->length() - 13; charPos++)
						c[charPos] = ' ';
					c[msg->length() - 14] = 0;
					delete msg;
					std::string * includedFromFormatted = new std::string(*includedFrom);
					replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
					std::cout << *includedFromFormatted << std::endl;
					delete [] c;
					delete includedFromFormatted;
					skipErrors = true;
				}
				delete hashtype;
			} else if (c == '{')
			{
				(*p)--;
				(*column)--;
				return;
			} else if (!skipErrors)
			{
				std::cout << std::endl;
				skipErrors = true;
				std::string * msg = new std::string();
				std::stringstream * numconv = new std::stringstream();
				std::string * numbers = new std::string();
				*msg = "Warning: Unexpected token '";
				*msg += c;
				*msg += "' at line ";
				*numconv << *line;
				*numconv >> *numbers;
				*msg += *numbers;
				*msg += " column ";
				delete numconv;
				numconv = new std::stringstream();
				*numconv << *column;
				*numconv >> *numbers;
				*msg += *numbers;
				delete numconv;
				delete numbers;
				*msg += " in ";
				std::cout << msg->data();
				char * c = new char[msg->length() - 13];
				for (int charPos = 0; charPos < msg->length() - 13; charPos++)
					c[charPos] = ' ';
				c[msg->length() - 14] = 0;
				std::string * includedFromFormatted = new std::string(*includedFrom);
				replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
				std::cout << *includedFromFormatted << std::endl;
				delete msg;
				delete [] c;
				delete includedFromFormatted;
			}
		}
	}

	bool
	ParseName(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::stringstream * variablename, byte * mask, byte * type)
	{
		while (true)
		{
			if (*p >= str.length())
			{
				std::cout << std::endl;
				std::string * msg = new std::string();
				*msg = "Error: Unexpected EOF in ";
				std::cout << msg->data();
				char * c = new char[msg->length() - 13];
				for (int charPos = 0; charPos < msg->length() - 13; charPos++)
					c[charPos] = ' ';
				c[msg->length() - 14] = 0;
				std::string * includedFromFormatted = new std::string(*includedFrom);
				replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
				std::cout << *includedFromFormatted << std::endl;
				delete msg;
				delete [] c;
				delete includedFromFormatted;
				return false;
			}

			char c = str.data()[(*p)++];
			(*column)++;
			if (c == ' ' || c == '	')
				continue;
			else if (c == '\n')
			{
				(*line)++;
				*column = 0;
				continue;
			} else if (c == '"')
			{
				unsigned long int declaration = *p;
				unsigned long int start = 0;
				unsigned long int len = 0;
				while (true)
				{
					if (*p >= str.length())
					{
						std::cout << std::endl;
						std::string * msg = new std::string();
						*msg = "Error: Unexpected EOF in ";
						std::cout << msg->data();
						char * c = new char[msg->length() - 13];
						for (int charPos = 0; charPos < msg->length() - 13; charPos++)
							c[charPos] = ' ';
						c[msg->length() - 14] = 0;
						std::string * includedFromFormatted = new std::string(*includedFrom);
						replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
						std::cout << *includedFromFormatted << std::endl;
						delete msg;
						delete [] c;
						delete includedFromFormatted;
						return false;
					}
					char c2 = str.data()[(*p)++];
					(*column)++;
					if (c2 == '\n')
					{
						(*line)++;
						*column = 0;
					}
					if (c2 == '"')
					{
						if (start != 0)
						{
							char * token = new char[len + 1];
							for (int i = 0; i < len; i++)
								token[i] = str.data()[start + i];
							token[len] = 0;
							std::cout << "Received token '" << token << "'" << std::endl;
							delete token;
							start = 0;
							len = 0;
						}
						break;
					}
					else if (c2 == ' ')
					{
						if (start != 0)
						{
							char * token = new char[len + 1];
							for (int i = 0; i < len; i++)
								token[i] = str.data()[start + i];
							token[len] = 0;
							if (token == "public")
							{
								if (*mask & 3 == 0)
								{
									*mask |= 3;
								} else
								{
									//TODO error
								}
							} else if (token == "protected")
							{
								if (*mask & 3 == 0)
								{
									*mask |= 2;
								} else
								{
									//TODO error
								}
							} else if (token == "private")
							{
								if (*mask & 3 == 0)
								{
									*mask |= 1;
								} else
								{
									//TODO error
								}
							} else if (token == "final" || token == "const" || token == "constant")
							{
								if (*mask & 8 == 0)
								{
									if (*mask & 4 == 0)
									{
										*mask |= 4;
									} else
									{
										//TODO warning
									}
								} else
								{
									//TODO error
								}
							} else if (token == "abstract" || token == "virtual")
							{
								if (*mask & 128 == 0)
								{
									if (*mask & 16 == 0)
									{
										if (*mask & 4 == 0)
										{
											if (*mask & 8 == 0)
											{
												*mask |= 8;
											} else
											{
												//TODO warning
											}
										} else
										{
											//TODO error
										}
									} else
									{
										//TODO error
									}
								} else
								{
									//TODO error
								}
							} else if (token == "native")
							{

							} else if (token == "instantiable")
							{

							} else if (token == "volatile")
							{

							} else if (token == "synchronized")
							{

							}
							delete token;
							start = 0;
							len = 0;
						}
					} else
					{
						if (start == 0)
							start = *p - 1;
						len++;
					}
				}
				break;
			} else
			{
				//TODO Error
			}
		}
		while (true) //Is the variable assigned to a value or to null
		{
			if (*p >= str.length())
			{
				std::cout << std::endl;
				std::string * msg = new std::string();
				*msg = "Error: Unexpected EOF in ";
				std::cout << msg->data();
				char * c = new char[msg->length() - 13];
				for (int charPos = 0; charPos < msg->length() - 13; charPos++)
					c[charPos] = ' ';
				c[msg->length() - 14] = 0;
				std::string * includedFromFormatted = new std::string(*includedFrom);
				replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
				std::cout << *includedFromFormatted << std::endl;
				delete msg;
				delete [] c;
				delete includedFromFormatted;
				return false;
			}
			char c = str.data()[(*p)++];
			(*column)++;
			if (c == ' ' || c == '	')
				continue;
			else if (c == '\n')
			{
				(*line)++;
				*column = 0;
				continue;
			} else if (c == ':')
			{
				return true;
			} else if (c == ',')
			{
				return false;
			} else
			{
				std::cout << std::endl;
				std::string * msg = new std::string();
				std::stringstream * numconv = new std::stringstream();
				std::string * numbers = new std::string();
				*msg = "Warning: Unexpected token '";
				*msg += c;
				*msg += "' at line ";
				*numconv << *line;
				*numconv >> *numbers;
				*msg += *numbers;
				*msg += " column ";
				delete numconv;
				numconv = new std::stringstream();
				*numconv << *column;
				*numconv >> *numbers;
				*msg += *numbers;
				delete numconv;
				delete numbers;
				*msg += " in ";
				std::cout << msg->data();
				char * c = new char[msg->length() - 13];
				for (int charPos = 0; charPos < msg->length() - 13; charPos++)
					c[charPos] = ' ';
				c[msg->length() - 14] = 0;
				std::string * includedFromFormatted = new std::string(*includedFrom);
				replaceAllInString(*includedFromFormatted, "<[\\spaces/]>", c);
				std::cout << *includedFromFormatted << std::endl;
				delete msg;
				delete [] c;
				delete includedFromFormatted;
			}
		}
		return false;
	}

	pointer
	ParseValue(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte * type)
	{
		(*p)++;
		(*column)++;
		std::stringstream * strstr = new std::stringstream();
		byte typer = 0;
		byte mask = 0;
		ParseName(p, str, line, column, includedFrom, strstr, &mask, &typer);
		return 0;
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
		std::string * str = new std::string("Jason Virtual Machine internal compiler");
		LoadScript("D:/JasonVM/Library/System.jll", str);
		delete str;
		//for (int i = 0; i < 65536; i++)
		//	printf("RAM length: %d\n", RAM->getLength());
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
