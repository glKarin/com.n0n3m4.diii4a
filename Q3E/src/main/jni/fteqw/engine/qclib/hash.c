#if _MSC_VER >= 1300
	#ifndef _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_WARNINGS
	#endif
	#ifndef _CRT_NONSTDC_NO_WARNINGS
		#define _CRT_NONSTDC_NO_WARNINGS
	#endif
#endif

#include "hash.h"
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#ifndef stricmp 
#define stricmp strcasecmp
#endif
#endif

// hash init assumes we get clean memory
void Hash_InitTable(hashtable_t *table, unsigned int numbucks, void *mem)
{
	table->numbuckets = numbucks;
	table->bucket = (bucket_t **)mem;
}

void *Hash_Enumerate(hashtable_t *table, void (*callback) (void *ctx, void *data), void *ctx)
{
	unsigned int bucknum;
	bucket_t *buck;
	void *data;

	for (bucknum = 0; bucknum < table->numbuckets; bucknum++)
	{
		buck = table->bucket[bucknum];

		while(buck)
		{
			data = buck->data;
			buck = buck->next;

			//now that we don't care about backlinks etc, we can call the callback and it can safely nuke it (Hash_RemoveData or even destroy the bucket if the hash table is going to die).
			callback(ctx, data);
		}
	}
	return NULL;
}

unsigned int Hash_Key(const char *name, unsigned int modulus)
{	//fixme: optimize.
	unsigned int key;
	for (key=0;*name; name++)
		key += ((key<<3) + (key>>28) + *name);
		
	return (key%modulus);
}
unsigned int Hash_KeyInsensitive(const char *name, unsigned int modulus)
{	//fixme: optimize.
	unsigned int key;
	for (key=0;*name; name++)
	{
		if (*name >= 'A' && *name <= 'Z')
			key += ((key<<3) + (key>>28) + (*name-'A'+'a'));
		else
			key += ((key<<3) + (key>>28) + *name);
	}
		
	return (key%modulus);
}

void *Hash_GetIdx(hashtable_t *table, unsigned int idx)
{
	unsigned int bucknum;
	bucket_t *buck;

	for (bucknum = 0; bucknum < table->numbuckets; bucknum++)
	{
		buck = table->bucket[bucknum];

		while(buck)
		{
			if (!idx--)
				return buck->data;

			buck = buck->next;
		}
	}
	return NULL;
}

void *Hash_Get(hashtable_t *table, const char *name)
{
	unsigned int bucknum = Hash_Key(name, table->numbuckets);
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (!STRCMP(name, buck->key.string))
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}
void *Hash_GetInsensitive(hashtable_t *table, const char *name)
{
	unsigned int bucknum = Hash_KeyInsensitive(name, table->numbuckets);
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (!stricmp(name, buck->key.string))
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}
void *Hash_GetInsensitiveBucket(hashtable_t *table, const char *name)
{
	unsigned int bucknum = Hash_KeyInsensitive(name, table->numbuckets);
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (!stricmp(name, buck->key.string))
			return buck;

		buck = buck->next;
	}
	return NULL;
}
void *Hash_GetKey(hashtable_t *table, unsigned int key)
{
	unsigned int bucknum = key%table->numbuckets;
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (buck->key.value == key)
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}
/*Does _NOT_ support items that are added with two names*/
void *Hash_GetNextKey(hashtable_t *table, unsigned int key, void *old)
{
	unsigned int bucknum = key%table->numbuckets;
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (buck->data == old)	//found the old one
			break;
		buck = buck->next;
	}
	if (!buck)
		return NULL;

	buck = buck->next;//don't return old
	while(buck)
	{
		if (buck->key.value == key)
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}
/*Does _NOT_ support items that are added with two names*/
void *Hash_GetNext(hashtable_t *table, const char *name, void *old)
{
	unsigned int bucknum = Hash_Key(name, table->numbuckets);
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (buck->data == old)	//found the old one
//			if (!STRCMP(name, buck->key.string))
				break;
		buck = buck->next;
	}
	if (!buck)
		return NULL;

	buck = buck->next;//don't return old
	while(buck)
	{
		if (!STRCMP(name, buck->key.string))
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}
/*Does _NOT_ support items that are added with two names*/
void *Hash_GetNextInsensitive(hashtable_t *table, const char *name, void *old)
{
	unsigned int bucknum = Hash_KeyInsensitive(name, table->numbuckets);
	bucket_t *buck;

	buck = table->bucket[bucknum];

	while(buck)
	{
		if (buck->data == old)	//found the old one
		{
//			if (!stricmp(name, buck->key.string))
				break;
		}

		buck = buck->next;
	}
	if (!buck)
		return NULL;

	buck = buck->next;//don't return old
	while(buck)
	{
		if (!stricmp(name, buck->key.string))
			return buck->data;

		buck = buck->next;
	}
	return NULL;
}


