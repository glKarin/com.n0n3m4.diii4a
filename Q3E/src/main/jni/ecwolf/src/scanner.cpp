/*
** Copyright (c) 2010, Braden "Blzut3" Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * The names of its contributors may be used to endorse or promote
**       products derived from this software without specific prior written
**       permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
** INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstdlib>
#include <cstdio>
#include <cstring>
/* On QNX/Blackberry cstdarg is broken, on the other hand stdarg.h is
   universal, hence let's use stdarg.h.  */
#include <stdarg.h>

#include "scanner_support.h"
#include "scanner.h"
#include "w_wad.h"

static void DefaultMessageHandler(Scanner::MessageLevel, const char* error, va_list list)
{
	vfprintf(stderr, error, list);
}

static void (*ScriptMessageHandler)(Scanner::MessageLevel, const char*, va_list) = DefaultMessageHandler;
static void DoScriptMessage(const Scanner::Position &pos, Scanner::MessageLevel level, const char* error, va_list args)
{
	const char* messageLevel;
	switch(level)
	{
		default:
			messageLevel = "Notice";
			break;
		case Scanner::WARNING:
			messageLevel = "Warning";
			break;
		case Scanner::ERROR:
			messageLevel = "Error";
			break;
	}

	char* newMessage = new char[strlen(error) + SCString_Len(pos.scriptIdentifier) + 25];
	sprintf(newMessage, "%s:%d:%d:%s: %s\n", SCString_GetChars(pos.scriptIdentifier), pos.tokenLine, pos.tokenLinePosition, messageLevel, error);
	ScriptMessageHandler(level, newMessage, args);
	delete[] newMessage;

	if(ScriptMessageHandler == DefaultMessageHandler && level == Scanner::ERROR)
		exit(0);
}

static const char* const TokenNames[TK_NumSpecialTokens] =
{
	"Identifier",
	"String Constant",
	"Integer Constant",
	"Float Constant",
	"Boolean Constant",
	"Logical And",
	"Logical Or",
	"Equals",
	"Not Equals",
	"Greater Than or Equals"
	"Less Than or Equals",
	"Left Shift",
	"Right Shift",
	"Increment",
	"Decrement",
	"Pointer Member",
	"Scope Resolution",
	"Macro Concatenation",
	"Assign Sum",
	"Assign Difference",
	"Assign Product",
	"Assign Quotient",
	"Assign Modulus",
	"Assign Left Shift",
	"Assign Right Shift",
	"Assign Bitwise And",
	"Assign Bitwise Or",
	"Assign Exclusive Or",
	"Ellipsis"
};

Scanner::Scanner(int lumpNum)
: line(1), lineStart(0), logicalPosition(0), scanPos(0), needNext(true)
{
	FMemLump lump = Wads.ReadLump(lumpNum);
	length = lump.GetSize();
	this->data = new char[length];
	memcpy(this->data, lump.GetMem(), length);

	SetScriptIdentifier(Wads.GetLumpFullPath(lumpNum));

	CheckForWhitespace();

	state.scanPos = scanPos;
	state.tokenLine = 0;
	state.tokenLinePosition = 0;
}

Scanner::Scanner(const char* data, size_t length)
: line(1), lineStart(0), logicalPosition(0), scanPos(0), needNext(true)
{
	if(length == 0 && *data != 0)
		length = strlen(data);
	this->length = length;
	this->data = new char[length];
	memcpy(this->data, data, length);

	CheckForWhitespace();

	state.scanPos = scanPos;
	state.tokenLine = 0;
	state.tokenLinePosition = 0;
}

Scanner::~Scanner()
{
	delete[] data;
}

