/*
 * Compiler.cpp
 *
 *  Created on: 2014. gada 14. aug.
 *      Author: Roberts
 */

#include "Compiler.h"

namespace jason
{
	struct Variable
	{
			std::string variableName;
			byte mask;
			std::string type;
			byte typeID;
			unsigned long int valueStringPointer;
			unsigned int line;
			unsigned int column;
			bool isNull;
	};

	void LoadScript(const char * file, std::string * includedFrom);
	void ParseHeader(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom);
	Variable ParseName(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte * returncode);
	void ParseValue(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte type, pointer parent);
	void ParseBrackets(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<Variable> & variables);



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
		std::string msg(message);
		std::cout << msg;
		std::cout << " in ";
		char * c = new char[msg.length() - 9];
		for (int charPos = 0; charPos < msg.length() - 9; charPos++)
			c[charPos] = ' ';
		std::string includedFromFormatted = std::string(includedFrom);
		ReplaceAllInString(includedFromFormatted, "<[\\\\spaces/]>", c);
		std::cout << includedFromFormatted << std::endl;
		delete [] c;
	}

	void
	PrintError(const char * message, unsigned int line, unsigned int column, std::string includedFrom)
	{
		std::cout << std::endl;
		std::stringstream numconv;
		std::string numbers;
		std::string msg(message);
		msg += " at line ";
		numconv << line;
		numconv >> numbers;
		msg += numbers;
		msg += " column ";
		numconv = std::stringstream();
		numconv << column;
		numconv >> numbers;
		msg += numbers;
		msg += " in ";
		std::cout << msg.data();
		char * c = new char[msg.length() - 13];
		for (int charPos = 0; charPos < msg.length() - 13; charPos++)
			c[charPos] = ' ';
		c[msg.length() - 14] = 0;
		std::string includedFromFormatted(includedFrom);
		ReplaceAllInString(includedFromFormatted, "<[\\\\spaces/]>", c);
		std::cout << includedFromFormatted << std::endl;
		delete [] c;
	}

	void
	LoadScript(const char * file, std::string * includedFrom)
	{
		std::string * str = new std::string;
		std::ifstream i(file);
		if (i.is_open())
		{
			std::string line;
			while (i.good())
			{
				getline(i, line);
				*str += line;
				*str += "\n";
			}
			i.close();
			delete i;
			std::cout << *str;

			ReplaceAllInString(*str, "/\\*\\*/", "");
			ReplaceAllInString(*str, "//\n", "\n");

			ReplaceAllInString(*str, "/\\**\\*/", "");
			ReplaceAllInString(*str, "//*\n", "\n");

			unsigned long int * p = new unsigned long int(0);
			unsigned int * line_num = new unsigned int(1); //Notepad++ starts at 1
			unsigned int * column = new unsigned int(1); //Notepad++ starts at 1

			std::string * includedFromNew = new std::string("\"");
			*includedFromNew += file;
			*includedFromNew += "\"\n<[\\spaces/]>included from ";
			*includedFromNew += includedFrom->data();

			//TODO tabs should round the column number up to the nearest number divisible by 4, not add 4

			ParseHeader(p, *str, line_num, column, includedFromNew);
			std::vector<Variable> v = std::vector<Variable>;
			ParseValue(p, *str, line_num, column, includedFromNew, (byte) 0, (pointer) 0);

			pointer size = v.size() * 10;

			for (unsigned int i = 0; i < v.size(); i++)
				size += v.at(i).variableName.length();

			pointer variable = RAM->createVariable(size);

			printf("Pointer: %d\n", variable);

			std::cout << "Beginning variable creation" << std::endl;

			for (unsigned int i = 0; i < v.size(); i++)
			{
				std::cout << "New var" << std::endl;
				Variable var = v.at(i);

				//printf("%s\n\tMask:%d\n\tType:%s\n\tTypeID:%d\n\tPIF:%d\n\tNull:%s\n", var->variableName->data(), var->mask, var->type->data(), var->typeID, var->valueStringPointer, var->isNull ? "true" : "false");
				//printf("Parsing next variable @ line %d column %d\n", var->line, var->column);

				pointer varpointer = 0;
				if (!var.isNull)
					varpointer = ParseValue(&(var.valueStringPointer), *str, &(var.line), &(var.column), includedFromNew, (var.typeID), variable) & 0x7FFFFFFFFFFFFFFF;
				else
					var.typeID = 0;

				RAM->write(variable + 8, (byte *) var.variableName.data(), var.variableName.length() + 1);

				RAM->write(variable + 8 + var.variableName.length() + 1, var.mask);
				varpointer |= (pointer) var.typeID << 59 & 0xF8;
				RAM->writeLong(variable + 8 + var.variableName.length() + 2, varpointer);
			}
			std::cout << "Ended variable creation" << std::endl;

			v.clear();

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
			if (c == ' ')
				continue;
			else if (c == '	')
			{
				*column += 3;
				continue;
			} else if (c == '\n')
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
					if (c2 == ' ')
						break;
					else if (c2 == '	')
					{
						*column += 3;
					} else if (c2 == '\n')
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
						if (c2 == '\n')
						{
							(*column) = 0;
							(*line)++;
						} else if (c2 == '	')
							*column += 3;

						if (c2 == '<' && closingType == ' ')
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
					PrintError(msg.data(), *line, *column - 1, *includedFrom);
					skipErrors = true;
				}
				delete hashtype;
			} else
			{
				return;
			}
		}
	}

	void
	SkipUntil(char until, unsigned long int * p, std::string str, unsigned int * column, unsigned int * line, std::string * includedFrom)
	{
		while (true)
		{
			if (*p >= str.length())
				return;
			char c = str.data()[(*p)++];
			(*column)++;
			if (c == '\n')
			{
				*column = 1;
				(*line)++;
			} else if (c == '	')
			{
				*column += 3;
			} else if (c == '{')
				SkipUntil('}', p, str, column, line, includedFrom);
			else if (c == '(')
				SkipUntil(')', p, str, column, line, includedFrom);
			else if (c == '[')
				SkipUntil(']', p, str, column, line, includedFrom);
			else if (c == until)
				return;
			else if (c == '}')
				PrintError("Warning: mismatched closing bracket", *line, *column - 1, *includedFrom);
			else if (c == ')')
				PrintError("Warning: mismatched closing parentheses", *line, *column - 1, *includedFrom);
			else if (c == ']')
				PrintError("Warning: mismatched closing array", *line, *column - 1, *includedFrom);
		}
	}

	void
	SkipValue(unsigned long int * p, std::string str, unsigned int * column, unsigned int * line, std::string * includedFrom)
	{
		while (true)
		{
			if (*p >= str.length())
				return;
			char c = str.data()[(*p)++];
			(*column)++;
			if (c == '\n')
			{
				*column = 1;
				(*line)++;
			} else if (c == '	')
			{
				*column += 3;
			} else if (c == '{')
				SkipUntil('}', p, str, column, line, includedFrom);
			else if (c == '(')
				SkipUntil(')', p, str, column, line, includedFrom);
			else if (c == '[')
				SkipUntil(']', p, str, column, line, includedFrom);
			else if (c == '}')
			{
				(*p)--;
				(*column)--;
				return;
			}
			else if (c == ',')
				return;
			else if (c == ')')
			{

			} else if (c == ']')
			{

			}
		}
	}

	byte
	ParseName_CreateMask(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, Variable & v)
	{

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
				return 1;
			}
			char c2 = str.data()[(*p)++];
			(*column)++;
			if (c2 == '\n')
			{
				(*line)++;
				*column = 1;
			} else if (c2 == '	')
				*column += 3;
			if (c2 == ' ' || c2 == '"' || c2 == '\n' || c2 == '	')
			{
				if (start != 0)
				{
					char * token = new char[len + 1];
					for (int i = 0; i < len; i++)
						token[i] = str.data()[start + i];
					token[len] = 0;
					if (std::string("public") == token)
					{
						if ((v.mask & 3) != 0)
							PrintError("Warning: Variable visibility already declared for variable", *line, *column - 1, *includedFrom);
						v.mask = (v.mask & 252) | 3;
					} else if (std::string("protected") == token)
					{
						if ((v.mask & 3) != 0)
							PrintError("Warning: Variable visibility already declared for variable", *line, *column - 1, *includedFrom);
						v.mask = (v.mask & 252) | 2;
					} else if (std::string("private") == token)
					{
						if ((v.mask & 3) != 0)
							PrintError("Warning: Variable visibility already declared for variable", *line, *column - 1, *includedFrom);
						v.mask = (v.mask & 252) | 1;
					} else if (std::string("final") == token || std::string("const") == token || std::string("constant") == token)
					{ //TODO update the mask to fit the actual guideline
						if ((v.mask & 12) == 0)
							v.mask |= 4;
						else if ((v.mask & 12) == 4)
							PrintError("Warning: Variable declared as " + token + " multiple times", *line, *column - 1, *includedFrom);
						else if ((v.mask & 12) == 12)
							PrintError("Warning: A native variable is automatically a " + token + " variable too", *line, *column - 1, *includedFrom);
						else
						{
							PrintError("Error: An abstract variable can't be declared as a " + token, *line, *column - 1, *includedFrom);
						}
					} else if (std::string("abstract") == token || std::string("virtual") == token)
					{
						if ((v.mask & 128) == 0)
						{
							if ((v.mask & 16) == 0)
							{
								if ((v.mask & 4) == 0)
								{
									if ((v.mask & 8) == 0)
										v.mask |= 8;
									else
										PrintError("Warning: Variable declared as abstract multiple times", *line, *column - 1, *includedFrom);
								} else
									PrintError("Warning: A constant variable can't be declared as abstract", *line, *column - 1, *includedFrom);
							} else
								PrintError("Warning: An instantiable variable can't be declared as abstract", *line, *column - 1, *includedFrom);
						} else
							PrintError("Warning: A native variable can't be declared as abstract", *line, *column - 1, *includedFrom);
					} else if (std::string("native") == token)
					{
						if ((v.mask & 8) == 0)
						{
							if ((v.mask & 16) == 0)
							{
								if ((v.mask & 128) == 0)
									v.mask |= 128;
								else
									PrintError("Warning: Variable declared as native multiple times", *line, *column - 1, *includedFrom);
							} else
								PrintError("Warning: An instantiable variable can't be declared as native (yet)", *line, *column - 1, *includedFrom);
						} else
							PrintError("Warning: An abstract variable can't be declared as native", *line, *column - 1, *includedFrom);
					} else if (std::string("instantiable") == token || std::string("class") == token)
					{
						if ((v.mask & 8) == 0)
						{
							if ((v.mask & 128) == 0)
							{
								if ((v.mask & 16) == 0)
									v.mask |= 16;
								else
									PrintError("Warning: Variable declared as instantiable multiple times", *line, *column - 1, *includedFrom);
							} else
								PrintError("Warning: A native variable can't be declared as instantiable (yet)", *line, *column - 1, *includedFrom);
						} else
							PrintError("Warning: An abstract variable can't be declared as instantiable", *line, *column - 1, *includedFrom);
					} else if (std::string("volatile") == token)
					{
						if ((v.mask & 32) == 0)
							v.mask |= 32;
						else
							PrintError("Warning: Variable declared as volatile multiple times", *line, *column - 1, *includedFrom);
					} else if (std::string("synchronized") == token)
					{
						if ((v.mask & 64) == 0)
							v.mask |= 64;
						else
							PrintError("Warning: Variable declared as synchronized multiple times", *line, *column - 1, *includedFrom);
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
							PrintError(msg.data(), *line, *column - 1, *includedFrom);
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
			PrintError("Error: No variable name supplied", *line, *column - 1, *includedFrom);
		v.variableName = name;
		v.type = objtype;

		return 0;
	}

	byte
	ParseName_VariableNext(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, Variable & v)
	{
		while (true) //Is the variable assigned to a value or to null
		{
			if (*p >= str.length())
			{
				PrintError("Error: Unexpected EOF", *includedFrom);
				return 1;
			}
			char c = str.data()[(*p)++];
			(*column)++;
			if (c == ' ')
				continue;
			else if (c == '\n')
			{
				(*line)++;
				*column = 1;
				continue;
			} else if (c == '	')
			{
				*column += 3;
				continue;
			} else if (c == ':')
			{
				v.valueStringPointer = *p;
				v.line = *line;
				v.column = *column;
				v.isNull = false;
				break;
			} else if (c == ',' || c == '}')
			{
				v.isNull = true;
				break;
			} else
			{
				std::string msg("Warning: Unexpected token '");
				msg += c;
				msg += "'";
				PrintError(msg.data(), *line, *column - 1, *includedFrom);
			}
		}
		return 0;
	}

	Variable
	ParseName(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte * returncode)
	{
		Variable v;
		v.mask = 0;
		v.typeID = 0;
		v.valueStringPointer = 0;
		v.isNull = false;

		if ((*returncode = ParseName_CreateMask(p, str, line, column, includedFrom, v)) == 1)
			return v;

		if ((*returncode = ParseName_VariableNext(p, str, line, column, includedFrom, v)) == 1)
			return v;

		return v;
	}

	std::string operators[] = {"+", "-", "*", "/", "%", "^", "<<", ">>", "<", "<=", "==", ">=", ">", "!=", "===", "!==", ":", "=", "&", "!&", "|", "!|", "&&", "||", "~", "!&&", "!||", "+:", "+=", "-:", "-=", "*:", "*=", "/:", "/=", "%:", "%=", "^:", "^=", "<<:", "<<=", ">>:", ">>=", "&:", "&=", "|:", "|=", "++", "--", ".", ".*", "[", "]", "(", ")", "?", ",", "{", "}"};
	std::string oplevels[][23/*TODO inefficient*/] = {
			{"++", "--", ".", "[", "("},
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
			{"c[", "g("}
	};
	byte opamount[] = {5, 9, 1, 1, 3, 2, 2, 4, 4, 2, 2, 2, 2, 23, 2, 1, 2};
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

	void
	ParseUntilReceived(std::string until, unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<std::string> & tokens)
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
				if (c == '\n')
				{
					(*line)++;
					*column = 1;
				} else if (c == '	')
				{
					*column += 3;
				}
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
					delete [] token;
					if (tokenstr == "{")
					{
						char insertabletoken[6];
						insertabletoken[0] = 'p';
						insertabletoken[1] = ((*p) >> 24) & 0xFF;
						insertabletoken[2] = ((*p) >> 16) & 0xFF;
						insertabletoken[3] = ((*p) >> 8) & 0xFF;
						insertabletoken[4] = (*p) & 0xFF;
						std::string insertable = std::string(insertabletoken);
						tokens.push_back(insertable);
						SkipUntil('}', p, str, column, line, includedFrom);
					} else if (tokenstr == "}")
					{
						PrintError("Warning: mismatched closing bracket", *line, *column - 1, *includedFrom);
						break;
					} else if (tokenstr == "(")
					{
						(*p)--;
						(*column)--;
						std::string insertable = std::string("o(");
						tokens.push_back(insertable);
						ParseUntilReceived(")", p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == until)
					{
						std::string insertable = std::string("o");
						insertable += until;
						tokens.push_back(insertable);
						break;
					} else if (tokenstr == ")")
					{
						PrintError("Warning: mismatched closing parentheses", *line, *column - 1, *includedFrom);
					} else if (tokenstr == "[")
					{
						(*p)--;
						(*column)--;
						std::string insertable = std::string("o[");
						tokens.push_back(insertable);
						ParseUntilReceived("]", p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == "]")
					{
						PrintError("Warning: mismatched closing array", *line, *column - 1, *includedFrom);
					} else
					{
						std::string insertable = std::string("o");
						insertable += tokenstr;
						tokens.push_back(insertable);
					}
					start = 0;
					len = 0;
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

	Pointer
	ParseToken(std::string token, pointer parent)
	{
		bool isBase = false;
		unsigned long int number = 0;

		bool isUnsigned = false;
		Pointer pointer;
		pointer.mask = 7;
		pointer.type = 4;
		bool isInMemory = false;
		for (int p = 0; p < token.length(); p++)
		{
			char c = token.data()[p];
			if (c >= '0' && c <= '9')
			{
				c -= '0';
				number *= 10;
				number += c;
			} else if (c == 'x' || c == 'X')
			{
				isBase = true;
				break;
			} else if ((c == 'u' || c == 'U') && p == token.length() - 2)
			{
				isUnsigned = true;
			} else if ((c == 'b' || c == 'B') && p == token.length() - 1)
			{
				pointer.type = isUnsigned ? 6 : 2;
			} else if ((c == 's' || c == 'S') && p == token.length() - 1)
			{
				pointer.type = isUnsigned ? 7 : 3;
			} else if ((c == 'i' || c == 'I') && p == token.length() - 1)
			{
				pointer.type = isUnsigned ? 8 : 4;
			} else if ((c == 'l' || c == 'L') && p == token.length() - 1)
			{
				pointer.type = isUnsigned ? 9 : 5;
			} else if ((c == 'f' || c == 'F') && p == token.length() - 1)
			{
				pointer.type = isUnsigned ? 13 : 10;
			} else if ((c == 'd' || c == 'D') && p == token.length() - 1)
			{
				pointer.type = isUnsigned ? 14 : 11;
			} else if ((c == 'q' || c == 'Q') && p == token.length() - 1)
			{
				pointer.type = isUnsigned ? 15 : 12;
			} else
			{
				isInMemory = true;
				break;
			}
		}
		if (isInMemory)
		{
			Pointer p;
			if (RAM->searchForVariable(parent, token.data(), p))
			{
				return p;
			} else
			{
				p.type = 1;
				return p;
			}
		} else
		{
			if (isBase)
			{
				unsigned long int number2 = 0;
				for (int p = 0; p < token.length(); p++)
				{
					char c = token.data()[p];
					if (c >= '0' && c <= '9')
					{
						c -= '0';
						number2 *= number;
						number2 += c;
					} else if ((c == 'u' || c == 'U') && p == token.length() - 2)
					{
						isUnsigned = true;
					} else if ((c == 'b' || c == 'B') && p == token.length() - 1)
					{
						pointer.type = isUnsigned ? 6 : 2;
					} else if ((c == 's' || c == 'S') && p == token.length() - 1)
					{
						pointer.type = isUnsigned ? 7 : 3;
					} else if ((c == 'i' || c == 'I') && p == token.length() - 1)
					{
						pointer.type = isUnsigned ? 8 : 4;
					} else if ((c == 'l' || c == 'L') && p == token.length() - 1)
					{
						pointer.type = isUnsigned ? 9 : 5;
					} else if ((c == 'f' || c == 'F') && p == token.length() - 1)
					{
						pointer.type = isUnsigned ? 13 : 10;
					} else if ((c == 'd' || c == 'D') && p == token.length() - 1)
					{
						pointer.type = isUnsigned ? 14 : 11;
					} else if ((c == 'q' || c == 'Q') && p == token.length() - 1)
					{
						pointer.type = isUnsigned ? 15 : 12;
					} else
					{
						c -= 32;
						if (c >= 'A' && c < ('A' + (number - 10)))
						{
							c -= 'A';
							c += 10;
							number2 *= number;
							number2 += c;
						} else
						{
							//TODO error
						}
					}
				}
				pointer.loc = number2;
			} else
				pointer.loc = number;

			//TODO put it in memory
			return pointer;
		}
	}

	std::vector<std::string>
	Tokenize(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom)
	{
		unsigned long int start = 0;
		unsigned long int len = 0;
		std::vector<std::string> tokens;

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
				if (c == '\n')
				{
					(*line)++;
					*column = 1;
				} else if (c == '	')
				{
					*column += 3;
				}
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
					delete [] token;
					if (tokenstr == "{")
					{
						char insertabletoken[14];
						insertabletoken[0] = 'p';
						memcpy(insertabletoken + 1, p, 4);
						memcpy(insertabletoken + 5, line, 4);
						memcpy(insertabletoken + 9, column, 4);
						std::string insertable = std::string(insertabletoken);
						tokens.push_back(insertable);
						SkipUntil('}', p, str, column, line, includedFrom);
					} else if (tokenstr == "," || tokenstr == "}")
					{
						break;
					} else if (tokenstr == "(")
					{
						(*p)--;
						(*column)--;
						std::string insertable = std::string("o(");
						tokens.push_back(insertable);
						ParseUntilReceived(")", p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == ")")
					{
						PrintError("Warning: mismatched closing parentheses", *line, *column - 1, *includedFrom);
					} else if (tokenstr == "[")
					{
						(*p)--;
						(*column)--;
						std::string insertable = std::string("o[");
						tokens.push_back(insertable);
						ParseUntilReceived("]", p, str, line, column, includedFrom, tokens);
					} else if (tokenstr == "]")
					{
						PrintError("Warning: mismatched closing array", *line, *column - 1, *includedFrom);
					} else
					{
						std::string insertable = std::string("o");
						insertable += tokenstr;
						tokens.push_back(insertable);
					}
					start = 0;
					len = 0;
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
		return tokens;
	}

	std::vector<std::string>
	CalculatePrecedence(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<std::string> tokens)
	{
		std::vector<std::string> output;
		std::vector<std::string> stack;

		bool wasLastOperator = true;

		for (size_t i = 0; i < tokens.size(); i++)
		{
			std::string str = tokens.at(i);

			char c = *(str.data());

			if (c == 'o')
			{
				byte oplevel;
				std::string token;
				if (wasLastOperator)
				{
					if (str == "o+")
						str = "ou+";
					else if (str == "o-")
						str = "ou-";
					else if (str == "o++")
						str = "opre++";
					else if (str == "o--")
						str = "opre--";
					else if (str == "o&")
						str = "oa&";
					else if (str == "o*")
						str = "op*";
					else if (str == "o[")
						str = "oc[";
					else if (str == "o(")
						str = "og(";
				}
				if (str == "o)" || str == "o]")
				{
					wasLastOperator = false;
					while (true)
					{
						if (stack.empty())
						{
							PrintError((std::string("Warning: mismatched ") + (str == "o)" ? "parentheses" : "array")).data(), *includedFrom);
							break;
						}
						token = stack.back();
						stack.pop_back();
						if (token == "Og(" || token == "Oc[" || token == "O[" || token == "O(" || token == "O,")
							break;
						output.push_back(token);
					}
				} else if (str == "o,")
				{
					wasLastOperator = true;
					while (true)
					{
						if (stack.empty())
							break;
						token = stack.back();
						if (token == "O(" || token == "O[" || token == "Og(" || token == "Oc[" || token == "O,")
							break;
						stack.pop_back();
						output.push_back(token);
					}
				} else if (str == "og(")
				{
					wasLastOperator = true;
					str.replace(0, 1, "O");
					stack.push_back(str);
				} else if(str == "o(" || str == "o[" || str == "oc[")
				{
					wasLastOperator = true;
					str.replace(0, 1, "O");
					output.push_back(str);
					stack.push_back(str);
				} else if ((oplevel = GetOperatorLevel((std::string) (str.substr((size_t) 1, (size_t) str.length() - 1)))) != 255)
				{
					str.replace(0, 1, "O");

					while (!stack.empty())
					{
						byte lastoplevel = GetOperatorLevel(stack.back().substr((size_t) 1, (size_t) stack.back().length() - 1));
						if (opdirection[oplevel] ? oplevel <= lastoplevel : (oplevel < lastoplevel))
							break;
						output.push_back(stack.back());
						stack.pop_back();
					}

					stack.push_back(str);
					wasLastOperator = true;
				} else
				{
					wasLastOperator = false;
					output.push_back(str);
				}
				if (str == "O++" || str == "O--")
				{
					wasLastOperator = false;
				}
			} else
			{
				wasLastOperator = false;
				output.push_back(str);
			}
		}

		while (!stack.empty())
		{
			output.push_back(stack.back());
			stack.pop_back();
		}

		tokens.clear();
		return output;
	}

	void
	InterpretStack(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<std::string> output, byte type, pointer parent)
	{
		std::vector<Pointer> objectStack;

		for (size_t i = 0; i < output.size(); i++)
		{
			std::string token = output.at(i);
			char c = *(token.data());
			token.replace(0, 1, "");
			if (c == 'o')
			{
				if (token == "new")
				{
					//TODO read next token - the value type, then check if the following token is (), arguments, then check if there is a {}
				} else if (token == "{")
				{
					//TODO create an object
				} else if (token == "throw")
				{

				} else if (token == "assert")
				{

				} else
				{
					objectStack.push_back(ParseToken(token, parent));
				}
			} else if (c == 'O')
			{
				if (token == "++")
				{
					Pointer var1 = objectStack.back();
					objectStack.pop_back();
					//if (var1.isInMemory)
					{
						byte size;
						if (var1.type > 1 && var1.type < 10)
						{
							switch (var1.type)
							{
								case 2:
								case 6:
									(*((byte8 *) var1.loc))++;
									break;
								case 3:
								case 7:
									(*((short16 *) var1.loc))++;
									break;
								case 4:
								case 8:
									(*((int32 *) var1.loc))++;
									break;
								case 5:
								case 9:
									(*((long64 *) var1.loc))++;
									break;
							}
						} else if (var1.type > 9 && var1.type < 16)
						{
							size = 2 << ((var1.type - 10) % 3 + 2);
						} else
						{
							//TODO call the operator
						}
					//} else
					//{
					//	//TODO just return the number + 1
					}
				} else if (token == "--")
				{

				} else if (token == ".")
				{

				} else if (token == "[")
				{

				} else if (token == "(")
				{

				} else if (token == "pre++")
				{

				} else if (token == "pre--")
				{

				} else if (token == "u+")
				{

				} else if (token == "u-")
				{

				} else if (token == "~")
				{

				} else if (token == "!")
				{

				} else if (token == "a&")
				{

				} else if (token == "p*")
				{

				} else if (token == "sizeof")
				{

				} else if (token == ".*")
				{

				} else if (token == "^")
				{

				} else if (token == "*")
				{

				} else if (token == "/")
				{

				} else if (token == "%")
				{

				} else if (token == "+")
				{

				} else if (token == "-")
				{

				} else if (token == "<<")
				{

				} else if (token == ">>")
				{

				} else if (token == "<")
				{

				} else if (token == "<=")
				{

				} else if (token == ">")
				{

				} else if (token == ">=")
				{

				} else if (token == "==")
				{

				} else if (token == "===")
				{

				} else if (token == "!=")
				{

				} else if (token == "!==")
				{

				} else if (token == "&")
				{

				} else if (token == "!&")
				{

				} else if (token == "|")
				{

				} else if (token == "!|")
				{

				} else if (token == "&&")
				{

				} else if (token == "!&")
				{

				} else if (token == "||")
				{

				} else if (token == "!|")
				{

				} else if (token == "?:")
				{

				} else if (token == ":" || token == "=")
				{

				} else if (token == "+:" || token == "+=")
				{

				} else if (token == "-:" || token == "-=")
				{

				} else if (token == "*:" || token == "*=")
				{

				} else if (token == "/:" || token == "/=")
				{

				} else if (token == "%:" || token == "%=")
				{

				} else if (token == "^:" || token == "^=")
				{

				} else if (token == "<<:" || token == "<<=")
				{

				} else if (token == ">>:" || token == ">>=")
				{

				} else if (token == "&:" || token == "&=")
				{

				} else if (token == "|:" || token == "|=")
				{

				} else if (token == "c[")
				{

				} else if (token == "g(")
				{

				} else
				{

				}
			} else if (c == 'p')
			{
				unsigned long int p = token.data()[0] << 24 | token.data()[1] << 16 | token.data()[2] << 8 | token.data()[3];
				//TODO parse
			}
		}
	}

	void //TODO untested :/
	ParseValue(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, byte type, pointer parent)
	{
		std::vector<std::string> tokens = Tokenize(p, str, line, column, includedFrom);
		std::vector<std::string> output = CalculatePrecedence(p, str, line, column, includedFrom, tokens);
		InterpretStack(p, str, line, column, includedFrom, output, type, parent);
		//return 9000;
	}

	void
	ParseBrackets(unsigned long int * p, std::string str, unsigned int * line, unsigned int * column, std::string * includedFrom, std::vector<Variable> & variables)
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
			else if (c2 == '\n')
			{
				(*line)++;
				*column = 1;
			} else if (c2 == '	')
			{
				*column += 3;
			} else
			{
				(*p)--;
				(*column)--;
				byte returnval = 0;
				Variable v = ParseName(p, str, line, column, includedFrom, &returnval);
				if (!v.isNull)
					SkipValue(p, str, column, line, includedFrom);
				variables.push_back(v);
			}
		}
	}
}

