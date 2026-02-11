// Moved out of file_wad.cpp for ECWolf since the Mac Wolf LZSS algorithm is
// mostly the same. Somewhat unfortunately the Mac Wolf LZSS comes in two
// flavors so we're best off making a template for it.

// Console Doom LZSS wrapper.
#ifdef MACWOLF
template<bool BigEndian>
#endif
class FileReaderLZSS : public FileReaderBase
{
private:
	enum { BUFF_SIZE = 4096, WINDOW_SIZE = 4096,
#ifndef MACWOLF
		INTERNAL_BUFFER_SIZE = 128
#else
		INTERNAL_BUFFER_SIZE = 144
#endif
	};

	FileReader &File;
	bool SawEOF;
	BYTE InBuff[BUFF_SIZE];

	enum StreamState
	{
		STREAM_EMPTY,
		STREAM_BITS,
		STREAM_FLUSH,
		STREAM_FINAL
	};
	struct
	{
		StreamState State;

		BYTE *In;
		unsigned int AvailIn;
		unsigned int InternalOut;

		const BYTE *WindowData;
		BYTE *InternalBuffer;
		BYTE CFlags, Bits;

		// Double sized window so that we can avoid shuffling bytes around until
		// we can purge a whole window of data.
		BYTE Window[WINDOW_SIZE*2+INTERNAL_BUFFER_SIZE];
	} Stream;

	void FillBuffer()
	{
		if(Stream.AvailIn)
			memmove(InBuff, Stream.In, Stream.AvailIn);

		long numread = File.Read(InBuff+Stream.AvailIn, BUFF_SIZE-Stream.AvailIn);

		if (numread < BUFF_SIZE)
		{
			SawEOF = true;
		}
		Stream.In = InBuff;
		Stream.AvailIn = numread+Stream.AvailIn;
	}

	// Reads a flag byte.
	void PrepareBlocks()
	{
		assert(Stream.InternalBuffer == Stream.WindowData);
		Stream.CFlags = *Stream.In++;
		--Stream.AvailIn;

		// If entirely uncompressed shortcut loop if possible
#ifndef MACWOLF
		if(Stream.CFlags == 0)
#else
		if(Stream.CFlags == 0xFF)
#endif
		{
			if(Stream.AvailIn >= 8)
			{
				memcpy(Stream.InternalBuffer, Stream.In, 8);

				Stream.AvailIn -= 8;
				Stream.In += 8;
				Stream.InternalBuffer += 8;
				Stream.InternalOut += 8;
				Stream.State = STREAM_FLUSH;
				return;
			}
		}

		Stream.Bits = 0xFF;
		Stream.State = STREAM_BITS;
	}

	// Reads the next chunk in the block. Returns true if successful and
	// returns false if it ran out of input data.
	bool UncompressBlock()
	{
#ifndef MACWOLF
		if(Stream.CFlags & 1)
#else
		if(!(Stream.CFlags & 1))
#endif
		{
			// Check to see if we have enough input
			if(Stream.AvailIn < 2)
				return false;
			Stream.AvailIn -= 2;

#ifndef MACWOLF
			WORD pos = BigShort(*(WORD*)Stream.In);
			BYTE len = (pos & 0xF)+1;
			pos >>= 4;
#else
			WORD pos = BigEndian ? BigShort(*(WORD*)Stream.In) : LittleShort(*(WORD*)Stream.In);
			BYTE len = ((pos>>12) & 0xF)+3;
			pos = 0xFFF-(pos&0xFFF);
#endif
			Stream.In += 2;
#ifndef MACWOLF
			if(len == 1)
			{
				// We've reached the end of the stream.
				Stream.State = STREAM_FINAL;
				return true;
			}
#endif

			const BYTE* copyStart = Stream.InternalBuffer-pos-1;

			// Complete overlap: Single byte repeated
			if(pos == 0)
				memset(Stream.InternalBuffer, *copyStart, len);
			// No overlap: One copy
			else if(pos >= len)
				memcpy(Stream.InternalBuffer, copyStart, len);
			else
			{
				// Partial overlap: Copy in 2 or 3 chunks.
				do
				{
					unsigned int copy = MIN<unsigned int>(len, pos+1);
					memcpy(Stream.InternalBuffer, copyStart, copy);
					Stream.InternalBuffer += copy;
					Stream.InternalOut += copy;
					len -= copy;
					pos += copy; // Increase our position since we can copy twice as much the next round.
				}
				while(len);
			}

			Stream.InternalOut += len;
			Stream.InternalBuffer += len;
		}
		else
		{
			// Uncompressed byte.
			*Stream.InternalBuffer++ = *Stream.In++;
			--Stream.AvailIn;
			++Stream.InternalOut;
		}

		Stream.CFlags >>= 1;
		Stream.Bits >>= 1;

		// If we're done with this block, flush the output
		if(Stream.Bits == 0)
			Stream.State = STREAM_FLUSH;

		return true;
	}

public:
	FileReaderLZSS(FileReader &file) : File(file), SawEOF(false)
	{
		Stream.State = STREAM_EMPTY;
		Stream.WindowData = Stream.InternalBuffer = Stream.Window+WINDOW_SIZE;
		Stream.InternalOut = 0;
		Stream.AvailIn = 0;

		FillBuffer();
	}

	~FileReaderLZSS()
	{
	}

	long Read(void *buffer, long len)
	{

		BYTE *Out = (BYTE*)buffer;
		long AvailOut = len;

		do
		{
			while(Stream.AvailIn)
			{
				if(Stream.State == STREAM_EMPTY)
					PrepareBlocks();
				else if(Stream.State == STREAM_BITS && !UncompressBlock())
					break;
				else
					break;
			}

			unsigned int copy = MIN<unsigned int>(Stream.InternalOut, AvailOut);
			if(copy > 0)
			{
				memcpy(Out, Stream.WindowData, copy);
				Out += copy;
				AvailOut -= copy;

				Stream.InternalOut -= copy;
				Stream.WindowData += copy;

				// If our sliding window has grown large enough, slide it back.
				if(Stream.WindowData >= Stream.Window+WINDOW_SIZE*2)
				{
					memmove(Stream.Window, Stream.Window+WINDOW_SIZE, WINDOW_SIZE+INTERNAL_BUFFER_SIZE);
					Stream.WindowData -= WINDOW_SIZE;
					Stream.InternalBuffer -= WINDOW_SIZE;
				}
			}

			if(Stream.State == STREAM_FINAL)
				break;

			if(Stream.InternalOut == 0 && Stream.State == STREAM_FLUSH)
				Stream.State = STREAM_EMPTY;

			if(Stream.AvailIn < 2)
				FillBuffer();
		}
		while(AvailOut && Stream.State != STREAM_FINAL);

		assert(AvailOut == 0);
		return static_cast<long>(Out - (BYTE*)buffer);
	}
};