// Here's my answer to the preprocessor screwing up line numbers. What we do is
// after a new line in CheckForWhitespace, look for a comment in the form of
// "/*meta:filename:line*/"
void Scanner::CheckForMeta()
{
	if(scanPos+10 < length)
	{
		char metaCheck[8];
		memcpy(metaCheck, data+scanPos, 7);
		metaCheck[7] = 0;
		if(strcmp(metaCheck, "/*meta:") == 0)
		{
			scanPos += 7;
			int metaStart = scanPos;
			int fileLength = 0;
			int lineLength = 0;
			while(scanPos < length)
			{
				char thisChar = data[scanPos];
				char nextChar = scanPos+1 < length ? data[scanPos+1] : 0;
				if(thisChar == '*' && nextChar == '/')
				{
					lineLength = scanPos-metaStart-1-fileLength;
					scanPos += 2;
					break;
				}
				if(thisChar == ':' && fileLength == 0)
					fileLength = scanPos-metaStart;
				scanPos++;
			}
			if(fileLength > 0 && lineLength > 0)
			{
				SetScriptIdentifier(SCString(data+metaStart, fileLength));
				SCString lineNumber(data+metaStart+fileLength+1, lineLength);
				line = atoi(SCString_GetChars(lineNumber));
				lineStart = scanPos;
			}
		}
	}
}

void Scanner::CheckForWhitespace()
{
	int comment = 0; // 1 = till next new line, 2 = till end block
	while(scanPos < length)
	{
		char cur = data[scanPos];
		char next = scanPos+1 < length ? data[scanPos+1] : 0;
		if(comment == 2)
		{
			if(cur != '*' || next != '/')
			{
				if(cur == '\n' || cur == '\r')
				{
					scanPos++;
					if(comment == 1)
						comment = 0;

					// Do a quick check for Windows style new line
					if(cur == '\r' && next == '\n')
						scanPos++;
					IncrementLine();
				}
				else
					scanPos++;
			}
			else
			{
				comment = 0;
				scanPos += 2;
			}
			continue;
		}

		if(cur == ' ' || cur == '\t' || cur == 0)
			scanPos++;
		else if(cur == '\n' || cur == '\r')
		{
			scanPos++;
			if(comment == 1)
				comment = 0;

			// Do a quick check for Windows style new line
			if(cur == '\r' && next == '\n')
				scanPos++;
			IncrementLine();
			CheckForMeta();
		}
		else if(cur == '/' && comment == 0)
		{
			switch(next)
			{
				case '/':
					comment = 1;
					break;
				case '*':
					comment = 2;
					break;
				default:
					return;
			}
			scanPos += 2;
		}
		else
		{
			if(comment == 0)
				return;
			else
				scanPos++;
		}
	}
}

bool Scanner::CheckToken(char token)
{
	if(needNext)
	{
		if(!GetNextToken(false))
			return false;
	}

	// An int can also be a float.
	if(nextState.token == token || (nextState.token == TK_IntConst && token == TK_FloatConst))
	{
		needNext = true;
		ExpandState();
		return true;
	}
	needNext = false;
	return false;
}

void Scanner::ExpandState()
{
	scanPos = nextState.scanPos;
	logicalPosition = scanPos;
	CheckForWhitespace();

	prevState = state;
	state = nextState;
}

bool Scanner::GetNextString()
{
	if(!needNext)
	{
		int prevLine = line;
		scanPos = state.scanPos;
		CheckForWhitespace();
		line = prevLine;
	}
	else
		CheckForWhitespace();

	nextState.tokenLine = line;
	nextState.tokenLinePosition = scanPos - lineStart;
	nextState.token = TK_NoToken;
	if(scanPos >= length)
		return false;

	unsigned int start = scanPos;
	unsigned int end = scanPos;
	bool quoted = data[scanPos] == '"';
	if(quoted) // String Constant
	{
		end = ++start; // Remove starting quote
		scanPos++;
		while(scanPos < length)
		{
			char cur = data[scanPos];
			if(cur == '"')
				end = scanPos;
			else if(cur == '\\')
			{
				scanPos += 2;
				continue;
			}
			scanPos++;
			if(start != end)
				break;
		}
	}
	else // Unquoted string
	{
		while(scanPos < length)
		{
			char cur = data[scanPos];
			switch(cur)
			{
				default:
					break;
				case ',':
					if(scanPos == start)
						break;
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					end = scanPos;
					break;
			}
			if(start != end)
				break;
			scanPos++;
		}

		if(scanPos == length)
			end = scanPos;
	}
	if(end-start > 0)
	{
		nextState.scanPos = scanPos;
		SCString thisString(data+start, end-start);
		if(quoted)
			Unescape(thisString);
		nextState.str = thisString;
		nextState.token = TK_StringConst;
		ExpandState();
		needNext = true;
		return true;
	}
	return false;
}

