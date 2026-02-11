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

#include "m_random.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_expression.h"
#include "scanner.h"
#include "thingdef/thingdef_type.h"
#include "wl_def.h"

static const struct ExpressionOperator
{
	unsigned char		token;
	unsigned char		density;
	const char* const	instruction;
	unsigned char		operands;
	const char* const	function;
	bool				reqSymbol; // Operate only on variables.
} operators[] =
{
	// List should start with a NOP and end with a NOP.
	{ '\0', 0, "", 0, "", false },

//	{ TK_Increment,		2,	"",					1,	"operator--",	true },
//	{ TK_Decrement,		2,	"",					1,	"operator++",	true },
	{ '+',				3,	"",					1,	"operator+",	false },
	{ '-',				3,	"unaryminus",		1,	"operator-",	false },
	{ '!',				3,	"negatelogical",	1,	"operator!",	false },
	{ '~',				3,	"negatebinary",		1,	"operator-",	false },
#define BINARYOPSTART 5
	{ '*',				5,	"multiply",			2,	"operator*",	false },
	{ '/',				5,	"divide",			2,	"operator/",	false },
	{ '%',				5,	"modulus",			2,	"operator%",	false },
	{ '+',				6,	"add",				2,	"operator+",	false },
	{ '-',				6,	"subtract",			2,	"operator-",	false },
	{ TK_ShiftLeft,		7,	"lshift",			2,	"operator<<",	false },
	{ TK_ShiftRight,	7,	"rshift",			2,	"operator>>",	false },
	{ '<',				8,	"lt",				2,	"operator<",	false },
	{ TK_LessEq,		8,	"le",				2,	"operator<=",	false },
	{ '>',				8,	"gt",				2,	"operator>",	false },
	{ TK_GtrEq,			8,	"ge",				2,	"operator>=",	false },
	{ TK_EqEq,			9,	"eq",				2,	"operator==",	false },
	{ TK_NotEq,			9,	"ne",				2,	"operator!=",	false },
	{ '&',				10,	"andbitwise",		2,	"operator&",	false },
	{ '^',				11,	"eorbitwise",		2,	"operator^",	false },
	{ '|',				12,	"orbitwise",		2,	"operator|",	false },
	{ TK_AndAnd,		13,	"andlogical",		2,	"operator&&",	false },
	{ TK_OrOr,			14,	"orlogical",		2,	"operator||",	false },
//	{ '?',				15,	"ifnotgoto",		3,	"",				false }, // Special handling
//	{ '=',				16,	"",					2,	"operator=",	true },
//	{ TK_AddEq,			16,	"",					2,	"operator+=",	true },
//	{ TK_SubEq,			16,	"",					2,	"operator-=",	true },
//	{ TK_MulEq,			16,	"",					2,	"operator*=",	true },
//	{ TK_DivEq,			16,	"",					2,	"operator/=",	true },
//	{ TK_ModEq,			16,	"",					2,	"operator%=" ,	true },
//	{ TK_ShiftLeftEq,	16,	"",					2,	"operator<<=",	true },
//	{ TK_ShiftRightEq,	16,	"",					2,	"operator>>=",	true },
//	{ TK_AndEq,			16,	"",					2,	"operator&=",	true },
//	{ TK_OrEq,			16,	"",					2,	"operator|=",	true },
//	{ TK_XorEq,			16,	"",					2,	"operator^=",	true },

	{ '\0', 255, "", 0, "", false }
};

