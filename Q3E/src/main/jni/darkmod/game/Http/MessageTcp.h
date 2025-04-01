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

#ifndef MESSAGE_TCP_H
#define MESSAGE_TCP_H

#include <memory>
#include "../RawVector.h"

/**
 * Framed protocol on top of idTCP.
 * Note: operates on arrays only, not very efficient (especially in terms of memory).
 */
class MessageTcp {
public:
	MessageTcp();
	void Init(std::unique_ptr<idTCP> &&connection);

	bool ReadMessage(idList<char> &message);
	void WriteMessage(const char *message, int len);

	void Think();
	bool IsAlive() const;

private:
	std::unique_ptr<idTCP> tcp;

	CRawVector inputBuffer;
	int inputPos;
	CRawVector outputBuffer;
	int outputPos;
};

#endif
