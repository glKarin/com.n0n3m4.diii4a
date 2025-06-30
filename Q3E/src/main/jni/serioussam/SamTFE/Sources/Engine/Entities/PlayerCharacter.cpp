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

#include "Engine/StdH.h"

#include <Engine/Entities/PlayerCharacter.h>
#include <Engine/Base/Timer.h>
#include <Engine/Base/Stream.h>
#include <Engine/Network/NetworkMessage.h>

#ifdef PLATFORM_WIN32
typedef HRESULT __stdcall CoCreateGuid_t(UBYTE *pguid);
#else
#include <stdlib.h>
#endif

// get a GUID from system
static void GetGUID(UBYTE aub[16])
{
#ifdef PLATFORM_WIN32
  HINSTANCE hOle32Lib = NULL;
  CoCreateGuid_t *pCoCreateGuid = NULL;

  try {
    // load ole32
    hOle32Lib = ::LoadLibraryA( "ole32.dll");
    if( hOle32Lib == NULL) {
      ThrowF_t(TRANS("Cannot load ole32.dll."));
    }

    // find GUID function
    pCoCreateGuid = (CoCreateGuid_t*)GetProcAddress(hOle32Lib, "CoCreateGuid");
    if (pCoCreateGuid==NULL) {
      ThrowF_t(TRANS("Cannot find CoCreateGuid()."));
    }

    // create the guid
    HRESULT hres = pCoCreateGuid(&aub[0]);

    // check for success
    if (hres!=S_OK) {
      ThrowF_t(TRANS("CoCreateGuid(): Error 0x%08x"), hres);
    }

    // free the ole32 library
    FreeLibrary(hOle32Lib);

  } catch (const char *strError) {
    FatalError(TRANS("Cannot make GUID for a player:\n%s"), strError);
  }

#else

    // !!! FIXME : rcg10112001 Is this sufficient for these purposes?
    for (int i = 0; i < 16; i++)
        aub[i] = (UBYTE) (255.0 * rand() / (RAND_MAX + 1.0));

#endif
}

/*
 * Default constructor -- no character.
 */
CPlayerCharacter::CPlayerCharacter(void)
  : pc_strName("<invalid player>"), pc_strTeam("")
{
  memset(pc_aubGUID, 0, PLAYERGUIDSIZE);
  memset(pc_aubAppearance, 0, MAX_PLAYERAPPEARANCE);
}

/*
 * Create a new character with its name.
 */
CPlayerCharacter::CPlayerCharacter(const CTString &strName)
  : pc_strName(strName), pc_strTeam("")
{
  // if the name passed to constructor is empty string
  if (strName=="") {
    // make this an unnamed player
    pc_strName = "<unnamed player>";
  }
  // create the guid
  GetGUID(pc_aubGUID);
  memset(pc_aubAppearance, 0, MAX_PLAYERAPPEARANCE);
}

void CPlayerCharacter::Load_t( const CTFileName &fnFile) // throw char *
{
  CTFileStream strm;
  strm.Open_t(fnFile);
  Read_t(&strm);
  strm.Close();
}

void CPlayerCharacter::Save_t( const CTFileName &fnFile) // throw char *
{
  CTFileStream strm;
  strm.Create_t(fnFile);
  Write_t(&strm);
  strm.Close();
}

/*
 * Read character from a stream.
 */
void CPlayerCharacter::Read_t(CTStream *pstr) // throw char *
{
  pstr->ExpectID_t("PLC4");
  (*pstr)>>pc_strName>>pc_strTeam;
  pstr->Read_t(pc_aubGUID, sizeof(pc_aubGUID));
  pstr->Read_t(pc_aubAppearance, sizeof(pc_aubAppearance));
}

/*
 * Write character into a stream.
 */
void CPlayerCharacter::Write_t(CTStream *pstr) // throw char *
{
  pstr->WriteID_t("PLC4");
  (*pstr)<<pc_strName<<pc_strTeam;
  pstr->Write_t(pc_aubGUID, sizeof(pc_aubGUID));
  pstr->Write_t(pc_aubAppearance, sizeof(pc_aubAppearance));
}

/* Get character name. */
const CTString &CPlayerCharacter::GetName(void) const 
{
  return pc_strName; 
};
const CTString CPlayerCharacter::GetNameForPrinting(void) const
{
  CTString strName(pc_strName);
  // get rid of newlines in the name
  strName.ReplaceSubstr("\n", "");
  strName.ReplaceSubstr("\r", "");
  return "^o"+pc_strName+"^r";
}
/* Set character name. */
void CPlayerCharacter::SetName(CTString strName) 
{ 
  // limit string length to 20 characters not including decorated text control codes
  // strName.TrimRightNaked(20); !!!! needs checking
  pc_strName = strName; 
};

/* Get character team. */
const CTString &CPlayerCharacter::GetTeam(void) const
{
  return pc_strTeam; 
}

const CTString CPlayerCharacter::GetTeamForPrinting(void) const
{
  return "^o"+pc_strTeam+"^r"; 
}

/* Set character team. */
void CPlayerCharacter::SetTeam(CTString strTeam)
{
  // limit string length to 20 characters not including decorated text control codes
  // strTeam.TrimRightNaked(20); !!!! needs checking
  pc_strTeam = strTeam; 
}

/* Assignment operator. */
CPlayerCharacter &CPlayerCharacter::operator=(const CPlayerCharacter &pcOther)
{
  ASSERT(this!=NULL && &pcOther!=NULL);
  pc_strName = pcOther.pc_strName;
  pc_strTeam = pcOther.pc_strTeam;
  memcpy(pc_aubGUID, pcOther.pc_aubGUID, PLAYERGUIDSIZE);
  memcpy(pc_aubAppearance, pcOther.pc_aubAppearance, MAX_PLAYERAPPEARANCE);
  return *this;
};

/* Comparison operator. */
BOOL CPlayerCharacter::operator==(const CPlayerCharacter &pcOther) const
{
  for(INDEX i=0;i<PLAYERGUIDSIZE; i++) {
    if (pc_aubGUID[i] != pcOther.pc_aubGUID[i]) {
      return FALSE;
    }
  }
  return TRUE;
};

// stream operations
CTStream &operator<<(CTStream &strm, CPlayerCharacter &pc)
{
  pc.Write_t(&strm);
  return strm;
};
CTStream &operator>>(CTStream &strm, CPlayerCharacter &pc)
{
  pc.Read_t(&strm);
  return strm;
};
// message operations
CNetworkMessage &operator<<(CNetworkMessage &nm, CPlayerCharacter &pc)
{
  nm<<pc.pc_strName<<pc.pc_strTeam;
  nm.Write(pc.pc_aubGUID, PLAYERGUIDSIZE);
  nm.Write(pc.pc_aubAppearance, MAX_PLAYERAPPEARANCE);
  return nm;
};
CNetworkMessage &operator>>(CNetworkMessage &nm, CPlayerCharacter &pc)
{
  nm>>pc.pc_strName>>pc.pc_strTeam;
  nm.Read(pc.pc_aubGUID, PLAYERGUIDSIZE);
  nm.Read(pc.pc_aubAppearance, MAX_PLAYERAPPEARANCE);
  return nm;
};
