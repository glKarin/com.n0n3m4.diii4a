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
#pragma once

#include <mutex>
#include <condition_variable>
#include "List.h"

/**
 * Simple implementation of queue for solving producer-consumer problem.
 * Supports multiple producers and multiple consumers.
 * Buffer can be bounded: total weight of all items in buffer cannot exceed specified limit.
 * Calling thread blocks/waits in case of buffer underflow/overflow.
 * Implemented with mutex + two condition variables --- should be good enough for small number of items.
 */
template<class T>
class idProducerConsumerQueue {
	struct Item {
		T data;
		int weight;
	};
	idList<Item> items;
	std::mutex mutex;
	std::condition_variable notFullSignal;
	std::condition_variable notEmptySignal;
	int currWeight = 0;

	bool bounded = false;
	int maxWeight = -1;

public:
	void SetBufferSize(int bound = -1) {
		std::unique_lock<std::mutex> lock(mutex);

		bounded = (bound >= 0);
		maxWeight = bound;
	}

	void ClearFree() {
		std::unique_lock<std::mutex> lock(mutex);

		items.ClearFree();
		currWeight = 0;
	}

	void Append(const T &data, int weight = 1) {
		std::unique_lock<std::mutex> lock(mutex);

		// note: never block additions to empty queue to avoid deadlock
		while (bounded && (currWeight > 0 && currWeight + weight >= maxWeight))
			notFullSignal.wait(lock);

		items.AddGrow( {data, weight} );
		currWeight += weight;

		notEmptySignal.notify_one();
	}

	// pickOldest = false: pop back like in stack, takes O(1) time
	// pickOldest = true: pop front like in queue, takes O(N) time
	T Pop(bool pickOldest = false) {
		std::unique_lock<std::mutex> lock(mutex);

		while (items.Num() == 0)
			notEmptySignal.wait(lock);

		Item res;
		if (pickOldest) {
			res = items[0];
			items.RemoveIndex(0);
		} else {
			res = items.Pop();
		}
		currWeight -= res.weight;

		notFullSignal.notify_one();

		return res.data;
	}
};
