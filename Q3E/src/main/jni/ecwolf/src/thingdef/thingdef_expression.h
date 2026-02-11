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

// ECWolf Note: This is a modified version of ACC++'s expression evaluator.
// I was tempted to try to generalize it for a libaccpp.a, but it would
// probably end up either being a.) too much work, or b.) add unneeded bloat to
// ACC++ itself.

#ifndef __EXPRESSION_H__
#define __EXPRESSION_H__

#include "zstring.h"

struct ExpressionOperator;
class ClassDef;
class Scanner;
class Symbol;
class Type;
class TypeHierarchy;
class FRandom;

class ExpressionNode
{
	public:
		class Value
		{
			public:
				void PerformOperation(const Value *other, const ExpressionOperator &op);

				const Value &operator=(int64_t val) { i = val; d = static_cast<double> (val); isDouble = false; return *this; }
				const Value &operator=(double val) { i = static_cast<int64_t>(val); d = val; isDouble = true; return *this; }

				int64_t GetInt() const { return isDouble ? static_cast<int64_t>(d) : i; }
				double GetDouble() const { return isDouble ? d : static_cast<double>(i); }

			private:
				bool isDouble;
				int64_t	i;
				double	d;
		};

		~ExpressionNode();

		const Value &Evaluate(AActor *self);
		//void	DumpExpression(std::stringstream &out, std::string endLabel=std::string()) const;

		static ExpressionNode	*ParseExpression(const ClassDef *cls, TypeHierarchy &types, Scanner &sc, ExpressionNode *root=NULL, unsigned char opLevel=255);
	protected:
		Value evaluation;

		enum ValueType
		{
			CONSTANT,	// Resolved at compile time
			IDENTIFIER,	// Resolved at assembly time
			SYMBOL,		// Resolved at compile time, non-constant
			STRING
		};

		ExpressionNode(ExpressionNode *parent=NULL);

		const Type	*GetType() const;

		const ExpressionOperator	*op;
		ExpressionNode				*term[2];
		//ExpressionNode				*subscript;
		FRandom						*subscript; // Since we don't have any array variables yet.
		ExpressionNode*				*args;
		ExpressionNode				*parent;

		ValueType					type;
		const Type					*classType;
		Value						value;
		FString						str;
		FString						identifier;
		Symbol						*symbol;
};

#endif /* __EXPRESSION_H__ */
