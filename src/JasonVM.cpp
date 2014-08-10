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
			//TODO keep a reference to the Memprovider instead of a string
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

	size_t
	Find(std::string& str, const std::string& from, size_t start_pos, size_t from_pos, size_t * length)
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
				size_t asterisk = Find(str, from, pos, currentChar, &asterisklen);
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
	ReplaceAllInString(std::string& str, const std::string& from, const std::string& to)
	{
		if(from.empty())
			return str;
		size_t start_pos = 0;
		size_t length = 0;
		while((start_pos = Find(str, from, start_pos, 0, &length)) != std::string::npos)
		{
			str.replace(start_pos, length, to);
			start_pos += to.length() - 1;
		}
		return str;
	}

	void
	PrintError(const char * message, std::string includedFrom)
	{
		std::cout << std::endl;
		std::string msg = std::string(message);
		std::cout << msg;
		std::cout << " in ";
		char * c = new char[msg.length() - 9];
		for (int charPos = 0; charPos < msg.length() - 9; charPos++)
			c[charPos] = ' ';
		std::string * includedFromFormatted = new std::string(includedFrom);
		ReplaceAllInString(*includedFromFormatted, "<[\\\\spaces/]>", c);
		std::cout << *includedFromFormatted << std::endl;
		delete [] c;
		delete includedFromFormatted;
	}

	void
	PrintError(const char * message, unsigned int * line, unsigned int * column, std::string includedFrom)
	{
		std::cout << std::endl;
		std::stringstream * numconv = new std::stringstream();
		std::string * numbers = new std::string();
		std::string * msg = new std::string(message);
		*msg += " at line ";
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
		std::string * includedFromFormatted = new std::string(includedFrom);
		ReplaceAllInString(*includedFromFormatted, "<[\\\\spaces/]>", c);
		std::cout << *includedFromFormatted << std::endl;
		delete msg;
		delete [] c;
		delete includedFromFormatted;
	}

	MemoryProvider * RAM;

	struct Variable
	{
			std::string * variableName;
			byte mask;
			std::string * type;
			byte typeID;
			unsigned long int valueStringPointer;
			unsigned int line;
			unsigned int column;
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

			ReplaceAllInString(*str, "/\\*\\*/", "");
			ReplaceAllInString(*str, "//\n", "\n");

			ReplaceAllInString(*str, "/\\**\\*/", "");
			ReplaceAllInString(*str, "//*\n", "\n");

			//replaceAllInString(*str, "—", "-"); TODO TODO TODO and again TODO
			//replaceAllInString(*str, "×", "*");
			//replaceAllInString(*str, "÷", "/");
			//replaceAllInString(*str, "«", "<<");
			//replaceAllInString(*str, "»", ">>");

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

				//printf("%s\n\tMask:%d\n\tType:%s\n\tTypeID:%d\n\tPIF:%d\n\tNull:%s\n", var->variableName->data(), var->mask, var->type->data(), var->typeID, var->valueStringPointer, var->isNull ? "true" : "false");
				//printf("Parsing next variable @ line %d column %d\n", var->line, var->column);

				pointer varpointer = 0;
				if (!var->isNull)
					varpointer = ParseValue(&(var->valueStringPointer), *str, &(var->line), &(var->column), includedFromNew, &(var->typeID)) & 0x7FFFFFFFFFFFFFFF;
				else
					var->typeID = 0;

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
			ReplaceAllInString(*includedFromFormatted, "<[\\\\spaces/]>", c);
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
				PrintError("Error: Unexpected EOF", *includedFrom);
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
						PrintError("Error: Unexpected EOF", *includedFrom);
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
							PrintError("Error: Unexpected EOF", *includedFrom);
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
					std::string msg = std::string("Warning: Unexpected token '#");
					msg += *hashtype;
					msg += "'";
					PrintError(msg.data(), line, column, *includedFrom);
					skipErrors = true;
				}
				delete hashtype;
			} else if (c == '{')
			{
				return;
			} else if (!skipErrors)
			{
				std::string msg = std::string("Warning: Unexpected token '");
				msg += c;
				msg += "'";
				PrintError(msg.data(), line, column, *includedFrom);
				skipErrors = true;
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
				PrintError("Error: Unexpected EOF", *includedFrom);
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
				std::string msg = std::string("Warning: Unexpected token '");
				msg += c;
				msg += "'";
				PrintError(msg.data(), line, column, *includedFrom);
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
				PrintError("Error: Unexpected EOF", *includedFrom);
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
							v->mask |= 3;
						else
							PrintError("Warning: Variable visibility already declared for variable", line, column, *includedFrom);
					} else if (std::string("protected") == token)
					{
						if ((v->mask & 3) == 0)
							v->mask |= 2;
						else
							PrintError("Warning: Variable visibility already declared for variable", line, column, *includedFrom);
					} else if (std::string("private") == token)
					{
						if ((v->mask & 3) == 0)
							v->mask |= 1;
						else
							PrintError("Warning: Variable visibility already declared for variable", line, column, *includedFrom);
					} else if (std::string("final") == token || std::string("const") == token || std::string("constant") == token)
					{
						if ((v->mask & 8) == 0)
						{
							if ((v->mask & 4) == 0)
								v->mask |= 4;
							else
								PrintError("Warning: Variable declared as constant multiple times", line, column, *includedFrom);
						} else
							PrintError("Warning: An abstract variable can't be declared as a constant", line, column, *includedFrom);
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
										PrintError("Warning: Variable declared as abstract multiple times", line, column, *includedFrom);
									}
								} else
								{
									PrintError("Warning: A constant variable can't be declared as abstract", line, column, *includedFrom);
								}
							} else
							{
								PrintError("Warning: An instantiable variable can't be declared as abstract", line, column, *includedFrom);
							}
						} else
						{
							PrintError("Warning: A native variable can't be declared as abstract", line, column, *includedFrom);
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
									PrintError("Warning: Variable declared as native multiple times", line, column, *includedFrom);
								}
							} else
							{
								PrintError("Warning: An instantiable variable can't be declared as native (yet)", line, column, *includedFrom);
							}
						} else
						{
							PrintError("Warning: An abstract variable can't be declared as native", line, column, *includedFrom);
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
									PrintError("Warning: Variable declared as instantiable multiple times", line, column, *includedFrom);
								}
							} else
							{
								PrintError("Warning: A native variable can't be declared as instantiable (yet)", line, column, *includedFrom);
							}
						} else
						{
							PrintError("Warning: An abstract variable can't be declared as instantiable", line, column, *includedFrom);
						}
					} else if (std::string("volatile") == token)
					{
						if ((v->mask & 32) == 0)
						{
							v->mask |= 32;
						} else
						{
							PrintError("Warning: Variable declared as volatile multiple times", line, column, *includedFrom);
						}
					} else if (std::string("synchronized") == token)
					{
						if ((v->mask & 64) == 0)
						{
							v->mask |= 64;
						} else
						{
							PrintError("Warning: Variable declared as synchronized multiple times", line, column, *includedFrom);
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
							std::string msg("Warning: Unexpected token '");
							msg += token;
							msg += "'";
							PrintError(msg.data(), line, column, *includedFrom);
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
			PrintError("Error: No variable name supplied", line, column, *includedFrom);
		v->variableName = name;
		v->type = objtype;
		while (true) //Is the variable assigned to a value or to null
		{
			if (*p >= str.length())
			{
				PrintError("Error: Unexpected EOF", *includedFrom);
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
				v->valueStringPointer = *p;
				v->line = *line;
				v->column = *column;
				v->isNull = false;
				break;
			} else if (c == ',' || c == '}')
			{
				v->isNull = true;
				break;
			} else
			{
				std::string msg("Warning: Unexpected token '");
				msg += c;
				msg += "'";
				PrintError(msg.data(), line, column, *includedFrom);
			}
		}
		*returncode = 0;
		return v;
	}

	std::string operators[] = {"+", "-", "*", "/", "%", "^", "<<", ">>", "<", "<=", "==", ">=", ">", "!=", "===", "!==", ":", "=", "&", "!&", "|", "!|", "&&", "||", "~", "!&&", "!||", "+:", "+=", "-:", "-=", "*:", "*=", "/:", "/=", "%:", "%=", "^:", "^=", "<<:", "<<=", ">>:", ">>=", "&:", "&=", "|:", "|=", "++", "--", ".", ".*", "[", "]", "(", ")", "?", ",", "{", "}"};
	std::string oplevels[][23/*TODO inefficient*/] = {
			{"++", "--", "."},
			{"pre++", "pre--", "u+", "u-", "~", "!", "a&", "p*", "sizeof"},
			{".*"},
			{"^"},
			{"*", "/", "%"},
			{"+", "-"},
			{"<<", ">>"},
			{"<", "<=", ">", ">="},
			{"==", "===", "!=", "!=="},
			{"&", "!&"},
			{"|", "!|"},
			{"&&", "!&&"},
			{"||", "!||"},
			{"?:", ":", "=", "+:", "+=", "-:", "-=", "*:", "*=", "/:", "/=", "%:", "%=", "^:", "^=", "<<:", "<<=", ">>:", ">>=", "&:", "&=", "|:", "|="},
			{"throw", "assert"},
			{","},
			{"[", "("}
	};
	byte opamount[] = {3, 9, 1, 1, 3, 2, 2, 4, 4, 2, 2, 2, 2, 23, 2, 1, 2};
	bool opdirection[] = {false, true, false, true, false, false, false, false, false, false, false, false, false, true, true, false, false};

	byte GetOperatorLevel(std::string token)
	{
		for (byte i = 0; i < 17; i++)
			for (int j = 0; j < opamount[i]; j++)
				if (oplevels[i][j] == token)
					return i;
		return 255;
	}

	bool IsPartOfOperator(std::string str, unsigned long int start, unsigned long int len)
	{
		if (start == 0)
			return false;
		for (int i = 0; i < 59; i++)
			if (operators[i].find(str.substr(start, len)) != std::string::npos)
				return true;
		return false;
	}

	void ParseArray(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<std::string *> * tokens);
	void ParseParentheses(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<std::string *> * tokens);
	//void ParseArray(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<std::string *> * tokens);

	void
	ParseArray(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<std::string *> * tokens)
	{
		unsigned long int start = 0;
		unsigned long int len = 0;

		bool expectingOperator = false;

		while (true)
		{
			bool change = false;
			bool skipc2 = false;
			if (*p >= str.length())
			{
				if (start != 0)
					change = skipc2 = true;
				else
					break;
			}
			char c = ' ';
			if (!skipc2)
			{
				c = str.data()[(*p)++];
				(*column)++;
			}
			if (c == '\n')
			{
				(*line)++;
				*column = 0;
			}
			if (c != ' ' && c != '\n' && c !=  '	')
			{
				if (IsPartOfOperator(str, start, len + 1))
				{
					if (!expectingOperator)
						change = expectingOperator = true;
				} else
				{
					bool partOfOperator = IsPartOfOperator(str, *p - 1, 1);
					if (expectingOperator)
						change = partOfOperator ? expectingOperator = true : !(expectingOperator = false);
					else if (partOfOperator)
						change = expectingOperator = true;
				}
			}
			if (change || c == ' ' || c == '\n' || c == '	')
			{
				if (!change) expectingOperator = !expectingOperator;
				if (start != 0)
				{
					char * token = new char[len + 1];
					for (int i = 0; i < len; i++)
						token[i] = str.data()[start + i];
					token[len] = 0;
					std::string tokenstr = std::string(token);
					if (tokenstr == "{")
					{
						std::stringstream * insertabletoken = new std::stringstream;
						*insertabletoken << "p";
						*insertabletoken << *p;
						std::string * insertable = new std::string;
						*insertabletoken >> *insertable;
						delete insertabletoken;
						tokens->push_back(insertable);
						SkipBrackets(p, str, column, line);
					} else if (tokenstr == "}")
					{
						PrintError("Warning: mismatched brackets", line, column, *includedFrom);
					} else if (tokenstr == "(")
					{
						(*p)--;
						(*column)--;
						std::string * insertable = new std::string("o(");
						tokens->push_back(insertable);
						ParseParentheses(p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == ")")
					{
						PrintError("Warning: mismatched parentheses", line, column, *includedFrom);
					} else if (tokenstr == "[")
					{
						(*p)--;
						(*column)--;
						std::string * insertable = new std::string("o[");
						tokens->push_back(insertable);
						ParseArray(p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == "]")
					{
						(*p)--;
						(*column)--;
						std::string * insertable = new std::string("o]");
						tokens->push_back(insertable);
						break;
					} else
					{
						std::string * insertable = new std::string("o");
						*insertable += token;
						tokens->push_back(insertable);
					}
					start = 0;
					len = 0;
					delete [] token;
				}
				if (change)
				{
					start = *p - 1;
					len = 1;
				}
			} else
			{
				if (start == 0)
					start = *p - 1;
				len++;
			}
		}
	}

	void
	ParseParentheses(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<std::string *> * tokens)
	{
		unsigned long int start = 0;
		unsigned long int len = 0;

		bool expectingOperator = false;

		while (true)
		{
			bool change = false;
			bool skipc2 = false;
			if (*p >= str.length())
			{
				if (start != 0)
					change = skipc2 = true;
				else
					break;
			}
			char c = ' ';
			if (!skipc2)
			{
				c = str.data()[(*p)++];
				(*column)++;
			}
			if (c == '\n')
			{
				(*line)++;
				*column = 0;
			}
			if (c != ' ' && c != '\n' && c !=  '	')
			{
				if (IsPartOfOperator(str, start, len + 1))
				{
					if (!expectingOperator)
						change = expectingOperator = true;
				} else
				{
					bool partOfOperator = IsPartOfOperator(str, *p - 1, 1);
					if (expectingOperator)
						change = partOfOperator ? expectingOperator = true : !(expectingOperator = false);
					else if (partOfOperator)
						change = expectingOperator = true;
				}
			}
			if (change || c == ' ' || c == '\n' || c == '	')
			{
				if (!change) expectingOperator = !expectingOperator;
				if (start != 0)
				{
					char * token = new char[len + 1];
					for (int i = 0; i < len; i++)
						token[i] = str.data()[start + i];
					token[len] = 0;
					std::string tokenstr = std::string(token);
					if (tokenstr == "{")
					{
						std::stringstream * insertabletoken = new std::stringstream;
						*insertabletoken << "p";
						*insertabletoken << *p;
						std::string * insertable = new std::string;
						*insertabletoken >> *insertable;
						delete insertabletoken;
						tokens->push_back(insertable);
						SkipBrackets(p, str, column, line);
					} else if (tokenstr == "}")
					{
						PrintError("Warning: mismatched brackets", line, column, *includedFrom);
					} else if (tokenstr == "(")
					{
						(*p)--;
						(*column)--;
						std::string * insertable = new std::string("o(");
						tokens->push_back(insertable);
						ParseParentheses(p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == ")")
					{
						(*p)--;
						(*column)--;
						std::string * insertable = new std::string("o)");
						tokens->push_back(insertable);
						break;
					} else if (tokenstr == "[")
					{
						(*p)--;
						(*column)--;
						std::string * insertable = new std::string("o[");
						tokens->push_back(insertable);
						ParseArray(p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == "]")
					{
						PrintError("Warning: mismatched array", line, column, *includedFrom);
					} else
					{
						std::string * insertable = new std::string("o");
						*insertable += token;
						tokens->push_back(insertable);
					}
					start = 0;
					len = 0;
					delete [] token;
				}
				if (change)
				{
					start = *p - 1;
					len = 1;
				}
			} else
			{
				if (start == 0)
					start = *p - 1;
				len++;
			}
		}
	}

	pointer //TODO untested :/
	ParseValue(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte * type)
	{
		unsigned long int start = 0;
		unsigned long int len = 0;
		std::vector<std::string *> * tokens = new std::vector<std::string *>;

		bool expectingOperator = false;

		while (true)
		{
			bool change = false;
			bool skipc2 = false;
			if (*p >= str.length())
			{
				if (start != 0)
					change = skipc2 = true;
				else
					break;
			}
			char c = ' ';
			if (!skipc2)
			{
				c = str.data()[(*p)++];
				(*column)++;
			}
			if (c == '\n')
			{
				(*line)++;
				*column = 0;
			}
			if (c != ' ' && c != '\n' && c !=  '	')
			{
				if (IsPartOfOperator(str, start, len + 1))
				{
					if (!expectingOperator)
						change = expectingOperator = true;
				} else
				{
					bool partOfOperator = IsPartOfOperator(str, *p - 1, 1);
					if (expectingOperator)
						change = partOfOperator ? expectingOperator = true : !(expectingOperator = false);
					else if (partOfOperator)
						change = expectingOperator = true;
				}
			}
			if (change || c == ' ' || c == '\n' || c == '	')
			{
				if (!change) expectingOperator = !expectingOperator;
				if (start != 0)
				{
					char * token = new char[len + 1];
					for (int i = 0; i < len; i++)
						token[i] = str.data()[start + i];
					token[len] = 0;
					std::string tokenstr = std::string(token);
					if (tokenstr == "{")
					{
						char * insertabletoken = new char[6];
						insertabletoken[0] = 'p';
						insertabletoken[1] = ((*p) >> 24) & 0xFF;
						insertabletoken[2] = ((*p) >> 16) & 0xFF;
						insertabletoken[3] = ((*p) >> 8) & 0xFF;
						insertabletoken[4] = (*p) & 0xFF;
						std::string * insertable = new std::string(insertabletoken);
						delete [] insertabletoken;
						tokens->push_back(insertable);
						SkipBrackets(p, str, column, line);
					} else if (tokenstr == "," || tokenstr == "}")
					{
						break;
					} else if (tokenstr == "(")
					{
						(*p)--;
						(*column)--;
						std::string * insertable = new std::string("o(");
						tokens->push_back(insertable);
						ParseParentheses(p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == ")")
					{
						PrintError("Warning: mismatched parentheses", line, column, *includedFrom);
					} else if (tokenstr == "[")
					{
						(*p)--;
						(*column)--;
						std::string * insertable = new std::string("o[");
						tokens->push_back(insertable);
						ParseArray(p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == "]")
					{
						PrintError("Warning: mismatched array", line, column, *includedFrom);
					} else
					{
						std::string * insertable = new std::string("o");
						*insertable += token;
						tokens->push_back(insertable);
					}
					start = 0;
					len = 0;
					delete [] token;
				}
				if (change)
				{
					start = *p - 1;
					len = 1;
				}
			} else
			{
				if (start == 0)
					start = *p - 1;
				len++;
			}
		}
		std::vector<std::string *> * output = new std::vector<std::string *>;
		std::vector<std::string *> * stack = new std::vector<std::string *>;

		bool wasLastOperator = true;

		for (size_t i = 0; i < tokens->size(); i++)
		{
			std::string * str = tokens->at(i);

			char c = *(str->data());

			if (c == 'o')
			{
				byte oplevel;
				std::string * token;
				if (wasLastOperator)
				{
					if (*str == "o+")
						*str = "ou+";
					else if (*str == "o-")
						*str = "ou-";
					else if (*str == "o++")
						*str = "opre++";
					else if (*str == "o--")
						*str = "opre--";
					else if (*str == "o&")
						*str = "oa&";
					else if (*str == "o*")
						*str = "op*";
					else
					{
						PrintError("Warning: unexpected operator, expecting variable", line, column, *includedFrom);
					}
				}
				if (*str == "o)" || *str == "o]")
				{
					const char * type = "array";
					if (*str == "o)")
						type = "parentheses";
					wasLastOperator = true;
					while (true)
					{
						if (stack->empty())
						{
							PrintError("Warning: mismatched " + type, line, column, *includedFrom);
							break;
						}
						token = stack->back();
						stack->pop_back();
						if (*token == "O(" || *token == "O[")
							break;
						output->push_back(token);
					}
				} else if (*str == "o,")
				{
					wasLastOperator = true;
					while (true)
					{
						if (stack->empty())
						{
							//TODO comma for multiple values
							break;
						}
						token = stack->back();
						if (*token == "O(" || *token == "O[")
							break;
						stack->pop_back();
						output->push_back(token);
					}
				} else if (*str == "o(" || *str == "o[")
				{
					wasLastOperator = true;
					str->replace(0, 1, "O");
					stack->push_back(str);
				} else if ((oplevel = GetOperatorLevel((std::string) (str->substr((size_t) 1, (size_t) str->length() - 1)))) != 255)
				{
					str->replace(0, 1, "O");

					while (!stack->empty())
					{
						byte lastoplevel = GetOperatorLevel(stack->back()->substr((size_t) 1, (size_t) stack->back()->length() - 1));
						if (opdirection[oplevel] ? oplevel <= lastoplevel : (oplevel < lastoplevel))
							break;
						output->push_back(stack->back());
						stack->pop_back();
					}

					stack->push_back(str);
					wasLastOperator = true;
				} else
				{
					wasLastOperator = false;
					output->push_back(str);
				}
			} else
			{
				wasLastOperator = false;
				stack->push_back(str);
			}
		}

		while (!stack->empty())
		{
			output->push_back(stack->back());
			stack->pop_back();
		}

		delete tokens;
		delete stack;

		for (size_t i = 0; i < output->size(); i++)
		{
			//TODO interpret the RPN tokens
			delete output->at(i);
		}

		delete output;
		return 9000;
	}

	void
	ParseBrackets(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<Variable *> * variables)
	{
		while (true)
		{
			if (*p >= str.length())
			{
				PrintError("Error: Unexpected EOF", *includedFrom);
				break;
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
			case 1:
				RAM = new RAMMemoryProvider(MemoryUnitToBytes(arg.memproviderargs.data(), arg.memproviderargs.length() - 1));
				break;
				//RAM = new FileMemoryProvider(arg.memproviderargs.data());
				//break;
		}
		std::string * str = new std::string(" JasonVM internal compiler");
		LoadScript("D:/JasonVM/Library/System.jll", str);
		delete str;
		str = new std::string(" JasonVM");

		//while (RAM->getPointer() < RAM->getLength()) //Memory diagnostics
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
	else if (arg == std::string("--usefile")) return 0x01;
	else if (arg == std::string("--version") || arg == std::string("-v")) return 0x02;
	else if (arg == std::string("--help") || arg == std::string("-h")) return 0x03;
	else return 0xFF;
}

int
PrintHelp()
{
	std::cout <<
	"JasonVM version 1.0.0_1\n\
Jason specification #1 (partially supported)\n\
\n\
Usage: JasonVM [options] file [arguments]\n\
\n\
The available options are:\n\
	--ram			<size>				Select the size of RAM memory to use\n\
\n\
	--usefile		<location>			Use a file at the specified location for RAM\n\
								(Note: Using a file as RAM is slower. It is recommended to use a USB for the file)\n\
								(Note 2: This file may grow and shrink as well)\n\
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
	jason::MachineArgs machine = { 0, std::string("1MB"), argc, argv };
	for (int i = 1; i < argc; i++)
	{
		switch (GetStringArgument(argv[i]))
		{
			case 0x00:
				if (i + 1 < argc)
				{
					machine.memprovider = 1;
					machine.memproviderargs = argv[++i];
				} else
				{
					std::cout << "Correct usage:" << std::endl;
					std::cout << "	--ram <size>" << std::endl;
					return 2;
				}
				break;
			case 0x01:
				if (i + 1 < argc)
				{
					machine.memprovider = 1;
					machine.memproviderargs = argv[++i];
				} else
				{
					std::cout << "Correct usage:" << std::endl;
					std::cout << "	--ram <size>" << std::endl;
					return 2;
				}
				break;
			case 0x02:
				std::cout << "JasonVM version 1.0.0_1" << std::endl;
				std::cout << "Jason specification #1 (partially supported)" << std::endl;
				break;
			case 0x03:
				PrintHelp();
				break;
			default:
				//TODO end the for loop, and then parse the file name and arguments
				break;
		}
	}
	return jason::Run(machine);
}
