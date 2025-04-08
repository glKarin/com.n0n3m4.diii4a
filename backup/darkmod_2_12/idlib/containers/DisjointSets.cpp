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
#include "DisjointSets.h"
#include "FlexList.h"


void idDisjointSets::Init(idList<int> &arr, int num) {
	arr.SetNum(num);
	for (int i = 0; i < num; i++)
		arr[i] = i;
}

int idDisjointSets::GetHead(idList<int> &arr, int v) {
	//pass 1: find head
	int x = v;
	while (x != arr[x])
		x = arr[x];
	int head = x;
	//pass 2: path compression
	int y = v;
	while (y != head) {
		x = arr[y];
		arr[y] = head;
		y = x;
	}
	return head;
}

bool idDisjointSets::Merge(idList<int> &arr, int u, int v) {
	u = GetHead(arr, u);
	v = GetHead(arr, v);
	if (u == v)
		return false;
	arr[u] = v;
	return true;
}

void idDisjointSets::CompressAll(idList<int> &arr) {
	//just let path compression do its job
	for (int i = 0; i < arr.Num(); i++)
		GetHead(arr, i);
}

int idDisjointSets::ConvertToColors(idList<int> &arr) {
	CompressAll(arr);
	idFlexList<int, 128> remap;
	remap.SetNum(arr.Num());
	int k = 0;
	for (int i = 0; i < arr.Num(); i++) if (arr[i] == i)
		remap[i] = k++;
	for (int i = 0; i < arr.Num(); i++)
		arr[i] = remap[arr[i]];
	return k;
}
