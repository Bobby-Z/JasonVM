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

#include "RAMMemoryProvider.h"
#include "FileMemoryProvider.h"
#include <fstream>
#include <sstream>
#include <vector>

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

	size_t
	find(std::string& str, const std::string& from, size_t start_pos, size_t from_pos, size_t * length)
	{
		size_t currentChar = from_pos;
		size_t pos = start_pos;
		size_t starting = std::string::npos;
		while (currentChar < from.length())
		{
			if (pos >= str.length())
				return std::string::npos;
			char c = str.data()[pos++];
			char c2 = from.data()[currentChar++];
			if (c2 == '*')
			{
				pos--;
				size_t asterisklen = 0;
				size_t asterisk = find(str, from, pos, currentChar, &asterisklen);
				if (asterisk != std::string::npos)
				{
					*length = asterisk + asterisklen - starting;
					return starting;
				} else
					return std::string::npos;
			} else
			{
				if (c2 == '\\')
					c2 = from.data()[currentChar++];
				if (c != c2)
				{
					currentChar = from_pos;
					starting = pos;
				}
			}
		}
		*length = pos - starting;
		return starting;
	}

	std::string
	replaceAllInString(std::string& str, const std::string& from, const std::string& to)
	{
		if(from.empty())
			return str;
		size_t start_pos = 0;
		size_t length = 0;
		while((start_pos = find(str, from, start_pos, 0, &length)) != std::string::npos)
		{
			str.replace(start_pos, length, to);
			start_pos += to.length() - 1;
		}
		return str;
	}

	MemoryProvider * RAM;

	struct Variable
	{
			std::string * variableName;
			byte mask;
			std::string * type;
			byte typeID;
			unsigned long int valueStringPointer;
			bool isNull;
	};

	void LoadScript(const char * file, std::string * includedFrom);
	void ParseHeader(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom);
	Variable * ParseName(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte * returncode);
	pointer ParseValue(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte * type);
	void ParseBrackets(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<Variable *> * variables);

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

			replaceAllInString(*str, "/\\*\\*/", ""); //TODO Hacky temporary fix
			replaceAllInString(*str, "//\n", "\n"); //TODO Hacky temporary fix

			replaceAllInString(*str, "/\\**\\*/", "");
			replaceAllInString(*str, "//*\n", "\n");

			unsigned long int * p = new unsigned long int(0);
			unsigned int * line_num = new unsigned int(1);
			unsigned int * column = new unsigned int(0);

			std::string * includedFromNew = new std::string("\"");
			*includedFromNew += file;
			*includedFromNew += "\"\n<[\\spaces/]>included from ";
			*includedFromNew += includedFrom->data();

			ParseHeader(p, *str, line_num, column, includedFromNew);
			std::vector<Variable *> * v = new std::vector<Variable *>;
			ParseBrackets(p, *str, line_num, column, includedFromNew, v);

			pointer size = v->size() * 10;

			for (unsigned int i = 0; i < v->size(); i++)
				size += v->at(i)->variableName->length();

			pointer variable = RAM->createVariable(size);

			printf("Pointer: %d\n", variable);

			RAM->seek(variable + 8);

			for (unsigned int i = 0; i < v->size(); i++)
			{
				Variable * var = v->at(i);
				//printf("%s\n\tMask:%d\n\tType:%s\n\tTypeID:%d\n\tPIF:%d\n\tNull:%s\n", var->variableName.data(), var->mask, var->type.data(), var->typeID, var->valueStringPointer, var->isNull ? "true" : "false");

				pointer varpointer = ParseValue(&(var->valueStringPointer), *str, line_num, column, includedFromNew, &(var->typeID)) & 0x7FFFFFFFFFFFFFFF;

				for (int i = 0; i < var->variableName->length() + 1; i++)
					RAM->write(var->variableName->data()[i]);
				RAM->write(var->mask);
				varpointer |= (pointer) var->typeID << 59 & 0xF8;
				RAM->writeLong(varpointer);
				delete var->variableName;
				delete var->type;
				delete var;
			}

			v->clear();

			delete v;

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

	void SkipArray(unsigned long int * p, std::string str, unsigned int * column, unsigned int * line);
	void SkipBrackets(unsigned long int * p, std::string str, unsigned int * column, unsigned int * line);
	void SkipParentheses(unsigned long int * p, std::string str, unsigned int * column, unsigned int * line);

	void
	SkipArray(unsigned long int * p, std::string str, unsigned int * column, unsigned int * line)
	{
		while (true)
		{
			if (*p >= str.length())
				return;
			char c = str.data()[(*p)++];
			(*column)++;
			if (c == '\n')
			{
				*column = 0;
				(*line)++;
			} else if (c == '{')
				SkipBrackets(p, str, column, line);
			else if (c == '(')
				SkipParentheses(p, str, column, line);
			else if (c == '[')
				SkipArray(p, str, column, line);
			else if (c == ']')
				return;
		}
	}

	void
	SkipBrackets(unsigned long int * p, std::string str, unsigned int * column, unsigned int * line)
	{
		while (true)
		{
			if (*p >= str.length())
				return;
			char c = str.data()[(*p)++];
			(*column)++;
			if (c == '\n')
			{
				*column = 0;
				(*line)++;
			} else if (c == '{')
				SkipBrackets(p, str, column, line);
			else if (c == '(')
				SkipParentheses(p, str, column, line);
			else if (c == '[')
				SkipArray(p, str, column, line);
			else if (c == '}')
				return;
		}
	}

	void
	SkipParentheses(unsigned long int * p, std::string str, unsigned int * column, unsigned int * line)
	{
		while (true)
		{
			if (*p >= str.length())
				return;
			char c = str.data()[(*p)++];
			(*column)++;
			if (c == '\n')
			{
				*column = 0;
				(*line)++;
			} else if (c == '{')
				SkipBrackets(p, str, column, line);
			else if (c == '(')
				SkipParentheses(p, str, column, line);
			else if (c == '[')
				SkipArray(p, str, column, line);
			else if (c == ')')
				return;
		}
	}

	void
	SkipValue(unsigned long int * p, std::string str, unsigned int * column, unsigned int * line)
	{
		while (true)
		{
			if (*p >= str.length())
				return;
			char c = str.data()[(*p)++];
			(*column)++;
			if (c == '\n')
			{
				*column = 0;
				(*line)++;
			} else if (c == '{')
				SkipBrackets(p, str, column, line);
			else if (c == '(')
				SkipParentheses(p, str, column, line);
			else if (c == '[')
				SkipArray(p, str, column, line);
			else if (c == '}')
			{
				(*p)--;
				(*column)--;
				return;
			}
			else if (c == ',')
				return;
		}
	}

	Variable *
	ParseName(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte * returncode)
	{
		Variable * v = new Variable;
		v->mask = 0;
		v->typeID = 0;
		v->valueStringPointer = 0;
		v->isNull = false;
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
				*returncode = 1;
				return v;
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
				break;
			else
			{
				//TODO error
				printf("Error, unexpected token (name parser): \"%c\"\n", c);
			}
		}
		unsigned long int declaration = *p;
		unsigned long int start = 0;
		unsigned long int len = 0;
		std::string * objtype = new std::string;
		std::string * name = new std::string;
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
				*returncode = 1;
				return v;
			}
			char c2 = str.data()[(*p)++];
			(*column)++;
			if (c2 == '\n')
			{
				(*line)++;
				*column = 0;
			}
			if (c2 == ' ' || c2 == '"' || c2 == '\n')
			{
				if (start != 0)
				{
					char * token = new char[len + 1];
					for (int i = 0; i < len; i++)
						token[i] = str.data()[start + i];
					token[len] = 0;
					if (std::string("public") == token)
					{
						if ((v->mask & 3) == 0)
						{
							v->mask |= 3;
						} else
						{
							//TODO error
						}
					} else if (std::string("protected") == token)
					{
						if ((v->mask & 3) == 0)
						{
							v->mask |= 2;
						} else
						{
							//TODO error
						}
					} else if (std::string("private") == token)
					{
						if ((v->mask & 3) == 0)
						{
							v->mask |= 1;
						} else
						{
							//TODO error
						}
					} else if (std::string("final") == token || std::string("const") == token || std::string("constant") == token)
					{
						if ((v->mask & 8) == 0)
						{
							if ((v->mask & 4) == 0)
							{
								v->mask |= 4;
							} else
							{
								//TODO warning
							}
						} else
						{
							//TODO error
						}
					} else if (std::string("abstract") == token || std::string("virtual") == token)
					{
						if ((v->mask & 128) == 0)
						{
							if ((v->mask & 16) == 0)
							{
								if ((v->mask & 4) == 0)
								{
									if ((v->mask & 8) == 0)
									{
										v->mask |= 8;
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
					} else if (std::string("native") == token)
					{
						if ((v->mask & 8) == 0)
						{
							if ((v->mask & 16) == 0)
							{
								if ((v->mask & 128) == 0)
								{
									v->mask |= 128;
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
					} else if (std::string("instantiable") == token || std::string("class") == token)
					{
						if ((v->mask & 8) == 0)
						{
							if ((v->mask & 128) == 0)
							{
								if ((v->mask & 16) == 0)
								{
									v->mask |= 16;
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
					} else if (std::string("volatile") == token)
					{
						if ((v->mask & 32) == 0)
						{
							v->mask |= 32;
						} else
						{
							//TODO warning
						}
					} else if (std::string("synchronized") == token)
					{
						if ((v->mask & 64) == 0)
						{
							v->mask |= 64;
						} else
						{
							//TODO warning
						}
					} else
					{
						if (*name != "" && *objtype == "")
						{
							*objtype = name->data();
							*name = token;
						} else if (*name == "")
							*name = token;
						else
						{
							//TODO error
						}
					}
					delete [] token;
					start = 0;
					len = 0;
				}
				if (c2 == '"')
					break;
			} else
			{
				if (start == 0)
					start = *p - 1;
				len++;
			}
		}
		if (*name == "")
		{
			//TODO error
		}
		v->variableName = name;
		v->type = objtype;
		while (true) //Is the variable assigned to a value or to null
		{
			if (*p >= str.length())
			{
				std::cout << std::endl;
				std::string * msg = new std::string;
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
				*returncode = 1;
				return v;
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
				if ((v->mask & 128) == 128)
				{
					//TODO error
					continue;
				}
				v->valueStringPointer = *p;
				v->isNull = false;
				break;
			} else if (c == ',' || c == '}')
			{
				v->isNull = true;
				break;
			} else
			{
				std::cout << std::endl;
				std::string * msg = new std::string;
				std::stringstream * numconv = new std::stringstream;
				std::string * numbers = new std::string;
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
		*returncode = 0;
		return v;
	}

	//TODO fix memory leak
	pointer
	ParseValue(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte * type)
	{
		unsigned long int declaration = *p;
		unsigned long int start = 0;
		unsigned long int len = 0;
		std::vector<std::string> * tokens = new std::vector<std::string>;

		char lasttype = 'o';
		char expecting = 'v';

		const char * operatorchars = ":=+-—*×/÷^%<>«»!&|{}[]()~;.?";

		while (true)
		{
			bool change = false;
			bool skipc2 = false;
			if (*p >= str.length())
			{
				if (start != 0)
				{
					change = true;
					skipc2 = true;
				} else
				{
					break;
				}
			}
			char c2 = ' ';
			if (!skipc2)
			{
				c2 = str.data()[(*p)++];
				(*column)++;
			}
			if (c2 == '\n')
			{
				(*line)++;
				*column = 0;
			}
			if (strchr(operatorchars, c2) != NULL)
			{
				if (expecting == 'v')
				{
					change = true;
					lasttype = 'v';
					expecting = 'o';
				}
			} else
			{
				if (expecting == 'o')
				{
					change = true;
					lasttype = 'o';
					expecting = 'v';
				}
			}
			if (change || c2 == ' ' || c2 == '\n' || c2 == '	')
			{
				if (start != 0)
				{
					char * token = new char[len + 1];
					for (int i = 0; i < len; i++)
						token[i] = str.data()[start + i];
					token[len] = 0;
					std::string * tokenstr = new std::string(token);
					if (*tokenstr == "{")
					{
						std::stringstream * insertabletoken = new std::stringstream;
						*insertabletoken << "p";
						*insertabletoken << *p;
						std::string * insertable = new std::string;
						*insertabletoken >> *insertable;
						delete insertabletoken;
						tokens->push_back(*insertable);
						std::cout << "{" << std::endl;
						SkipBrackets(p, str, column, line);
					} else if (*tokenstr == "}")
					{
						std::cout << "}" << std::endl;
						break;
					} else if (*tokenstr == "[")
					{
						std::stringstream * insertabletoken = new std::stringstream;
						*insertabletoken << "a";
						*insertabletoken << *p;
						std::string * insertable = new std::string;
						*insertabletoken >> *insertable;
						delete insertabletoken;
						tokens->push_back(*insertable);
						std::cout << "[" << std::endl;
						SkipArray(p, str, column, line);
						std::cout << "]" << std::endl;
					} else
					{
						std::stringstream * insertabletoken = new std::stringstream;
						*insertabletoken << lasttype;
						*insertabletoken << *tokenstr;
						std::string * insertable = new std::string;
						*insertabletoken >> *insertable;
						delete insertabletoken;
						tokens->push_back(*insertable);
						std::cout << *insertable << std::endl;
					}
					start = 0;
					len = 0;
				}
				if (change)
				{
					start = *p - 1;
					len++;
				}
			} else
			{
				if (start == 0)
					start = *p - 1;
				len++;
			}
		}
		return 0;
	}

	void
	ParseBrackets(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<Variable *> * variables)
	{
		while (true)
		{
			if (*p >= str.length())
			{
				std::cout << "Error: unexpected EOF" << std::endl;
				break;
				//TODO error
			}
			char c2 = str.data()[(*p)++];
			(*column)++;
			if (c2 == '}')
				break;
			else
			{
				byte returnval = 0;
				Variable * v = ParseName(p, str, line, column, includedFrom, &returnval);
				if (!v->isNull)
					SkipValue(p, str, column, line);
				variables->push_back(v);
			}
		}
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
		std::string * str = new std::string(" JasonVM internal compiler");
		LoadScript("D:/JasonVM/Library/System.jll", str);
		delete str;
		str = new std::string(" JasonVM");

		//while (RAM->getPointer() < RAM->getLength()) //Memory diagnostics :)
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
