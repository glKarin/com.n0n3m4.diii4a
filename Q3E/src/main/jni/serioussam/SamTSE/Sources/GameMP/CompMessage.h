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

#ifndef SE_INCL_COMPMESSAGE_H
#define SE_INCL_COMPMESSAGE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

class CCompMessage {
public:
  CTFileName cm_fnmFileName;        // message identificator
  CCompMessageID *cm_pcmiOriginal;  // identifier in player's array

  BOOL cm_bLoaded;            // set if message is loaded
  CTString cm_strSubject;     // message subject
  enum ImageType {
    IT_NONE,
    IT_MODEL,
    IT_PICTURE,
    IT_STATISTICS,
  } cm_itImage;               // accompanying image if any
  CTString cm_strModel;       // name of model if model
  CTFileName cm_fnmPicture;   // filename of picture if picture
  CTString cm_strText;        // original message text

  BOOL cm_bRead;

  INDEX cm_ctFormattedWidth;    // chars per row in formatted text
  INDEX cm_ctFormattedLines;    // number of lines in formatted text
  CTString cm_strFormattedText; // text formatted for given line width

  // load the message from file
  void Load_t(void);  // throw char *
  // format message for given line width
  //void Format(INDEX ctCharsPerLine);
  // [Cecil] Format message based on text width
  void Format(CDrawPort* pdp, PIX pixMaxWidth);

public:
  CCompMessage(void);
  void Clear(void);
  // constructs message from ID
  void SetMessage(CCompMessageID *pcmi);
  // prepare message for using (load, format, etc.)
  //void PrepareMessage(INDEX ctCharsPerLine);
  void PrepareMessage();
  // free memory used by message, but keep message filename
  void UnprepareMessage(void);
  // mark message as read
  void MarkRead(void);
  // get one formatted line
  CTString GetLine(INDEX iLine);
};


#endif  /* include-once check. */

