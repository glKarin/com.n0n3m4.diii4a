#include "PrecompCommon.h"
#include "ChunkedFile.h"

const obuint32 CF_MASTER_HEADER = Utils::MakeId32('C', 'F', 'H', 'R');

ChunkedFile::ChunkedFile()
{
}

ChunkedFile::~ChunkedFile()
{
}

bool ChunkedFile::OpenForWrite(const char *_name, bool _append)
{
	if(m_File.OpenForWrite(_name, File::Binary, _append))
	{
		if(!_append && !m_File.WriteInt32(CF_MASTER_HEADER))
			return false;
		return true;
	}
	return false;
}

bool ChunkedFile::OpenForRead(const char *_name)
{
	if(m_File.OpenForRead(_name, File::Binary))
	{
		// check the header
		obuint32 hdr = 0;
		if(!m_File.ReadInt32(hdr) || hdr != CF_MASTER_HEADER)
			return false;
		return true;
	}
	return false;
}

void ChunkedFile::Close()
{
	m_File.Close();
}

bool ChunkedFile::FirstChunk(obuint32 &_chunk, obuint32 &_size)
{
	if(m_File.IsOpen() && m_File.Seek(0))
		return NextChunk(_chunk, _size);
	return false;	
}

bool ChunkedFile::NextChunk(obuint32 &_chunk, obuint32 &_size)
{
	if(m_File.IsOpen() && m_File.ReadInt32(_chunk) && m_File.ReadInt32(_size))
		return true;
	return false;
}

//bool ChunkedFile::WriteChunkHdr(obuint32 _chunk)
//{
//	if(m_File.IsOpen() && m_File.WriteInt32(_chunk))
//		return true;
//	return false;
//}
//
//bool ChunkedFile::WriteChunkData(obint32 _num)
//{
//	if(m_File.IsOpen() && m_File.WriteInt32(_size) && m_File.Write(_data, _size))
//		return true;
//	return false;
//}
//
//bool ChunkedFile::WriteChunkData(obReal _num)
//{
//	if(m_File.IsOpen() && m_File.WriteInt32(_size) && m_File.Write(_data, _size))
//		return true;
//	return false;
//}

bool ChunkedFile::WriteChunkData(const void *_data, obuint32 _size)
{
	if(m_File.IsOpen() && m_File.WriteInt32(_size) && m_File.Write(_data, _size))
		return true;
	return false;
}
