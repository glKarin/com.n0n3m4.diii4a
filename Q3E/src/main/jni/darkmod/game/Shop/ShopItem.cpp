/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "ShopItem.h"

CShopItem::CShopItem() :
	id(""),
	name(""),
	description(""),
	cost(0),
	image(""),
	count(0),
	persistent(false),
	canDrop(false),
	dropped(0),			// tels (#2567) remember how many we dropped	
	stackable(false)	// grayman (#2376)
{}

CShopItem::CShopItem(const idStr& _id, const idStr& _name, const idStr& _description,
					 int _cost, const idStr& _image, int _count, bool _persistent, bool _canDrop, bool _stackable) : // grayman (#2376)
	id(_id),
	name(_name),
	description(_description),
	cost(_cost),
	image(_image),
	count(_count),
	persistent(_persistent),
	canDrop(_canDrop),
	dropped(0),
	stackable(_stackable) // grayman (#2376)
{}

CShopItem::CShopItem(const CShopItem& item, int _count, int _cost, bool _persistent) :
	id(item.id),
	name(item.name),
	description(item.description),
	cost(_cost == 0 ? item.cost : _cost),
	image(item.image),
	count(_count),
	persistent(_persistent == false ? item.persistent : _persistent),
	canDrop(item.canDrop),
	dropped(0),
	classNames(item.classNames),
	stackable(item.stackable) // grayman (#2376)
{}

const idStr& CShopItem::GetID() const {
	return this->id;
}

const idStr CShopItem::GetName() const {
	// Tels: If nec., translate the name
	return common->Translate( this->name );
}

const idStr CShopItem::GetDescription() const {
	// Tels: If nec., translate the description
	return common->Translate( this->description );
}

const idStringList& CShopItem::GetClassnames() const
{
	return classNames;
}

void CShopItem::AddClassname(const idStr& className)
{
	classNames.AddUnique(className);
}

const idStr& CShopItem::GetImage() const {
	return this->image;
}

int CShopItem::GetCost() {
	return this->cost;
}

int CShopItem::GetCount() {
	return this->count;
}

int CShopItem::GetDroppedCount() {
	return this->dropped;
}

bool CShopItem::GetPersistent() {
	return this->persistent;
}

bool CShopItem::GetCanDrop() {
	return this->canDrop;
}

void CShopItem::SetCanDrop(bool canDrop) {
	this->canDrop = canDrop;
}

// tels (#2567)
void CShopItem::Drop(void) {
	if (this->canDrop && this->count > 0)
	{
		this->dropped = this->count;
		this->count = 0;
	}
}

// tels (#2567)
void CShopItem::Undrop(void) {
	if (this->canDrop && this->dropped > 0)
	{
		this->count = this->dropped;
		this->dropped = 0;
	}
}

// grayman (#2376) - add stackable methods
bool CShopItem::GetStackable() {
	return this->stackable;
}

void CShopItem::SetStackable(bool stackable) {
	this->stackable = stackable;
}

void CShopItem::ChangeCount(int amount) {
	this->count += amount;
}

void CShopItem::Save(idSaveGame *savefile) const
{
	savefile->WriteString(id);
	savefile->WriteString(name);
	savefile->WriteString(description);

	savefile->WriteInt(cost);
	savefile->WriteString(image);
	savefile->WriteInt(count);
	savefile->WriteInt(dropped);
	savefile->WriteBool(persistent);
	savefile->WriteBool(canDrop);

	savefile->WriteInt(classNames.Num());
	for (int i = 0; i < classNames.Num(); ++i)
	{
		savefile->WriteString(classNames[i]);
	}
	savefile->WriteBool(stackable); // grayman (#2376)
}

void CShopItem::Restore(idRestoreGame *savefile)
{
	savefile->ReadString(id);
	savefile->ReadString(name);
	savefile->ReadString(description);

	savefile->ReadInt(cost);
	savefile->ReadString(image);
	savefile->ReadInt(count);
	savefile->ReadInt(dropped);
	savefile->ReadBool(persistent);
	savefile->ReadBool(canDrop);

	int temp;
	savefile->ReadInt(temp);
	classNames.SetNum(temp);
	for (int i = 0; i < temp; ++i)
	{
		savefile->ReadString(classNames[i]);
	}
	savefile->ReadBool(stackable); // grayman (#2376)
}