bool Scanner::GetNextToken(bool expandState)
{
	if(!needNext)
	{
		needNext = true;
		if(expandState)
			ExpandState();
		return true;
	}

	nextState.tokenLine = line;
	nextState.tokenLinePosition = scanPos - lineStart;
	nextState.token = TK_NoToken;
	if(scanPos >= length)
	{
		if(expandState)
			ExpandState();
		return false;
	}

	unsigned int start = scanPos;
	unsigned int end = scanPos;
	int integerBase = 10;
	bool floatHasDecimal = false;
	bool floatHasExponent = false;
	bool stringFinished = false; // Strings are the only things that can have 0 length tokens.

	char cur = data[scanPos++];
	// Determine by first character
	if(cur == '_' || (cur >= 'A' && cur <= 'Z') || (cur >= 'a' && cur <= 'z'))
		nextState.token = TK_Identifier;
	else if(cur >= '0' && cur <= '9')
	{
		if(cur == '0')
			integerBase = 8;
		nextState.token = TK_IntConst;
	}
	else if(cur == '.' && scanPos < length && data[scanPos] != '.')
	{
		floatHasDecimal = true;
		nextState.token = TK_FloatConst;
	}
	else if(cur == '"')
	{
		end = ++start; // Move the start up one character so we don't have to trim it later.
		nextState.token = TK_StringConst;
	}
	else
	{
		end = scanPos;
		nextState.token = cur;

		// Now check for operator tokens
		if(scanPos < length)
		{
			char next = data[scanPos];
			if(cur == '&' && next == '&')
				nextState.token = TK_AndAnd;
			else if(cur == '|' && next == '|')
				nextState.token = TK_OrOr;
			else if(
				(cur == '<' && next == '<') ||
				(cur == '>' && next == '>')
			)
			{
				// Next for 3 character tokens
				if(scanPos+1 > length && data[scanPos+1] == '=')
				{
					scanPos++;
					nextState.token = cur == '<' ? TK_ShiftLeftEq : TK_ShiftRightEq;
					
				}
				else
					nextState.token = cur == '<' ? TK_ShiftLeft : TK_ShiftRight;
			}
			else if(cur == '#' && next == '#')
				nextState.token = TK_MacroConcat;
			else if(cur == ':' && next == ':')
				nextState.token = TK_ScopeResolution;
			else if(cur == '+' && next == '+')
				nextState.token = TK_Increment;
			else if(cur == '-')
			{
				if(next == '-')
					nextState.token = TK_Decrement;
				else if(next == '>')
					nextState.token = TK_PointerMember;
			}
			else if(cur == '.' && next == '.' &&
				scanPos+1 < length && data[scanPos+1] == '.')
			{
				nextState.token = TK_Ellipsis;
				++scanPos;
			}
			else if(next == '=')
			{
				switch(cur)
				{
					case '=':
						nextState.token = TK_EqEq;
						break;
					case '!':
						nextState.token = TK_NotEq;
						break;
					case '>':
						nextState.token = TK_GtrEq;
						break;
					case '<':
						nextState.token = TK_LessEq;
						break;
					case '+':
						nextState.token = TK_AddEq;
						break;
					case '-':
						nextState.token = TK_SubEq;
						break;
					case '*':
						nextState.token = TK_MulEq;
						break;
					case '/':
						nextState.token = TK_DivEq;
						break;
					case '%':
						nextState.token = TK_ModEq;
						break;
					case '&':
						nextState.token = TK_AndEq;
						break;
					case '|':
						nextState.token = TK_OrEq;
						break;
					case '^':
						nextState.token = TK_XorEq;
						break;
					default:
						break;
				}
			}

			if(nextState.token != cur)
			{
				scanPos++;
				end = scanPos;
			}
		}
	}

	if(start == end)
	{
		while(scanPos < length)
		{
			cur = data[scanPos];
			switch(nextState.token)
			{
				default:
					break;
				case TK_Identifier:
					if(cur != '_' && (cur < 'A' || cur > 'Z') && (cur < 'a' || cur > 'z') && (cur < '0' || cur > '9'))
						end = scanPos;
					break;
				case TK_IntConst:
					if(cur == '.' || (scanPos-1 != start && cur == 'e'))
						nextState.token = TK_FloatConst;
					else if((cur == 'x' || cur == 'X') && scanPos-1 == start)
					{
						integerBase = 16;
						break;
					}
					else
					{
						switch(integerBase)
						{
							default:
								if(cur < '0' || cur > '9')
									end = scanPos;
								break;
							case 8:
								if(cur < '0' || cur > '7')
									end = scanPos;
								break;
							case 16:
								if((cur < '0' || cur > '9') && (cur < 'A' || cur > 'F') && (cur < 'a' || cur > 'f'))
									end = scanPos;
								break;
						}
						break;
					}
				case TK_FloatConst:
					if(cur < '0' || cur > '9')
					{
						if(!floatHasDecimal && cur == '.')
						{
							floatHasDecimal = true;
							break;
						}
						else if(!floatHasExponent && cur == 'e')
						{
							floatHasDecimal = true;
							floatHasExponent = true;
							if(scanPos+1 < length)
							{
								char next = data[scanPos+1];
								if((next < '0' || next > '9') && next != '+' && next != '-')
									end = scanPos;
								else
									scanPos++;
							}
							break;
						}
						end = scanPos;
					}
					break;
				case TK_StringConst:
					if(cur == '"')
					{
						stringFinished = true;
						end = scanPos;
						scanPos++;
					}
					else if(cur == '\\')
						scanPos++; // Will add two since the loop automatically adds one
					break;
			}
			if(start == end && !stringFinished)
				scanPos++;
			else
				break;
		}
		// Handle small tokens at the end of a file.
		if(scanPos == length && !stringFinished)
			end = scanPos;
	}

	nextState.scanPos = scanPos;
	if(end-start > 0 || stringFinished)
	{
		nextState.str = SCString(data+start, end-start);
		if(nextState.token == TK_FloatConst)
		{
			if(floatHasDecimal && SCString_Len(nextState.str) == 1)
			{
				// Don't treat a lone '.' as a decimal.
				nextState.token = '.';
			}
			else
			{
				nextState.decimal = atof(SCString_GetChars(nextState.str));
				nextState.number = static_cast<int> (nextState.decimal);
				nextState.boolean = (nextState.number != 0);
			}
		}
		else if(nextState.token == TK_IntConst)
		{
			nextState.number = strtol(SCString_GetChars(nextState.str), NULL, integerBase);
			nextState.decimal = nextState.number;
			nextState.boolean = (nextState.number != 0);
		}
		else if(nextState.token == TK_Identifier)
		{
			// Check for a boolean constant.
			if(SCString_Compare(nextState.str, "true") == 0)
			{
				nextState.token = TK_BoolConst;
				nextState.boolean = true;
			}
			else if(SCString_Compare(nextState.str, "false") == 0)
			{
				nextState.token = TK_BoolConst;
				nextState.boolean = false;
			}
		}
		else if(nextState.token == TK_StringConst)
		{
			Unescape(nextState.str);
		}
		if(expandState)
			ExpandState();
		return true;
	}
	nextState.token = TK_NoToken;
	if(expandState)
		ExpandState();
	return false;
}

