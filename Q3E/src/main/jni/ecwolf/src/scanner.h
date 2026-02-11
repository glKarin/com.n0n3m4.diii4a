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

#ifndef __SCANNER_H__
#define __SCANNER_H__

#include "scanner_support.h"

enum
{
	TK_Identifier,		// Ex: SomeIdentifier
	TK_StringConst,		// Ex: "Some String"
	TK_IntConst,		// Ex: 27
	TK_FloatConst,		// Ex: 1.5
	TK_BoolConst,		// Ex: true
	TK_AndAnd,			// &&
	TK_OrOr,			// ||
	TK_EqEq,			// ==
	TK_NotEq,			// !=
	TK_GtrEq,			// >=
	TK_LessEq,			// <=
	TK_ShiftLeft,		// <<
	TK_ShiftRight,		// >>
	TK_Increment,		// ++
	TK_Decrement,		// --
	TK_PointerMember,	// ->
	TK_ScopeResolution,	// ::
	TK_MacroConcat,		// ##
	TK_AddEq,			// +=
	TK_SubEq,			// -=
	TK_MulEq,			// *=
	TK_DivEq,			// /=
	TK_ModEq,			// %=
	TK_ShiftLeftEq,		// <<=
	TK_ShiftRightEq,	// >>=
	TK_AndEq,			// &=
	TK_OrEq,			// |=
	TK_XorEq,			// ^=
	TK_Ellipsis,		// ...

	TK_NumSpecialTokens,

	TK_NoToken = -1
};

class Scanner
{
	public:
		enum MessageLevel
		{
			ERROR,
			WARNING,
			NOTICE
		};

		struct Position
		{
			SCString scriptIdentifier;
			unsigned int tokenLine, tokenLinePosition;

			void ScriptMessage(MessageLevel level, const char* error, ...) const;
		};

		struct ParserState
		{
			SCString		str;
			int				number;
			double			decimal;
			bool			boolean;
			char			token;
			unsigned int	tokenLine;
			unsigned int	tokenLinePosition;
			unsigned int	scanPos;
		};

		Scanner(int lump);
		Scanner(const char* data, size_t length=0);
		~Scanner();

		void			CheckForMeta();
		void			CheckForWhitespace();
		bool			CheckToken(char token);
		void			ExpandState();
		const char*		GetData() const { return data; }
		Position		GetPosition() const { Position pos = { scriptIdentifier, GetLine(), GetLinePos() }; return pos; }
		unsigned int	GetLine() const { return state.tokenLine; }
		unsigned int	GetLinePos() const { return state.tokenLinePosition; }
		unsigned int	GetLogicalPos() const { return logicalPosition; }
		unsigned int	GetScanPos() const { return scanPos; }
		bool			GetNextString();
		bool			GetNextToken(bool expandState=true);
		void			MustGetToken(char token);
		void			Rewind(); // Only can rewind one step.
		void			ScriptMessage(MessageLevel level, const char* error, ...) const;
		void			SetScriptIdentifier(const SCString &ident) { scriptIdentifier = ident; }
		int				SkipLine();
		bool			TokensLeft() const;
		const ParserState &operator*() const { return state; }
		const ParserState *operator->() const { return &state; }

		static const SCString	&Escape(SCString &str);
		static const SCString	Escape(const char *str);
		static const SCString	&Unescape(SCString &str);

		static void		SetMessageHandler(void (*handler)(MessageLevel, const char*, va_list));


		ParserState		state;

	protected:
		void	IncrementLine();

	private:
		ParserState		nextState, prevState;

		char*	data;
		size_t	length;

		unsigned int	line;
		unsigned int	lineStart;
		unsigned int	logicalPosition;
		unsigned int	scanPos;

		bool			needNext; // If checkToken returns false this will be false.

		SCString		scriptIdentifier;
};

#endif /* __SCANNER_H__ */
