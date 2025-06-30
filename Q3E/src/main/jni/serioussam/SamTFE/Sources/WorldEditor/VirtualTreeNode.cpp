/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "stdafx.h"
#include "WorldEditor.h"
#include "VirtualTreeNode.h"

CVirtualTreeNode::CVirtualTreeNode()
{
  vtn_pTextureData = NULL;
  vtn_bmBrowsingMode = BM_ICONS_MEDIUM;
  vtn_bSelected = FALSE;
}

CVirtualTreeNode::~CVirtualTreeNode()
{
  FORDELETELIST( CVirtualTreeNode, vtn_lnInDirectory, vtn_lhChildren, litDel)
  {
    delete &litDel.Current();
  }
}

INDEX _iTabs=0;
void CVirtualTreeNode::Dump(CTStream *pFile)
{
  for( INDEX iTab=0; iTab<_iTabs; iTab++)
  {
    pFile->PutString_t("  ");
  }
  if( vtn_bIsDirectory)
  {
    CTString strDirectory;
    strDirectory.PrintF("Directory: %s", vtn_strName);
    pFile->PutLine_t(strDirectory);
    _iTabs++;
    
    FOREACHINLIST(CVirtualTreeNode, vtn_lnInDirectory, vtn_lhChildren, it)
    {
      it->Dump(pFile);
    }
    _iTabs--;
  }
  else
  {
    CTString strItem;
    strItem.PrintF("Item: %s", vtn_fnItem);
    pFile->PutLine_t(strItem);
  }
}

void CVirtualTreeNode::Read_t( CTStream *pFile, CVirtualTreeNode* pParent)
{
  if( pParent != NULL)
  {
    pParent->vtn_lhChildren.AddTail( vtn_lnInDirectory);
  }
  vnt_pvtnParent = pParent;
  pFile->Read_t( &vtn_bIsDirectory, sizeof(BOOL));
  
  if( vtn_bIsDirectory)
  {
    pFile->Read_t( &vtn_itIconType, sizeof(INDEX));     // Symbolic icons for directories
    pFile->Read_t( &vtn_bmBrowsingMode, sizeof(INDEX)); // Icons (and size) or descriptions
    *pFile >> vtn_strName;
    INDEX iDirEntries;
    pFile->Read_t( &iDirEntries, sizeof(INDEX));
    for( INDEX i=0; i<iDirEntries; i++)
    {
      CVirtualTreeNode *pVTN = new CVirtualTreeNode;
      pVTN->Read_t( pFile, this);
    }
  }
  else
  {
    pFile->Read_t( &vtn_bSelected, sizeof(BOOL));       // Item's selection bit
    *pFile >> vtn_strName;
    *pFile >> vtn_fnItem;
  }
}

void CVirtualTreeNode::Write_t( CTStream *pFile)
{
  *pFile << (INDEX) vtn_bIsDirectory;
  if( vtn_bIsDirectory)
  {
    pFile->Write_t( &vtn_itIconType, sizeof(INDEX));
    pFile->Write_t( &vtn_bmBrowsingMode, sizeof(INDEX));
    *pFile << vtn_strName;
    INDEX iDirEntries = vtn_lhChildren.Count();
    pFile->Write_t( &iDirEntries, sizeof(INDEX));
    FOREACHINLIST(CVirtualTreeNode, vtn_lnInDirectory, vtn_lhChildren, it)
    {
      it->Write_t( pFile);
    }
  }
  else
  {
    pFile->Write_t( &vtn_bSelected, sizeof(BOOL));
    *pFile << vtn_strName;
    *pFile << vtn_fnItem;
  }
}

void CVirtualTreeNode::MakeRoot(void)
{
  FORDELETELIST( CVirtualTreeNode, vtn_lnInDirectory, vtn_lhChildren, litDel)
  {
    delete &litDel.Current();
  }
  
  vtn_bIsDirectory=TRUE;
  vnt_pvtnParent=NULL;
  vtn_Handle=NULL;
  vtn_itIconType=0;
  vtn_bmBrowsingMode=BM_ICONS_MEDIUM;
  vtn_bSelected=FALSE;
  vtn_pTextureData=NULL;
  vtn_strName="VRT not loaded";
  vtn_fnItem=CTString("");
}

void CVirtualTreeNode::MoveToDirectory(CVirtualTreeNode *pVTNDst)
{
  ASSERT( vnt_pvtnParent!=NULL);
  if( vnt_pvtnParent==NULL) return;

  vnt_pvtnParent=pVTNDst;
  vtn_lnInDirectory.Remove();
  pVTNDst->vtn_lhChildren.AddTail( vtn_lnInDirectory);
}
