/*
** file_vgagraph.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "doomerrors.h"
#include "filesys.h"
#include "wl_def.h"
#include "m_swap.h"
#include "resourcefile.h"
#include "tmemory.h"
#include "w_wad.h"
#include "lumpremap.h"
#include "zstring.h"

#include <algorithm>

struct Huffnode
{
	public:
		WORD	bit0, bit1;	// 0-255 is a character, > is a pointer to a node
};

struct Dimensions
{
	public:
		WORD	width, height;
};

////////////////////////////////////////////////////////////////////////////////

struct FVGALump : public FResourceLump
{
	public:
		DWORD		position;
		DWORD		length;
		Huffnode*	huffman;

		bool		isImage;
		bool		noSkip;
		Dimensions	dimensions;

		int FillCache()
		{
			Owner->Reader->Seek(position+(noSkip ? 0 : 4), SEEK_SET);

			byte* data = new byte[length];
			byte* out = new byte[LumpSize];
			memset(out, 0, LumpSize);
			Owner->Reader->Read(data, length);
			HuffExpand(data, out);
			delete[] data;

			Cache = new char[LumpSize];
			if(!isImage)
				memcpy(Cache, out, LumpSize);
			else
			{
				// We flip again on big endian so that the code that reads the data makes sense
				WriteLittleShort((BYTE*)Cache, dimensions.width);
				WriteLittleShort((BYTE*)Cache+2, dimensions.height);
				memcpy(Cache+4, out, LumpSize-4);
			}
			delete[] out;

			RefCount = 1;
			return 1;
		}

		byte *HuffExpand(byte* source, byte* dest)
		{
			byte *end, *send;
			Huffnode *headptr, *huffptr;
		
			if(!LumpSize || !dest)
			{
				I_FatalError("length or dest is null!");
				return NULL;
			}
		
			headptr = huffman+254;        // head node is always node 254
		
			int written = 0;
		
			end=dest+LumpSize;
			send=source+length;

			byte val = *source++;
			byte mask = 1;
			word nodeval;
			huffptr = headptr;
			while(1)
			{
				if(!(val & mask))
					nodeval = huffptr->bit0;
				else
					nodeval = huffptr->bit1;
				if(mask==0x80)
				{
					val = *source++;
					mask = 1;
					if(source>=send) break;
				}
				else mask <<= 1;
			
				if(nodeval<256)
				{
					*dest++ = (byte) nodeval;
					written++;
					huffptr = headptr;
					if(dest>=end) break;
				}
				else
				{
					huffptr = huffman + (nodeval - 256);
				}
			}

			return dest;
		}
};

////////////////////////////////////////////////////////////////////////////////

class FVGAGraph : public FResourceFile
{
	public:
		FVGAGraph(const char* filename, FileReader *file) : FResourceFile(filename, file), lumps(NULL)
		{
			FString path(filename);
			int lastSlash = path.LastIndexOfAny("/\\:");
			extension = path.Mid(lastSlash+10);
			path = path.Left(lastSlash+1);

			FString vgadictFile = FString("vgadict.") + extension;
			FString vgaheadFile = FString("vgahead.") + extension;
			if(Wads.CheckIfWadLoaded(path.Left(lastSlash)) == -1)
			{
				File directory(path.Len() > 0 ? path : ".");
				FString vgadictFile = path + directory.getInsensitiveFile(FString("vgadict.") + extension, true);
				FString vgaheadFile = path + directory.getInsensitiveFile(FString("vgahead.") + extension, true);

				vgadictReader = new FileReader();
				if(!vgadictReader->Open(vgadictFile))
					vgadictReader.Reset();

				vgaheadReader = new FileReader();
				if(!vgaheadReader->Open(vgaheadFile))
					vgaheadReader.Reset();
			}
			else // Embedded vanilla data?
			{
				FLumpReader *lreader = reinterpret_cast<FLumpReader *>(file);

				for(DWORD i = 0; i < lreader->LumpOwner()->LumpCount(); ++i)
				{
					FResourceLump *lump = lreader->LumpOwner()->GetLump(i);
					if(lump->FullName.CompareNoCase(vgaheadFile) == 0)
						vgaheadReader = lump->NewReader();
					else if(lump->FullName.CompareNoCase(vgadictFile) == 0)
						vgadictReader = lump->NewReader();

					if(vgaheadReader && vgadictReader)
						break;
				}
			}

			if(!vgadictReader)
			{
				FString error;
				error.Format("Could not open vgagraph since %s is missing.", vgadictFile.GetChars());
				throw CRecoverableError(error);
			}
			if(!vgaheadReader)
			{
				FString error;
				error.Format("Could not open vgagraph since %s is missing.", vgaheadFile.GetChars());
				throw CRecoverableError(error);
			}
		}

		~FVGAGraph()
		{
			if(lumps != NULL)
				delete[] lumps;
		}

		bool Open(bool quiet)
		{
			vgadictReader->Read(huffman, sizeof(huffman));
			for(unsigned int i = 0;i < 255;++i)
			{
				huffman[i].bit0 = LittleShort(huffman[i].bit0);
				huffman[i].bit1 = LittleShort(huffman[i].bit1);
			}

			NumLumps = vgaheadReader->GetLength()/3;
			vgaheadReader->Seek(0, SEEK_SET);
			lumps = new FVGALump[NumLumps];
			// The vgahead has 24-bit ints.
			BYTE* data = new BYTE[NumLumps*3];
			vgaheadReader->Read(data, NumLumps*3);

			unsigned int numPictures = 0;
			unsigned int numFonts = 0;
			Dimensions* dimensions = NULL;
			for(unsigned int i = 0;i < NumLumps;i++)
			{
				// Give the lump a temporary name.
				char lumpname[9];
				sprintf(lumpname, "VGA%05d", i);
				lumps[i].Owner = this;
				lumps[i].LumpNameSetup(lumpname);

				lumps[i].noSkip = false;
				lumps[i].isImage = (i > numFonts+2 && i-numFonts-1 < numPictures);
				lumps[i].Namespace = lumps[i].isImage ? ns_graphics : ns_global;
				lumps[i].position = ReadLittle24(&data[i*3]);
				lumps[i].huffman = huffman;

				// The actual length isn't stored so we need to go by the position of the following lump.
				lumps[i].length = 0;
				if(i != 0)
				{
					lumps[i-1].length = lumps[i].position - lumps[i-1].position;
				}

				Reader->Seek(lumps[i].position, SEEK_SET);
				if(!Reader->Read(&lumps[i].LumpSize, 4))
					lumps[i].LumpSize = 0;
				else
					lumps[i].LumpSize = LittleLong(lumps[i].LumpSize);

				if(i == 1) // We must do this starting with the second lump due to how the position is filled.
				{
					// It looks like editors often neglect to give proper sizes
					// for the pictable. If we can at least assume that the
					// compression code will work fine then we can make up a
					// maximum length. We will get the actual length based off
					// where the decoder ends. (Wolf3D hard coded the number of
					// pictures so it just used that for the size.)
					Reader->Seek(lumps[0].position+4, SEEK_SET);
					lumps[0].LumpSize = (NumLumps-1)*4;

					byte* data = new byte[lumps[0].length];
					byte* out = new byte[lumps[0].LumpSize];
					Reader->Read(data, lumps[0].length);
					byte* endPtr = lumps[0].HuffExpand(data, out);
					delete[] data;

					lumps[0].LumpSize = (unsigned int)(endPtr - out);
					numPictures = lumps[0].LumpSize/4;

					dimensions = new Dimensions[numPictures];
					for(unsigned int j = 0;j < numPictures;j++)
					{
						dimensions[j].width = ReadLittleShort(&out[j*4]);
						dimensions[j].height = ReadLittleShort(&out[(j*4)+2]);

						// More correction due to bad vgagraphs
						if(dimensions[j].width > 640 || dimensions[j].height > 480 ||
							dimensions[j].width == 0 || dimensions[j].height == 0)
							numPictures = j;
					}
					delete[] out;
				}
				// Check if the last lump is a font, but only until we hit a
				// lump we determined was not a font.
				else if(i == numFonts+2)
				{
					// First check if it's large enough for the font header
					if(lumps[i-1].LumpSize > 770)
					{
						Reader->Seek(lumps[i-1].position+4, SEEK_SET);

						byte* data = new byte[lumps[i-1].length];
						byte* out = new byte[lumps[i-1].LumpSize];
						Reader->Read(data, lumps[i-1].length);
						byte* endPtr = lumps[i-1].HuffExpand(data, out);
						delete[] data;

						bool endhit = false;
						WORD height = ReadLittleShort(out);
						for(unsigned int c = 0;c < 256;++c)
						{
							WORD offset = ReadLittleShort(&out[c*2+2]);
							BYTE width = out[c+514];

							int space = lumps[i-1].LumpSize - (offset + width*height);
							if(space < 0)
							{
								lumps[i-1].isImage = lumps[i].isImage = true;
								break;
							}
							else if(space == 0)
								endhit = true;
						}
						delete[] out;

						if(!endhit)
							lumps[i-1].isImage = lumps[i].isImage = true;

						if(!lumps[i].isImage)
							++numFonts;
					}
					else
						lumps[i-1].isImage = lumps[i].isImage = true;

					if(lumps[i-1].isImage)
					{
						lumps[i-1].dimensions = dimensions[0];
						lumps[i-1].LumpSize += 4;
					}
				}

				if(lumps[i].isImage)
				{
					lumps[i].dimensions = dimensions[i-numFonts-1];
					lumps[i].LumpSize += 4;
				}
			}

			// HACK: Wolfstone has a chunk of garbage data after the pictures
			//       and before the TILE8.  It's a partially zero-filled version
			//       of the Get Psyched graphic so no idea why it exists or how
			//       it got there.  Unless I see a reason to change this I
			//       believe the best method to attack that problem is to detect
			//       that the size is the same and delete the lump.
			unsigned int tile8Position = 1+numFonts+numPictures;
			if(tile8Position < NumLumps && lumps[tile8Position].LumpSize == lumps[tile8Position-1].LumpSize-4)
			{
				std::copy(&lumps[tile8Position+1], &lumps[NumLumps], &lumps[tile8Position]);
				--NumLumps;
			}

			// HACK: For some reason id decided the tile8 lump will not tell
			//       its size.  So we need to assume it's right after the
			//       graphics. To make matters worse, we can't assume a size
			//       for it since S3DNA has more than 72 tiles.
			//       We will use the method from before to guess a size.
			if(tile8Position < NumLumps && (unsigned)lumps[tile8Position].LumpSize > lumps[tile8Position].length)
			{
				byte* data = new byte[lumps[tile8Position].length];
				byte* out = new byte[64*256];
				Reader->Seek(lumps[tile8Position].position, SEEK_SET);
				Reader->Read(data, lumps[tile8Position].length);
				byte* endPtr = lumps[tile8Position].HuffExpand(data, out);
				delete[] data;
				delete[] out;

				lumps[tile8Position].noSkip = true;
				lumps[tile8Position].LumpSize = (unsigned int)(endPtr - out)&~0x3F;
			}
			if(dimensions != NULL)
				delete[] dimensions;
			delete[] data;

			// We don't care about the PICTABLE lump now so we can just skip
			// over it.
			--NumLumps;
			if(!quiet) Printf(", %d lumps\n", NumLumps);

			LumpRemapper::AddFile(extension, this, LumpRemapper::VGAGRAPH);
			return true;
		}

		FResourceLump *GetLump(int no)
		{
			return &lumps[no+1];
		}

	private:
		Huffnode huffman[255];
		FVGALump* lumps;

		FString extension;
		TUniquePtr<FileReader> vgaheadReader;
		TUniquePtr<FileReader> vgadictReader;
};

FResourceFile *CheckVGAGraph(const char *filename, FileReader *file, bool quiet)
{
	FString fname(filename);
	int lastSlash = fname.LastIndexOfAny("/\\:");
	if(lastSlash != -1)
		fname = fname.Mid(lastSlash+1, 8);
	else
		fname = fname.Left(8);

	if(fname.Len() == 8 && fname.CompareNoCase("vgagraph") == 0) // file must be vgagraph.something
	{
		FResourceFile *rf = new FVGAGraph(filename, file);
		if(rf->Open(quiet)) return rf;
		rf->Reader = NULL; // to avoid destruction of reader
		delete rf;
	}
	return NULL;
}
