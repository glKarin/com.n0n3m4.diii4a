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

#ifndef __TYPE_H__
#define __TYPE_H__

#include "wl_def.h"
#include "name.h"
#include "tarray.h"
#include "zstring.h"
#include "thingdef/thingdef_expression.h"

class TypeRef;
class TypeHierarchy;

class Type
{
	public:
		enum TypeStatus
		{
			FORWARD,
			STRUCTURE
		};

		const FName		&GetName() const { return name; }
		unsigned int	GetSize() const { return 1; }
		bool			IsForwardDeclared() const { return status == FORWARD; }
		bool			IsKindOf(const Type *other) const;

	protected:
		friend class TypeHierarchy;

		Type(const FName &name, const Type *parent);

		const Type	*parent;

		TypeStatus	status;
		FName		name;
};

class TypeRef
{
	public:
		TypeRef(const Type *type=NULL) : type(type) {}

		const Type	*GetType() const { return type; }
		bool		operator==(const TypeRef &other) const { return GetType() == other.GetType(); }
		bool		operator!=(const TypeRef &other) const { return GetType() != other.GetType(); }
	protected:
		const Type	*type;
};

class TypeHierarchy
{
	public:
		static TypeHierarchy staticTypes;

		enum PrimitiveTypes
		{
			VOID,
			STRING,
			BOOL,
			INT,
			FLOAT,
			STATE,
			ANGLE_T,

			NUM_TYPES
		};

		TypeHierarchy();

		Type		*CreateType(const FName &name, const Type *parent);
		const Type	*GetType(PrimitiveTypes type) const;
		const Type	*GetType(const FName &name) const;

	protected:
		typedef TMap<FName, Type> TypeMap;

		TypeMap	types;
};

class Symbol
{
	public:
		Symbol(const FName &name, const TypeRef &type);
		virtual ~Symbol() {}

		virtual void	FillValue(ExpressionNode::Value &val, AActor *self=NULL) const=0;
		FName			GetName() const { return name; }
		const Type		*GetType() const { return type.GetType(); }
		virtual bool	IsArray() const { return false; }
		virtual bool	IsFunction() const { return false; }
	protected:
		FName			name;
		TypeRef			type;
};

class ConstantSymbol : public Symbol
{
	public:
		ConstantSymbol(const FName &name, const TypeRef &type, const ExpressionNode::Value &value);

		void FillValue(ExpressionNode::Value &val, AActor *self=NULL) const
		{
			val = this->val;
		}
	protected:
		ExpressionNode::Value	val;
};

class FRandom;
class FunctionSymbol : public Symbol
{
	public:
		typedef void (*ExprFunction) (AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng);

		FunctionSymbol(const FName &name, const TypeRef &ret, unsigned short args, ExprFunction function, bool takesRNG);

		void CallFunction(AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng) const;
		void FillValue(ExpressionNode::Value &val, AActor *self=NULL) const;
		unsigned short GetNumArgs() const { return args; }
		bool IsArray() const { return takesRNG; }
		bool IsFunction() const { return true; }

	protected:
		unsigned short	args;
		bool			takesRNG;
		ExprFunction	function;
};

class VariableSymbol : public Symbol
{
	public:
		VariableSymbol(const FName &var, const TypeRef &type, const int offset);

		void FillValue(ExpressionNode::Value &val, AActor *self=NULL) const;
	protected:
		const int	offset;
};

#endif /* __TYPE_H__ */