void ExpressionNode::Value::PerformOperation(const ExpressionNode::Value *other, const ExpressionOperator &op)
{
	if(op.operands > 1)
	{
		if(!isDouble)
		{
			isDouble = other->isDouble;
			d = static_cast<double>(i);
		}

		switch(op.token)
		{
			default:
				break;
			case '*':
				if(isDouble) d *= other->GetDouble();
				else i *= other->GetInt();
				break;
			case '/':
				if(isDouble) d /= other->GetDouble();
				else
				{
					isDouble = true;
					d = double(i)/other->GetInt();
				}
				break;
			case '%':
				if(isDouble)
				{
					isDouble = false;
					i = int64_t(d)%other->GetInt();
				}
				else i %= other->GetInt();
				break;
			case '+':
				if(isDouble) d += other->GetDouble();
				else i += other->GetInt();
				break;
			case '-':
				if(isDouble) d -= other->GetDouble();
				else i -= other->GetInt();
				break;
			case TK_ShiftLeft:
				if(isDouble)
				{
					isDouble = false;
					i = int64_t(d)<<other->GetInt();
				}
				else i <<= other->GetInt();
				break;
			case TK_ShiftRight:
				if(isDouble)
				{
					isDouble = false;
					i = int64_t(d)>>other->GetInt();
				}
				else i >>= other->GetInt();
				break;
			case '<':
				if(isDouble)
				{
					isDouble = false;
					i = d < other->GetDouble();
				}
				else i = i < other->GetInt();
				break;
			case TK_LessEq:
				if(isDouble)
				{
					isDouble = false;
					i = d <= other->GetDouble();
				}
				else i = i <= other->GetInt();
				break;
			case '>':
				if(isDouble)
				{
					isDouble = false;
					i = d > other->GetDouble();
				}
				else i = i > other->GetInt();
				break;
			case TK_GtrEq:
				if(isDouble)
				{
					isDouble = false;
					i = d >= other->GetDouble();
				}
				else i = i >= other->GetInt();
				break;
			case TK_EqEq:
				if(isDouble)
				{
					isDouble = false;
					i = d == other->GetDouble();
				}
				else i = i == other->GetInt();
				break;
			case TK_NotEq:
				if(isDouble)
				{
					isDouble = false;
					i = d != other->GetDouble();
				}
				else i = i != other->GetInt();
				break;
			case '&':
				if(isDouble)
				{
					isDouble = false;
					i = int64_t(d)&other->GetInt();
				}
				else i &= other->GetInt();
				break;
			case '|':
				if(isDouble)
				{
					isDouble = false;
					i = int64_t(d)|other->GetInt();
				}
				else i |= other->GetInt();
				break;
			case '^':
				if(isDouble)
				{
					isDouble = false;
					i = int64_t(d)^other->GetInt();
				}
				else i ^= other->GetInt();
				break;
			case TK_AndAnd:
				if(isDouble)
				{
					isDouble = false;
					i = d && other->GetDouble();
				}
				else i = i && other->GetInt();
				break;
			case TK_OrOr:
				if(isDouble)
				{
					isDouble = false;
					i = d || other->GetDouble();
				}
				else i = i || other->GetInt();
				break;
		}
	}
	else
	{
		switch(op.token)
		{
			default:
				break;
			case '-':
				if(isDouble) d *= -1;
				else i *= -1;
				break;
			case '!':
				if(isDouble)
				{
					isDouble = false;
					i = !d;
				}
				else i = !i;
				break;
			case '~':
				if(isDouble)
				{
					isDouble = false;
					i = ~int64_t(d);
				}
				else i = ~i;
				break;
		}
	}
}

ExpressionNode::ExpressionNode(ExpressionNode *parent) : op(&operators[0]), parent(parent), type(CONSTANT), classType(NULL)
{
	term[0] = term[1] = /* term[2] =*/ NULL;
	subscript = NULL;
}

ExpressionNode::~ExpressionNode()
{
	for(unsigned char i = 0;i < 2;i++)
		delete term[i];
	//delete subscript;

	if(type == SYMBOL && symbol->IsFunction())
	{
		FunctionSymbol *fsymbol = static_cast<FunctionSymbol *>(symbol);
		for(unsigned int i = 0;i < fsymbol->GetNumArgs();++i)
			delete args[i];
		delete[] args;
	}
}