void Scanner::IncrementLine()
{
	line++;
	lineStart = scanPos;
}

void Scanner::MustGetToken(char token)
{
	if(!CheckToken(token))
	{
		ExpandState();
		if(state.token == TK_NoToken)
			ScriptMessage(Scanner::ERROR, "Unexpected end of script.");
		else if(token < TK_NumSpecialTokens && state.token < TK_NumSpecialTokens)
			ScriptMessage(Scanner::ERROR, "Expected '%s' but got '%s' instead.", TokenNames[(int)token], TokenNames[(int)state.token]);
		else if(token < TK_NumSpecialTokens && state.token >= TK_NumSpecialTokens)
			ScriptMessage(Scanner::ERROR, "Expected '%s' but got '%c' instead.", TokenNames[(int)token], state.token);
		else if(token >= TK_NumSpecialTokens && state.token < TK_NumSpecialTokens)
			ScriptMessage(Scanner::ERROR, "Expected '%c' but got '%s' instead.", token, TokenNames[(int)state.token]);
		else
			ScriptMessage(Scanner::ERROR, "Expected '%c' but got '%c' instead.", token, state.token);
	}
}

void Scanner::Rewind()
{
	needNext = false;
	nextState = state;
	state = prevState;
	scanPos = state.scanPos;

	line = prevState.tokenLine;
	logicalPosition = prevState.tokenLinePosition;
}

