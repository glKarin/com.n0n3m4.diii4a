//=============================
//David's hash tables
//string based.

#ifndef HASH_H__
#define HASH_H__

#define Hash_BytesForBuckets(b) (sizeof(bucket_t*)*(b))

#define STRCMP(s1,s2) (((*s1)!=(*s2)) || strcmp(s1,s2))	//saves about 2-6 out of 120 - expansion of idea from fastqcc
typedef struct bucket_s {
	void *data;
	union {
		const char *string;
		unsigned int value;
	} key;
	struct bucket_s *next;
} bucket_t;
typedef struct hashtable_s {
	unsigned int numbuckets;
	bucket_t **bucket;
} hashtable_t;

void Hash_InitTable(hashtable_t *table, unsigned int numbucks, void *mem);	//mem must be 0 filled. (memset(mem, 0, size))
void *Hash_Enumerate(hashtable_t *table, void (*callback) (void *ctx, void *data), void *ctx);
unsigned int Hash_Key(const char *name, unsigned int modulus);
void *Hash_GetIdx(hashtable_t *table, unsigned int idx);
void *Hash_Get(hashtable_t *table, const char *name);
void *Hash_GetInsensitive(hashtable_t *table, const char *name);
void *Hash_GetInsensitiveBucket(hashtable_t *table, const char *name);
void *Hash_GetKey(hashtable_t *table, unsigned int key);
void *Hash_GetNext(hashtable_t *table, const char *name, void *old);
void *Hash_GetNextInsensitive(hashtable_t *table, const char *name, void *old);
void *Hash_GetNextKey(hashtable_t *table, unsigned int key, void *old);
void *Hash_Add(hashtable_t *table, const char *name, void *data, bucket_t *buck);
void *Hash_AddInsensitive(hashtable_t *table, const char *name, void *data, bucket_t *buck);
void Hash_Remove(hashtable_t *table, const char *name);
void Hash_RemoveData(hashtable_t *table, const char *name, void *data);
void Hash_RemoveDataInsensitive(hashtable_t *table, const char *name, void *data);
void Hash_RemoveBucket(hashtable_t *table, const char *name, bucket_t *data);
void Hash_RemoveKey(hashtable_t *table, unsigned int key);
void Hash_RemoveDataKey(hashtable_t *table, unsigned int key, void *data);
void *Hash_AddKey(hashtable_t *table, unsigned int key, void *data, bucket_t *buck);

#endif
