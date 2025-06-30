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

#ifndef SE_INCL_DEPENDENCY_H
#define SE_INCL_DEPENDENCY_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// adjust file path automatically for application path prefix
void AdjustFilePath_t(CTFileName &fnm);

class CDependInfo {
public:
  // atributes
  CListNode di_Node;
  CTFileName di_fnFileName;
  time_t di_tTime;
  CTFileName di_fnParent;

  // default constructor
  CDependInfo( CTFileName fnFileName, CTFileName fnParent);
  // if this file is updated
  BOOL IsFileOnDiskUpdated(void);
  // if time of given file is same
  inline BOOL IsUpdated(const CDependInfo &diOther) {
    return( diOther.di_tTime == di_tTime);};
  // if given file is older
  inline BOOL IsOlder(const CDependInfo &diOther) {
    return( di_tTime < diOther.di_tTime);};
  // Comparison operator.
  inline BOOL operator==(const CDependInfo &diOther) const {
    return( diOther.di_fnFileName == di_fnFileName);};
  // read and write opertaions
  inline void Read_t( CTStream *istrFile) {
    *istrFile >> di_fnFileName;
    istrFile->Read_t( &di_tTime, sizeof( time_t));
  };
	inline void Write_t( CTStream *ostrFile) const {
    *ostrFile << di_fnFileName;
    ostrFile->Write_t( &di_tTime, sizeof( time_t));
  };
};

class CDependencyList {
public:
  CListHead dl_ListHead;
  // operations
  void ExtractDependencies();
  // remove updated files from list
  void RemoveUpdatedFiles();
  // create list from ascii file
  void ImportASCII( CTFileName fnAsciiFile);
  // export list members into ascii file in form sutable for archivers
  void ExportASCII_t( CTFileName fnAsciiFile);
  // substracts given list from this
  void Substract( CDependencyList &dlToSubstract);
  // extract translation strings from all files in list
  void ExtractTranslations_t( const CTFileName &fnTranslations);
  // clear dependency list
  void Clear( void);
  // if given file allready has its own DependInfo object linked in list
  BOOL ExistsInList(CListHead &lh, CTFileName fnTestName) const;
  // read and write opertaions
	void Read_t( CTStream *istrFile); // throw char *
	void Write_t( CTStream *ostrFile); // throw char *
};


#endif  /* include-once check. */