const ExpressionNode::Value &ExpressionNode::Evaluate(AActor *self)
{
	if(term[0] == NULL)
	{
		if(type == CONSTANT)
			evaluation = value;
		else if(type == SYMBOL)
		{
			if(symbol->IsFunction())
			{
				FunctionSymbol *fsymbol = static_cast<FunctionSymbol *>(symbol);
				fsymbol->CallFunction(self, evaluation, args, subscript);
			}
			else
				symbol->FillValue(evaluation, self);
		}
		else
		{
			// At this time this shouldn't happen...
			assert(type == SYMBOL || type == CONSTANT);
		}
	}
	else
	{
		term[0]->Evaluate(self);
		evaluation = term[0]->evaluation;
	}

	if(op->token == TK_OrOr && evaluation.GetInt())
		return (evaluation = int64_t(1));
	else if(op->token == TK_AndAnd && !evaluation.GetInt())
		return (evaluation = int64_t(0));

	if(op->operands > 1 && term[1] != NULL)
	{
		term[1]->Evaluate(self);

		evaluation.PerformOperation(&term[1]->evaluation, *op);
	}
	else
		evaluation.PerformOperation(NULL, *op);
	return evaluation;
}

const Type *ExpressionNode::GetType() const
{
	bool isOp = op->token != '\0';
	TypeRef argumentTypes[2] = { TypeRef(classType), TypeRef(NULL) };
	if(isOp && op->operands > 1)
		argumentTypes[1] = TypeRef(term[1]->GetType());

	if(classType == NULL && term[0] != NULL)
	{
		argumentTypes[0] = TypeRef(term[0]->GetType());
	}
	return argumentTypes[0].GetType();
}