void Scanner::ScriptMessage(MessageLevel level, const char* error, ...) const
{
	va_list list;
	va_start(list, error);
	DoScriptMessage(GetPosition(), level, error, list);
	va_end(list);
}

void Scanner::Position::ScriptMessage(MessageLevel level, const char* error, ...) const
{
	va_list list;
	va_start(list, error);
	DoScriptMessage(*this, level, error, list);
	va_end(list);
}

void Scanner::SetMessageHandler(void (*handler)(MessageLevel, const char*, va_list))
{
	ScriptMessageHandler = handler;
}

int Scanner::SkipLine()
{
	int ret = GetLogicalPos();
	while(logicalPosition < length)
	{
		char thisChar = data[logicalPosition];
		char nextChar = logicalPosition+1 < length ? data[logicalPosition+1] : 0;
		if(thisChar == '\n' || thisChar == '\r')
		{
			ret = logicalPosition++; // Return the first newline character we see.
			if(nextChar == '\r')
				logicalPosition++;
			IncrementLine();
			CheckForWhitespace();
			break;
		}
		logicalPosition++;
	}
	if(logicalPosition > scanPos)
	{
		scanPos = logicalPosition;
		CheckForWhitespace();
		needNext = true;
		logicalPosition = scanPos;
	}
	return ret;
}

bool Scanner::TokensLeft() const
{
	return scanPos < length;
}

// NOTE: Be sure that '\\' is the first thing in the array otherwise it will re-escape.
static char escapeCharacters[] = {'\\', '"', 'n', 0};
static char resultCharacters[] = {'\\', '"', '\n', 0};
const SCString &Scanner::Escape(SCString &str)
{
	for(unsigned int i = 0;escapeCharacters[i] != 0;i++)
	{
		// += 2 because we'll be inserting 1 character.
		for(SCString_Index p = 0;p < SCString_Len(str) && (p = SCString_IndexOf(str, resultCharacters[i], p)) != SCString_NPos(str);p += 2)
		{
			SCString_InsertChar(str, p, '\\');
		}
	}
	return str;
}
const SCString Scanner::Escape(const char *str)
{
	SCString tmp(str);
	Escape(tmp);
	return tmp;
}
const SCString &Scanner::Unescape(SCString &str)
{
	for(unsigned int i = 0;escapeCharacters[i] != 0;i++)
	{
		SCString sequence("\\");
		SCString_AppendChar(sequence, escapeCharacters[i]);
		for(SCString_Index p = 0;p < SCString_Len(str) && (p = SCString_IndexOfSeq(str, sequence, p)) != SCString_NPos(str);p++)
		{
			SCString_Unescape(str, SCString_IndexOfSeq(str, sequence, p), resultCharacters[i]);
		}
	}
	return str;
}
