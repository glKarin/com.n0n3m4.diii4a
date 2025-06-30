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

614
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/CutSequences/Book/Book.h"
#include "ModelsMP/CutSequences/Book/CoverPages.h"
%}

class CPhotoAlbum: CMovableModelEntity {
name      "PhotoAlbum";
thumbnail "Thumbnails\\PhotoAlbum.tbn";
features  "IsTargetable", "HasName", "IsImportant";

properties:
  1 FLOAT m_fStretch "Stretch" 'S' = 1.0f,
  2 CTString m_strName  "Name" 'N' = "Photo album",
  3 INDEX m_iCurrentPage = -1,
  4 FLOAT m_tmPageWait "Page wait" 'W' = 0.5f,

 10 CSoundObject m_soPage,

components:

// ************** DATA **************
  10 model   MODEL_ALBUM           "ModelsMP\\CutSequences\\Book\\Book.mdl",
  11 texture TEXTURE_ALBUM         "ModelsMP\\CutSequences\\Book\\Book.tex",
  12 model   MODEL_PAGE            "ModelsMP\\CutSequences\\Book\\CoverPages.mdl",
  13 texture TEXTURE_PAGE01        "ModelsMP\\CutSequences\\Book\\Page01.tex",
  14 texture TEXTURE_PAGE02        "ModelsMP\\CutSequences\\Book\\Page02.tex",
  15 texture TEXTURE_PAGE03        "ModelsMP\\CutSequences\\Book\\Page03.tex",
  16 texture TEXTURE_PAGE04        "ModelsMP\\CutSequences\\Book\\Page04.tex",
  17 texture TEXTURE_PAGE_JOKE     "ModelsMP\\CutSequences\\Book\\PageJoke.tex",
  18 texture TEXTURE_PAGE05        "ModelsMP\\CutSequences\\Book\\Page05.tex",
  19 texture TEXTURE_PAGE06        "ModelsMP\\CutSequences\\Book\\Page06.tex",
  20 texture TEXTURE_PAGE07        "ModelsMP\\CutSequences\\Book\\Page07.tex",
  21 texture TEXTURE_BLANK_PAGE    "ModelsMP\\CutSequences\\Book\\Blank_Page.tex",

  50 sound   SOUND_PAGE            "ModelsMP\\CutSequences\\Book\\Sounds\\PageFlip.wav",
  
functions:
  void Precache(void) {
    PrecacheTexture(TEXTURE_PAGE01);
    PrecacheTexture(TEXTURE_PAGE02);
    PrecacheTexture(TEXTURE_PAGE03);
    PrecacheTexture(TEXTURE_PAGE04);
    PrecacheTexture(TEXTURE_PAGE_JOKE);
    PrecacheTexture(TEXTURE_PAGE05);
    PrecacheTexture(TEXTURE_PAGE06);
    PrecacheTexture(TEXTURE_PAGE07);
    PrecacheTexture(TEXTURE_BLANK_PAGE);
    PrecacheSound(SOUND_PAGE);
  }

procedures: 
  OpenBook(EVoid)
  {
    // main book
    GetModelObject()->PlayAnim(BOOK_ANIM_OPENING, 0);
    // left cover
    CModelObject &mo1=GetModelObject()->GetAttachmentModel(BOOK_ATTACHMENT_PAGE01)->amo_moModelObject;
    mo1.PlayAnim(COVERPAGES_ANIM_OPENING, 0);
    // right cover
    CModelObject &mo2=GetModelObject()->GetAttachmentModel(BOOK_ATTACHMENT_PAGE02)->amo_moModelObject;
    mo2.PlayAnim(COVERPAGES_ANIM_RIGHTOPENING, 0);
    m_soPage.Set3DParameters(1000.0f, 500.0f, 2.0f, 1.0f);
    PlaySound(m_soPage, SOUND_PAGE, SOF_3D);
    // wait book to open
    autowait( GetModelObject()->GetAnimLength(BOOK_ANIM_OPENING));
    autowait( 0.1f);
    m_iCurrentPage=TEXTURE_PAGE01;
    while(m_iCurrentPage<TEXTURE_PAGE07)
    {
      autowait( m_tmPageWait);
      AddAttachment(BOOK_ATTACHMENT_PAGE03, MODEL_PAGE, m_iCurrentPage);
      GetModelObject()->StretchModel( FLOAT3D(m_fStretch,m_fStretch,m_fStretch));
      CModelObject &mo3=GetModelObject()->GetAttachmentModel(BOOK_ATTACHMENT_PAGE03)->amo_moModelObject;
      mo3.PlayAnim(COVERPAGES_ANIM_OPENING, 0);
      PlaySound(m_soPage, SOUND_PAGE, SOF_3D);
      // switch page 2 to blank page texture
      CModelObject &mo2=GetModelObject()->GetAttachmentModel(BOOK_ATTACHMENT_PAGE02)->amo_moModelObject;
      mo2.SetTextureData(GetTextureDataForComponent(m_iCurrentPage+1));
      autowait(1.0f);
      CModelObject &mo3=GetModelObject()->GetAttachmentModel(BOOK_ATTACHMENT_PAGE03)->amo_moModelObject;
      mo3.SetTextureData(GetTextureDataForComponent(TEXTURE_BLANK_PAGE));
      autowait( mo3.GetAnimLength(COVERPAGES_ANIM_OPENING)-1.0f);
      RemoveAttachment(BOOK_ATTACHMENT_PAGE03);
      m_iCurrentPage+=1;
    }

    return EReturn();
  }

 /************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();

    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set your appearance
    SetModel(MODEL_ALBUM);
    SetModelMainTexture(TEXTURE_ALBUM);
    GetModelObject()->StartAnim(BOOK_ANIM_CLOSED);
    AddAttachment(BOOK_ATTACHMENT_PAGE01, MODEL_PAGE, TEXTURE_BLANK_PAGE);
    AddAttachment(BOOK_ATTACHMENT_PAGE02, MODEL_PAGE, TEXTURE_PAGE01);
    CAttachmentModelObject *pamo;
    pamo = GetModelObject()->GetAttachmentModel(BOOK_ATTACHMENT_PAGE01);
    CModelObject *pmo;
    pmo=&pamo->amo_moModelObject;
    pmo->PlayAnim(COVERPAGES_ANIM_LEFTCLOSED, 0);
    pamo = GetModelObject()->GetAttachmentModel(BOOK_ATTACHMENT_PAGE02);
    pmo=&pamo->amo_moModelObject;
    pmo->PlayAnim(COVERPAGES_ANIM_RIGHTCLOSED, 0);

    GetModelObject()->StretchModel( FLOAT3D(m_fStretch,m_fStretch,m_fStretch));
    ModelChangeNotify();

    wait()
    {
      // on the beginning
      on(EBegin): {
        resume;
      }
      on(EStart): {
        call OpenBook();
        resume;
      }
      on(EReturn): {
        resume;
      }
    };
  };
};