ExpressionNode *ExpressionNode::ParseExpression(const ClassDef *cls, TypeHierarchy &types, Scanner &sc, ExpressionNode *root, unsigned char opLevel)
{
	// We can't back out of our level in this recursion
	unsigned char initialLevel = opLevel;
	if(root == NULL)
		root = new ExpressionNode();

	ExpressionNode *thisNode = root;
	// We're going to basically alternate term/operator in this loop.
	bool awaitingTerm = true;
	do
	{
		if(awaitingTerm)
		{
			// Check for unary operators
			bool foundUnary = false;
			// We can check the density here since the only lower operators are
			// increment/decrement which I would think should always come before a variable so...
			for(unsigned char i = 1;operators[i].operands == 1 && operators[i].density <= opLevel;i++)
			{
				if(sc.CheckToken(operators[i].token))
				{
					foundUnary = true;
					thisNode->term[0] = new ExpressionNode(thisNode);
					thisNode->term[0]->op = &operators[i];
					ParseExpression(cls, types, sc, thisNode->term[0], operators[i].density);
					awaitingTerm = false;
					break;
				}
			}
			if(foundUnary)
				continue;

			// Remember parenthesis are just normal terms.  So recurse.
			if(sc.CheckToken('('))
			{
				thisNode->term[0] = new ExpressionNode(thisNode);
				ParseExpression(cls, types, sc, thisNode->term[0]);
				sc.MustGetToken(')');
			}
			else if(sc.CheckToken(TK_IntConst))
			{
				thisNode->classType = types.GetType(TypeHierarchy::INT);
				thisNode->value = (int64_t)sc->number;
			}
			else if(sc.CheckToken(TK_FloatConst))
			{
				thisNode->classType = types.GetType(TypeHierarchy::FLOAT);
				thisNode->value = sc->decimal;
			}
			else if(sc.CheckToken(TK_BoolConst))
			{
				thisNode->classType = types.GetType(TypeHierarchy::BOOL);
				thisNode->value = (int64_t)(sc->boolean ? 1 : 0);
			}
			/*else if(sc.CheckToken(TK_StringConst))
			{
				thisNode->type = STRING;
				thisNode->classType = types.GetType(TypeHierarchy::STRING);
				thisNode->str = sc->str;
			}*/
			else if(sc.CheckToken(TK_Identifier))
			{
				Symbol *symbol = cls->FindSymbol(sc->str);
				if(symbol == NULL)
					sc.ScriptMessage(Scanner::ERROR, "Undefined symbol `%s`.", sc->str.GetChars());
				thisNode->type = SYMBOL;
				thisNode->classType = symbol->GetType();
				thisNode->symbol = symbol;

				if(sc.CheckToken('['))
				{
					if(symbol->IsArray())
					{
						sc.MustGetToken(TK_Identifier);
						thisNode->subscript = FRandom::StaticFindRNG(sc->str);
						sc.MustGetToken(']');
					}
					else
						sc.ScriptMessage(Scanner::ERROR, "Symbol is not a valid array.");
				}

				if(symbol->IsFunction())
				{
					if(thisNode->subscript == NULL && symbol->IsArray())
					{
						static FRandom pr_exrandom("Expression");
						thisNode->subscript = &pr_exrandom;
					}
					assert(thisNode->subscript);
					sc.MustGetToken('(');
					FunctionSymbol *fsymbol = static_cast<FunctionSymbol *>(symbol);
					thisNode->args = new ExpressionNode*[fsymbol->GetNumArgs()];
					unsigned short argc = 0;
					do
					{
						thisNode->args[argc++] = ExpressionNode::ParseExpression(cls, types, sc, NULL);
					}
					while(sc.CheckToken(','));
					sc.MustGetToken(')');

					if(argc != fsymbol->GetNumArgs())
						sc.ScriptMessage(Scanner::ERROR, "Incorrect number of args for function call.\n");
				}
			}
			else
				sc.ScriptMessage(Scanner::ERROR, "Expected expression term.");
			awaitingTerm = false;
		}
		else
		{
			// ternary operator support.
			/*if(thisNode->parent != NULL && thisNode->parent->op->operands == 3 && thisNode->parent->term[2] == NULL)
			{
				sc.MustGetToken(':');
				thisNode = thisNode->parent->term[2] = new ExpressionNode(thisNode);
				awaitingTerm = true;
			}
			else*/
			{
				// Go through the operators list until we either hit the end or an
				// operator that has too high of a priority (what I'm calling density)
				for(unsigned char i = BINARYOPSTART;operators[i].token != '\0';i++)
				{
					if(sc.CheckToken(operators[i].token))
					{
						if(operators[i].reqSymbol && thisNode->type != SYMBOL)
							sc.ScriptMessage(Scanner::ERROR, "Operation only valid on variables.");

						if(operators[i].density <= opLevel)
						{
							opLevel = operators[i].density;
							thisNode->op = &operators[i];
							thisNode = thisNode->term[1] = new ExpressionNode(thisNode);
							awaitingTerm = true;
						}
						else
						{
							// Go up the tree to find where we can insert this operation
							ExpressionNode *floatCheck = thisNode;
							ExpressionNode **floatRoot = &thisNode->parent;
							bool rightSide = *floatRoot ? (*floatRoot)->term[1] == floatCheck : false;
							while(*floatRoot && operators[i].density > (*floatRoot)->op->density)
							{
								floatCheck = *floatRoot;
								floatRoot = &(*floatRoot)->parent;
								rightSide = *floatRoot ? (*floatRoot)->term[1] == floatCheck : false;
							}

							// We need to relink our tree to fit the new node.
							ExpressionNode *newRoot = new ExpressionNode(*floatRoot);
							if(*floatRoot != NULL)
								(*floatRoot)->term[rightSide] = newRoot;
							floatCheck->parent = newRoot;
							newRoot->term[0] = floatCheck;
							newRoot->term[1] = new ExpressionNode(newRoot);
							newRoot->op = &operators[i];
							ParseExpression(cls, types, sc, newRoot->term[1], operators[i].density);
						}
						break;
					}
				}
			}

			// Are we done with the recursion?
			if(!awaitingTerm)
				break;
		}
	}
	while(true);

	return root;
}
