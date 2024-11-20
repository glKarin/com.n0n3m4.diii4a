#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "prof.h"
#include "prof_internal.h"
#include <stdint.h>

Prof_Define(_global);

Prof_Zone_Stack Prof_dummy  ;  // impossible parent
Prof_Zone_Stack Prof_dummy2 ; 
Prof_Zone_Stack *Prof_stack = &Prof_dummy2;

int Prof_num_zones;
Prof_Zone *Prof_zones[];

#define MAX_HASH_SIZE     65536   // not unlimited, to catch unbalanced BEGIN/END_PROF
#define INIT_HASH_SIZE    256     // balance resource usage and avoid initial growth

static Prof_Zone_Stack *init_hash[] = { &Prof_dummy };
static Prof_Zone_Stack **zone_hash = init_hash;
static int zone_hash_count = 1;
static int zone_hash_max   = 1;
static int zone_hash_mask  = 0;

static int hash(Prof_Zone *z, Prof_Zone_Stack *s)
{
   int n = (uintptr_t) z + (uintptr_t) s;
   return n + (n >> 8);
}

static void insert_node(Prof_Zone_Stack *q)
{
   int h = hash(q->zone, q->parent);
   int x = h & zone_hash_mask;
   int s = ((h << 4) + (h >> 4)) | 1;

   while (zone_hash[x] != &Prof_dummy)
      x = (x + s) & zone_hash_mask;

   zone_hash[x] = q;

   ++zone_hash_count;
}

static void init_zone(Prof_Zone *zone)
{
   Prof_zones[Prof_num_zones++] = zone;

   zone->initialized = 1;
}

static int count_recursion_depth(Prof_Zone_Stack *stack, Prof_Zone *zone)
{
   int n=0;
   while (stack) {
      if (stack->zone == zone)
         ++n;
      stack = stack->parent;
   }
   return n;
}

static Prof_Zone_Stack *createStackNode(Prof_Zone *zone, Prof_Zone_Stack *parent)
{
   // create a new node
   Prof_Zone_Stack *z = (Prof_Zone_Stack *) malloc(sizeof(*z));
   z->zone = zone;
   z->parent = parent;
   z->total_entry_count = 0;
   z->total_hier_ticks = 0;
   z->total_self_ticks = 0;
   z->t_self_start = 0;
   z->highlevel = NULL;
   z->recursion_depth = count_recursion_depth(parent, zone);
   return z;
}

static void init_zone_hash(int size)
{
   int i;
   assert(size <= MAX_HASH_SIZE);
   zone_hash_max   = size;
   zone_hash_count = 0;
   zone_hash       = (Prof_Zone_Stack **) malloc(sizeof(*zone_hash) * zone_hash_max);
   zone_hash_mask  = size-1;
   for (i=0; i < zone_hash_max; ++i)
      zone_hash[i] = &Prof_dummy;
}

static void Prof_init_lowlevel(void);

// this code is structured to minimize computation
// assuming there's a hit in the very first slot
Prof_extern_C Prof_Zone_Stack *Prof_StackAppend(Prof_Zone *zone)
{
   int h = hash(zone, Prof_stack), s;
   int x = h & zone_hash_mask;
   Prof_Zone_Stack *z = zone_hash[x];
   if (z->parent == Prof_stack && z->zone == zone) return z;
   if (z != &Prof_dummy) {

      // compute a secondary hash function; force it to be odd
      // so it's relatively prime to the power-of-two table size
      s = ((h << 4) + (h >> 4)) | 1;
      for(;;) {
         x = (x + s) & zone_hash_mask;
         z = zone_hash[x];
         if (z->parent == Prof_stack && z->zone == zone) return z;
         if (z == &Prof_dummy) break;
      }
      // loop is guaranteed to terminate because the hash table is never full
   }

   // now's as good a time as any to initialize this zone
   if (!zone->initialized) {
      if (zone_hash_max == 1) {
         Prof_init_lowlevel();
         // the above is reentrant since it initializes _global
         // so now invariants are broken, so start over
         return Prof_StackAppend(zone);
      }
      init_zone(zone);
   }

   // check if we need to grow the table
   // we keep it at most 1/2 full to be very fast
   if (zone_hash_count*2 > zone_hash_max) {
      Prof_Zone_Stack **old_hash = zone_hash, *z;
      int i,n = zone_hash_max;

      init_zone_hash(zone_hash_max*2);

      for (i=0; i < n; ++i)
         if (old_hash[i] != &Prof_dummy)
            insert_node(old_hash[i]);

      z = createStackNode(zone, Prof_stack);
      insert_node(z);
      return z;
   }

   // insert new entry in hash table
   ++zone_hash_count;
   return zone_hash[x] = createStackNode(zone, Prof_stack);
}

void Prof_traverse(void (*func)(Prof_Zone_Stack *z))
{
   int i;
   for (i=0; i < zone_hash_max; ++i)
      if (zone_hash[i] != &Prof_dummy)
         func(zone_hash[i]);
}

static void Prof_init_lowlevel(void)
{
   init_zone_hash(INIT_HASH_SIZE);

   Prof_init_highlevel();

   // intentionally unbalanced, this wraps everything else
   {
      Prof_Region(_global)
   }
}

//////////////////////////////////////////////////////////////////////////
