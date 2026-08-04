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

#include "MessageTcp.h"


MessageTcp::MessageTcp() {}

void MessageTcp::Init(std::unique_ptr<idTCP> &&connection) {
	tcp = std::move(connection);
	inputBuffer.clear();
	outputBuffer.clear();
	inputPos = 0;
	outputPos = 0;
}

bool MessageTcp::IsAlive() const {
	return tcp.get() && tcp->IsAlive();
}

bool MessageTcp::ReadMessage(idList<char> &message) {
	message.Clear();
	Think();

	const char *buffer = &inputBuffer[inputPos];
	int remains = inputBuffer.size() - inputPos;
	auto Pull = [&](void *ptr, int size) -> void {
		assert(size <= remains);
		memcpy(ptr, buffer, size);
		buffer += size;
		remains -= size;
	};

	if (remains < 12)
		return false;
	int len = -1;
	char magic[5] = {0};

	//note: little-endianness is assumed
	Pull(magic, 4);
	if (strcmp(magic, "TDM[") != 0)
		goto zomg;
	Pull(&len, 4);
	if (len < 0)
		goto zomg;
	Pull(magic, 4);
	if (strcmp(magic, "]   ") != 0)
		goto zomg;

	if (remains < len + 12)
		return false;

	message.Resize(len + 1);
	message.SetNum(len, false);
	Pull(message.Ptr(), len);
	message.Ptr()[len] = '\0';

	Pull(magic, 4);
	if (strcmp(magic, "   (") != 0)
		goto zomg;
	int len2;
	Pull(&len2, 4);
	if (len2 != len)
		goto zomg;
	Pull(magic, 4);
	if (strcmp(magic, ")TDM") != 0)
		goto zomg;

	inputPos = buffer - inputBuffer.data();
	return true;

zomg:
	common->Printf("ERROR: MessageTCP: wrong message format\n");
	message.Clear();
	Init({});
	return false;
}

void MessageTcp::WriteMessage(const char *message, int len) {
	int where = outputBuffer.size();
	outputBuffer.resize(where + len + 24);
	auto Push = [&](const void *ptr, int size) -> void {
		memcpy(&outputBuffer[where], ptr, size);
		where += size;
	};

	//note: little-endianness is assumed
	Push("TDM[", 4);
	Push(&len, 4);
	Push("]   ", 4);
	Push(message, len);
	Push("   (", 4);
	Push(&len, 4);
	Push(")TDM", 4);

	assert(where == outputBuffer.size());

	Think();
}

void MessageTcp::Think() {
	if (!tcp)
		return;
	static const int BUFFER_SIZE = 1024;

	//if data in buffer is too far from start, then it is moved to the beginning
	auto CompactBuffer = [](CRawVector &vec, int &pos) -> void {
		int remains = vec.size() - pos;
		if (pos > remains + BUFFER_SIZE) {
			memcpy(&vec[0], &vec[pos], remains);
			vec.resize(remains);
			pos = 0;
		}
	};

	CompactBuffer(inputBuffer, inputPos);

	//fetch incoming data from socket
	while (true) {
		inputBuffer.reserve(inputBuffer.size() + BUFFER_SIZE);
		//note: we rely on writing to space after rawvector's end
		int read = tcp->Read(inputBuffer.data() + inputBuffer.size(), BUFFER_SIZE);
		if (read == -1)
			goto onerror;
		if (read == 0)
			break;
		inputBuffer.resize(inputBuffer.size() + read);
	}

	//push outcoming data to socket
	while (outputPos < outputBuffer.size()) {
		int remains = outputBuffer.size() - outputPos;
		int written = tcp->Write(&outputBuffer[outputPos], remains);
		if (written == -1)
			goto onerror;
		if (written == 0)
			break;
		outputPos += written;
	}

	CompactBuffer(outputBuffer, outputPos);

	return;

onerror:
	common->Printf("Automation lost connection\n");
	tcp.reset();
}