void *Hash_Add(hashtable_t *table, const char *name, void *data, bucket_t *buck)
{
	unsigned int bucknum = Hash_Key(name, table->numbuckets);

	buck->data = data;
	buck->key.string = name;
	buck->next = table->bucket[bucknum];
	table->bucket[bucknum] = buck;

	return buck;
}
void *Hash_AddInsensitive(hashtable_t *table, const char *name, void *data, bucket_t *buck)
{
	unsigned int bucknum = Hash_KeyInsensitive(name, table->numbuckets);

	buck->data = data;
	buck->key.string = name;
	buck->next = table->bucket[bucknum];
	table->bucket[bucknum] = buck;

	return buck;
}
void *Hash_AddKey(hashtable_t *table, unsigned int key, void *data, bucket_t *buck)
{
	unsigned int bucknum = key%table->numbuckets;

	buck->data = data;
	buck->key.value = key;
	buck->next = table->bucket[bucknum];
	table->bucket[bucknum] = buck;

	return buck;
}

void Hash_Remove(hashtable_t *table, const char *name)
{
	unsigned int bucknum = Hash_Key(name, table->numbuckets);
	bucket_t *buck;	

	buck = table->bucket[bucknum];

	if (!STRCMP(name, buck->key.string))
	{
		table->bucket[bucknum] = buck->next;
		return;
	}


	while(buck->next)
	{
		if (!STRCMP(name, buck->next->key.string))
		{
			buck->next = buck->next->next;
			return;
		}

		buck = buck->next;
	}
	return;
}

void Hash_RemoveDataInsensitive(hashtable_t *table, const char *name, void *data)
{
	unsigned int bucknum = Hash_KeyInsensitive(name, table->numbuckets);
	bucket_t **link, *buck;	

	for (link = &table->bucket[bucknum]; *link; link = &(*link)->next)
	{
		buck = *link;
		if (buck->data == data && !stricmp(name, buck->key.string))
		{
			*link = buck->next;
			return;
		}
	}
}
void Hash_RemoveData(hashtable_t *table, const char *name, void *data)
{
	unsigned int bucknum = Hash_Key(name, table->numbuckets);
	bucket_t **link, *buck;	

	for (link = &table->bucket[bucknum]; *link; link = &(*link)->next)
	{
		buck = *link;
		if (buck->data == data && !stricmp(name, buck->key.string))
		{
			*link = buck->next;
			return;
		}
	}
}
void Hash_RemoveBucket(hashtable_t *table, const char *name, bucket_t *data)
{
	unsigned int bucknum = Hash_Key(name, table->numbuckets);
	bucket_t **link, *buck;	

	for (link = &table->bucket[bucknum]; *link; link = &(*link)->next)
	{
		buck = *link;
		if (buck == data && !stricmp(name, buck->key.string))
		{
			*link = buck->next;
			return;
		}
	}
	return;
}

void Hash_RemoveDataKey(hashtable_t *table, unsigned int key, void *data)
{
	unsigned int bucknum = key%table->numbuckets;
	bucket_t **link, *buck;

	for (link = &table->bucket[bucknum]; *link; link = &(*link)->next)
	{
		buck = *link;
		if (buck->data == data && buck->key.value == key)
		{
			*link = buck->next;
			return;
		}
	}
}
void Hash_RemoveKey(hashtable_t *table, unsigned int key)
{
	unsigned int bucknum = key%table->numbuckets;
	bucket_t *buck;	

	buck = table->bucket[bucknum];

	if (buck->key.value == key)
	{
		table->bucket[bucknum] = buck->next;
		return;
	}


	while(buck->next)
	{
		if (buck->next->key.value == key)
		{
			buck->next = buck->next->next;
			return;
		}

		buck = buck->next;
	}
	return;
}
