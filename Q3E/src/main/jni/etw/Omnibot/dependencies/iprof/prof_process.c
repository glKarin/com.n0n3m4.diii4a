#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "prof.h"
#include "prof_internal.h"

// whether zone-self-data is kept to allow the history graph
#define Prof_ZONE_HISTORY

// whether full detailed (and large) 
#define Prof_CALL_HISTORY

// number of frames of history to keep
#define NUM_FRAME_SLOTS                    128


// number of unique zones allowed in the entire application
// @TODO: remove MAX_PROFILING_ZONES and make it dynamic
#define MAX_PROFILING_ZONES                512

////////////////////////////////////////////////////////////////////////

// the number of moving averages
#define NUM_PROFILE_TRACKER_HISTORY_SLOTS  3

// the number of frames to ignore before starting the moving averages
#define NUM_THROWAWAY_UPDATES              3

// threshhold for a moving average of an integer to be at zero
#define INT_ZERO_THRESHHOLD                0.25

Prof_Zone *Prof_zones[MAX_PROFILING_ZONES];

#ifdef Prof_ZONE_HISTORY
static float zone_history[MAX_PROFILING_ZONES][NUM_FRAME_SLOTS]; // 256K
#endif

// these structures are used solely to track data over time
typedef struct
{
   double values[NUM_PROFILE_TRACKER_HISTORY_SLOTS];
   double variances[NUM_PROFILE_TRACKER_HISTORY_SLOTS];
#ifdef Prof_CALL_HISTORY
   float  history[NUM_FRAME_SLOTS];
#endif
} History_Scalar;

typedef struct
{
   History_Scalar self_time;
   History_Scalar hierarchical_time;
   History_Scalar entry_count;
   int max_recursion;
} Profile_Tracker_Data_Record;

static History_Scalar frame_time;

static double  times_to_reach_90_percent[NUM_PROFILE_TRACKER_HISTORY_SLOTS];
static double  precomputed_factors      [NUM_PROFILE_TRACKER_HISTORY_SLOTS];

static int        num_active_zones;
static int        update_index;     // 2^31 at 100fps = 280 days
static double     last_update_time;
static Prof_Report_Mode displayed_quantity;

#define FRAME_TIME_INITIAL          0.001

static int history_index;
static int display_frame;
static int slot = NUM_PROFILE_TRACKER_HISTORY_SLOTS-1;

static void clear(History_Scalar *s)
{
   int i;
   for (i = 0; i < NUM_PROFILE_TRACKER_HISTORY_SLOTS; i++) {
      s->values[i] = 0;
      s->variances[i] = 0;
   }
}

static void update(History_Scalar *s, double new_value, double *k_array)
{
   int i;
    
   double new_variance = new_value * new_value;

   for (i = 0; i < NUM_PROFILE_TRACKER_HISTORY_SLOTS; i++) {
      double k = k_array[i];
      s->values[i] = s->values[i] * k + new_value * (1 - k);
      s->variances[i] = s->variances[i] * k + new_variance * (1 - k);
   }
#ifdef Prof_CALL_HISTORY
   s->history[history_index] = (float) new_value;
#endif
}

static void eternity_set(History_Scalar *s, double new_value)
{
   double new_variance = new_value * new_value;

   int i;
   for (i = 0; i < NUM_PROFILE_TRACKER_HISTORY_SLOTS; i++) {
      s->values[i] = new_value;
      s->variances[i] = new_variance;
   }
#ifdef Prof_CALL_HISTORY
   s->history[history_index] = (float) new_value;
#endif
}

static double get_value(History_Scalar *s)
{
#ifdef Prof_CALL_HISTORY
   if (display_frame) {
      return s->history[(history_index - display_frame + NUM_FRAME_SLOTS) % NUM_FRAME_SLOTS];
   }
#endif
   return s->values[slot];
}

void Prof_init_highlevel()
{
   int j;

   update_index = 0;
   last_update_time = 0;

   times_to_reach_90_percent[0] = 0.1f;
   times_to_reach_90_percent[1] = 0.8f;
   times_to_reach_90_percent[2] = 2.5f;

   displayed_quantity = Prof_CALL_GRAPH;

   clear(&frame_time);

   for (j = 0; j < NUM_PROFILE_TRACKER_HISTORY_SLOTS; j++) {
      frame_time.values[j] = FRAME_TIME_INITIAL;
   }
}


#ifdef Prof_ENABLED
static Prof_Zone *expand = &Prof_region__global;
#else
static Prof_Zone *expand = NULL;
#endif

Prof_extern_C Prof_Report_Mode Prof_get_report_mode()
{
	return displayed_quantity;
}

Prof_extern_C void Prof_set_report_mode(Prof_Report_Mode desired)
{
   displayed_quantity = desired;
}

// visit all Prof_Zone_Stacks
extern void Prof_traverse(void (*func)(Prof_Zone_Stack *c));

static void propogate_stack(Prof_Zone_Stack *c)
{
   Prof_Zone_Stack *p = c;

   // propogate times up the stack for hierarchical
   // times, but watch out for recursion

   while (p->zone) {
      if (!p->zone->visited) {
         p->total_hier_ticks += c->total_self_ticks;
         p->zone->visited = 1;
      }
      p = p->parent;
   }
   p = c;
   while (p->zone) {
      p->zone->visited = 0;
      p = p->parent;
   }
}

static void clear_stack(Prof_Zone_Stack *c)
{
   c->total_hier_ticks = 0;
   c->total_self_ticks = 0;
   c->total_entry_count = 0;
}

static double sum;
static void sum_times(Prof_Zone_Stack *c)
{
   sum += c->total_self_ticks;
}

static double timestamps_to_seconds;
static void update_history(Prof_Zone_Stack *c)
{
   double self_time, hier_time, entry_count;

   Profile_Tracker_Data_Record *record = (Profile_Tracker_Data_Record *) c->highlevel;
   Prof_Zone *z = c->zone;

   if (record == NULL) {
      record = (Profile_Tracker_Data_Record *) malloc(sizeof(*record));
      c->highlevel = (void *) record;
      clear(&record->entry_count);
      clear(&record->self_time);
      clear(&record->hierarchical_time);
      record->max_recursion = 0;
   }

   if (c->recursion_depth > record->max_recursion)
      record->max_recursion = c->recursion_depth;

   self_time = c->total_self_ticks * timestamps_to_seconds;
   hier_time = c->total_hier_ticks * timestamps_to_seconds;
   entry_count = c->total_entry_count;

   if (update_index < NUM_THROWAWAY_UPDATES) {
      eternity_set(&record->entry_count, entry_count);
      eternity_set(&record->self_time, self_time);
      eternity_set(&record->hierarchical_time, hier_time);
   } else {
      update(&record->self_time, self_time, precomputed_factors);
      update(&record->hierarchical_time, hier_time, precomputed_factors);
      update(&record->entry_count, entry_count, precomputed_factors);
   }

#ifdef Prof_ZONE_HISTORY
   * ((float *) z->highlevel) += (float) self_time;
#endif
}

const double SPEEDSTEP_DETECTION_RATIO = 0.08;
static int speedstep_warning;

Prof_extern_C void Prof_update(int record_data)
{
#ifdef Prof_ENABLED
   Prof_Begin(iprof_update)

   static History_Scalar integer_timestamps_per_second;
   static Prof_Int64 last_integer_timestamp;
   static Prof_Int64 current_integer_timestamp;

   int i;
   double now, dt;
   Prof_Int64 timestamp_delta;
   double timestamps_per_second;

   assert(Prof_num_zones <= MAX_PROFILING_ZONES);

   Prof_traverse(propogate_stack);

   // Precompute the time factors

   now = Prof_get_time();

   if (update_index == 0) {
      dt = FRAME_TIME_INITIAL;
   } else {
      dt = now - last_update_time;
      if (dt == 0) dt = FRAME_TIME_INITIAL;
   }

   last_update_time = now;

   for (i = 0; i < NUM_PROFILE_TRACKER_HISTORY_SLOTS; i++) {
      precomputed_factors[i] = pow(0.1f, dt / times_to_reach_90_percent[i]);
   }

   precomputed_factors[0] = 0; // instantaneous.

   Prof_get_timestamp(&current_integer_timestamp);
   if (update_index == 0) {
      sum = 0;
      Prof_traverse(sum_times);
      if (sum == 0) sum = 1;
      timestamp_delta = (Prof_Int64) sum;
   } else {
      timestamp_delta = current_integer_timestamp - last_integer_timestamp;
      if (timestamp_delta == 0) timestamp_delta = 1;
   }

   last_integer_timestamp = current_integer_timestamp;
   timestamps_per_second = (double) timestamp_delta / dt;

   if (update_index < NUM_THROWAWAY_UPDATES) {
      eternity_set(&integer_timestamps_per_second, timestamps_per_second);
   } else {
      update(&integer_timestamps_per_second, timestamps_per_second, precomputed_factors);
   }

   {
      const int ss_slot = 1;
      double ss_val, ss_variance, ss_stdev, ss_ratio;

      ss_val = integer_timestamps_per_second.values[ss_slot];
      ss_variance = integer_timestamps_per_second.variances[ss_slot] - ss_val*ss_val;
      ss_stdev = sqrt(fabs(ss_variance));
      ss_ratio;
      if (ss_val) {
         ss_ratio = ss_stdev / fabs(ss_val);
      } else {
         ss_ratio = 0;
      }

      speedstep_warning = (ss_ratio > SPEEDSTEP_DETECTION_RATIO);
   }

   if (!record_data) {
      Prof_traverse(clear_stack);
      Prof_End
      return;
   }
   
   if (timestamps_per_second) {
      timestamps_to_seconds = 1.0 / timestamps_per_second;
   } else {
      timestamps_to_seconds = 0;
   }

#ifdef Prof_ZONE_HISTORY
   for (i=0; i < Prof_num_zones; ++i) {
      Prof_zones[i]->highlevel = (void *) &zone_history[i][history_index];
      zone_history[i][history_index] = 0;
   }
#endif

   Prof_traverse(update_history);

   update(&frame_time, dt, precomputed_factors);

   ++update_index;
   history_index = (history_index + 1) % NUM_FRAME_SLOTS;

   Prof_traverse(clear_stack);

   Prof_End
#endif // Prof_ENABLED
}

static Prof_Report *allocate_buffer(int n)
{
   int i;
   Prof_Report *pob = (Prof_Report *) malloc(sizeof(*pob));
   pob->num_record = n;
   pob->record = (Prof_Report_Record *) malloc(sizeof(*pob->record) * pob->num_record);
   pob->title[0] = pob->title[1] = NULL;
   for (i=0; i < NUM_TITLE; ++i)
      pob->title[i] = NULL;
   for (i=0; i < NUM_HEADER; ++i)
      pob->header[i] = NULL;
   for (i=0; i < n; ++i) {
      pob->record[i].values[0] = 0;
      pob->record[i].values[1] = 0;
      pob->record[i].values[2] = 0;
      pob->record[i].values[3] = 0;
      pob->record[i].value_flag = 0;
      pob->record[i].heat = 0;
      pob->record[i].indent = 0;
      pob->record[i].number = 0;
   }
   return pob;
}

static int uncounted;
static Prof_Recursion_Mode recurse = Prof_FLATTEN_RECURSION;

static void propogate_to_zone(Prof_Zone_Stack *c)
{
   Prof_Zone *z = c->zone;
   Profile_Tracker_Data_Record *d = (Profile_Tracker_Data_Record *) c->highlevel;
   Prof_Report_Record *r;

#if 1
   r = (Prof_Report_Record *) z->highlevel;
#else   
   if (recurse == Prof_FLATTEN_RECURSION)
      r = (Prof_Report_Record *) z->highlevel;
   else
      r = ((Prof_Report_Record **) z->highlevel)[c->recursion_depth];
#endif

   if (d) {
      double t;

      r->values[0] += 1000 * get_value(&d->self_time);
      r->values[1] += 1000 * get_value(&d->hierarchical_time);
      r->values[2] += get_value(&d->entry_count);

      // arbitrary determination for how low a moving average
      // has to go to reach 0
      if (get_value(&d->entry_count) > INT_ZERO_THRESHHOLD) {
         if (d->max_recursion > r->number)
            r->number = d->max_recursion;
         if (c->parent->zone)
            ((Prof_Report_Record *) c->parent->zone->highlevel)->prefix = '+';
      }

#ifdef Prof_CALL_HISTORY
      if (display_frame) return;  // no variances when examining history
#endif
      if (displayed_quantity == Prof_HIERARCHICAL_TIME) {
         t = d->hierarchical_time.variances[slot];
      } else {
         t = d->self_time.variances[slot];
      }

      t = 1000 * 1000 * t;

      if (r->heat == 0)
         r->heat = t;
      else
         r->heat = r->heat + t + 2 * sqrt(r->heat * t);
   } else {
      ++uncounted;
   }
}

static void propogate_expanded(Prof_Zone_Stack *c)
{
   Profile_Tracker_Data_Record *d = (Profile_Tracker_Data_Record *) c->highlevel;
   if (d == NULL) {
      ++uncounted;
      return;
   }
   if (c->parent->zone && get_value(&d->entry_count) > INT_ZERO_THRESHHOLD) {
      ((Prof_Report_Record *) c->parent->zone->highlevel)[0].prefix = '+';
      ((Prof_Report_Record *) c->parent->zone->highlevel)[1].prefix = '+';
      ((Prof_Report_Record *) c->parent->zone->highlevel)[2].prefix = '+';
   }

   if (c->zone == expand) {
      Prof_Report_Record *r = (Prof_Report_Record *) expand->highlevel;
      // accumulate this time to ourselves
      r[2].values[0] += 1000 * get_value(&d->self_time);
      r[2].values[1] += 1000 * get_value(&d->hierarchical_time);
      r[2].values[2] += get_value(&d->entry_count);
      if (d->max_recursion > r[2].number && get_value(&d->entry_count) > INT_ZERO_THRESHHOLD)
         r[2].number = d->max_recursion;
      // propogate it to the parents
      if (c->parent->zone) {
         r = (Prof_Report_Record *) c->parent->zone->highlevel;
         r[1].values[0] += 1000 * get_value(&d->self_time);
         r[1].values[1] += 1000 * get_value(&d->hierarchical_time);
         r[1].values[2] += get_value(&d->entry_count);
         d = (Profile_Tracker_Data_Record *) c->parent->highlevel;
         if (d->max_recursion > r[1].number && get_value(&d->entry_count) > INT_ZERO_THRESHHOLD)
            r[1].number = d->max_recursion;
      }
   }
   
   if (c->parent->zone == expand) {
      Prof_Report_Record *r = (Prof_Report_Record *) c->zone->highlevel;
      r[0].values[0] += 1000 * get_value(&d->self_time);
      r[0].values[1] += 1000 * get_value(&d->hierarchical_time);
      r[0].values[2] += get_value(&d->entry_count);
      if (d->max_recursion > r[0].number && get_value(&d->entry_count) > INT_ZERO_THRESHHOLD)
         r[0].number = d->max_recursion;
   }
}

static double compute_heat(double variance, double value)
{
   double factor, stdev;
   double fabs_value = fabs(value);
   const float VARIANCE_TOLERANCE_FACTOR = 0.5f;

   variance = variance - value*value;
   if (variance < 0) variance = 0;
   stdev = sqrt(variance);

   if (fabs_value < 0.000001) {
      return 0;
   } else {
      factor = (stdev / fabs_value) * (1.0f / VARIANCE_TOLERANCE_FACTOR);
   }

   if (factor < 0) return 0;
   if (factor > 1) return 1;
   return factor;
}

static int pob_compare(const void *p, const void *q)
{
   double a = ((Prof_Report_Record *) p)->values[0];
   double b = ((Prof_Report_Record *) q)->values[0];

   return (b < a) ? -1 : (b > a);
}

static int pob_expand_compare(const void *p, const void *q)
{
   Prof_Report_Record * a = (Prof_Report_Record *) p;
   Prof_Report_Record * b = (Prof_Report_Record *) q;

   if (a->indent != b->indent) {
      if (a->indent == 5) return -1;
      if (b->indent == 5) return 1;
      if (a->indent == 3) return 1;
      if (b->indent == 3) return -1;
      return 0;
   }
   if (a->values[1] == b->values[1])
      return 0;

   if (a->values[1] < b->values[1]) {
      if (a->indent == 5) return -1;
      return 1;
   }

   if (a->indent == 5) return 1;
   return -1;
}

static int cursor;
static int update_cursor;

Prof_Report *Prof_create_report(void)
{
   double avg_frame_time,fps;
   char *displayed_quantity_name;
   int i,s;
   Prof_Report *pob;

   if (displayed_quantity == Prof_CALL_GRAPH)
      s = 3;
   else
      s = 1;
   
   pob = allocate_buffer(Prof_num_zones * s);
   for (i=0; i < Prof_num_zones; ++i) {
      Prof_Zone *z = Prof_zones[i];
      Prof_Report_Record *r = &pob->record[i*s];
      z->highlevel = (void *) r;
      if (displayed_quantity == Prof_CALL_GRAPH) {
         r[0].name = r[1].name = r[2].name = z->name;
         r[0].value_flag = 1 | 2 | 4;
         r[1].value_flag = 1 | 2 | 4;
         r[2].value_flag = 1 | 2 | 4;
         r[0].indent = 3;
         r[1].indent = 5;
         r[2].indent = 0;
         r[0].zone = r[1].zone = r[2].zone = (void *) z;
         r[0].prefix = r[1].prefix = r[2].prefix = 0;
      } else {
         r->value_flag = 1 | 2 | 4;
         r->name = z->name;
         r->zone = (void *) z;
         r->indent = 0;
         r->prefix = 0;
      }
   }

   avg_frame_time = frame_time.values[slot];
   if (avg_frame_time == 0) avg_frame_time = 0.01f;
   fps = 1.0f / avg_frame_time;

   displayed_quantity_name = "*error*";
   switch (displayed_quantity) {
      case Prof_SELF_TIME:
         displayed_quantity_name = "sort self";
         break;
      case Prof_HIERARCHICAL_TIME:
         displayed_quantity_name = "sort hier";
         break;
      case Prof_CALL_GRAPH:
         displayed_quantity_name = "sort hier";
         break;
   }

   pob->title[0] = (char *)  malloc(BUFSIZ);
   sprintf(pob->title[0],
          "%3.3lf ms/frame (fps: %3.2lf)  %s",
           avg_frame_time * 1000, fps, displayed_quantity_name);

#ifdef Prof_CALL_HISTORY
   if (display_frame) {
      sprintf(pob->title[0] + strlen(pob->title[0]), " - %d frame%s ago",
          display_frame, display_frame == 1 ? "" : "s");
   } else {
      strcat(pob->title[0], " - current frame");
   }
#endif

   if (speedstep_warning)
      pob->title[1] = _strdup("WARNING: SpeedStep-like timer inconsistencies detected.  Results are unreliable!");

   if (displayed_quantity == Prof_CALL_GRAPH) {
      Prof_Report_Record *r = (Prof_Report_Record *) expand->highlevel;
      int j=0;

      Prof_traverse(propogate_expanded);

      r[2].prefix = '-';

      for (i=0; i < pob->num_record; ++i) {
         if (pob->record[i].values[0] || pob->record[i].values[1] || pob->record[i].values[2]) {
            pob->record[j] = pob->record[i];
            ++j;
         }
      }
      pob->num_record = j;

      qsort(pob->record, pob->num_record, sizeof(pob->record[0]), pob_expand_compare);

      for (i=0; i < pob->num_record; ++i)
         if (pob->record[i].indent == 5)
             pob->record[i].indent = 3;
   } else {

      uncounted = 0;
      Prof_traverse(propogate_to_zone);

      for (i=0; i < Prof_num_zones; ++i) {
         if (displayed_quantity == Prof_HIERARCHICAL_TIME) {
            double t = pob->record[i].values[0];
            pob->record[i].values[0] = pob->record[i].values[1];
            pob->record[i].values[1] = t;
         }

         pob->record[i].heat = compute_heat(pob->record[i].heat, pob->record[i].values[0]);
      }
         
      qsort(pob->record, pob->num_record, sizeof(pob->record[0]), pob_compare);

   }

   if (update_cursor) {
      for (i=0; i < pob->num_record; ++i) {
         if (pob->record[i].zone == expand) {
            cursor = i;
            break;
         }
      }
      update_cursor = 0;
   }

   pob->header[0] = _strdup("zone");
   if (displayed_quantity == Prof_HIERARCHICAL_TIME) {
      pob->header[1] = _strdup("hier");
      pob->header[2] = _strdup("self");
   } else {
      pob->header[1] = _strdup("self");
      pob->header[2] = _strdup("hier");
   }
   pob->header[3] = _strdup("count");

   if (cursor < 0) cursor = 0;
   if (cursor >= pob->num_record) cursor = pob->num_record-1;
   pob->hilight = cursor;

   return pob;
}

void Prof_free_report(Prof_Report *z)
{
   int i;
   for (i=0; i < NUM_TITLE; ++i)
      if (z->title[i])
         free(z->title[i]);
   for (i=0; i < NUM_HEADER; ++i)
      if (z->header[i])
         free(z->header[i]);
   free(z->record);
   free(z);
}

Prof_extern_C void Prof_move_cursor(int num)
{
   cursor += num;
}

Prof_extern_C void Prof_set_cursor(int num)
{
   cursor = num;
}

Prof_extern_C void Prof_select(void)
{
   Prof_Report *b = Prof_create_report();
   if (b->hilight >= 0) {
      void *z = b->record[b->hilight].zone;
      if (z != NULL) {
         expand = (Prof_Zone *) z;
         displayed_quantity = Prof_CALL_GRAPH;
      }
   }
   Prof_free_report(b);
   update_cursor = 1;
}

Prof_extern_C void Prof_select_parent(void)
{
   int i;
   void *old = (void *) expand;
   Prof_Report *b = Prof_create_report();
   for (i=0; i < b->num_record; ++i) {
      if (b->record[i].indent == 0) break;
      if (b->record[i].zone == old) continue;
      expand = (Prof_Zone *) b->record[i].zone;
   }
   Prof_free_report(b);
   update_cursor = 1;
}

Prof_extern_C void Prof_set_frame(int num)
{
   if (num < 0) num = 0;
   if (num >= NUM_FRAME_SLOTS) num = NUM_FRAME_SLOTS-1;

   display_frame = num;
}

Prof_extern_C void Prof_move_frame(int delta)
{
   // convert so negative delta = "into the past"
   Prof_set_frame(display_frame - delta);
}

Prof_extern_C void Prof_set_smoothing(int x)
{
   if (x <= 0) x = 0;
   if (x >= NUM_PROFILE_TRACKER_HISTORY_SLOTS)
      x = NUM_PROFILE_TRACKER_HISTORY_SLOTS-1;

   slot = x;
}

// currently does nothing
Prof_extern_C void Prof_set_recursion(Prof_Recursion_Mode e)
{
   recurse = e;
}

static int id(Prof_Zone *z)
{
   // hash the string so that the id is consistent from
   // run to run (rather than using the pointer itself which isn't)
   // @TODO: only compute this at zone init time?

   unsigned int h = 0x55555555;
   char *n = z->name;

   while (*n)
      h = (h << 5) + (h >> 27) + *n++;

   return h;
}

void Prof_graph(int num_frames, void (*callback)(int id, int x0, int x1, float *values, void *data), void *data)
{
#ifdef Prof_ZONE_HISTORY
   int i;
   int h = history_index;
   if (num_frames > NUM_FRAME_SLOTS)
      num_frames = NUM_FRAME_SLOTS;

   for (i=0; i < Prof_num_zones; ++i) 
   {
      if (h >= num_frames) 
	  {
         callback(id(Prof_zones[i]), 0, num_frames, &zone_history[i][h-num_frames], data);
      } 
	  else 
	  {
         callback(id(Prof_zones[i]), num_frames - h, num_frames, &zone_history[i][0], data);
         callback(id(Prof_zones[i]), 0, num_frames-h, &zone_history[i][NUM_FRAME_SLOTS-(num_frames-h)], data);
      }
   }

   // display frame "cursor"
   if (display_frame != 0) 
   {
      float value[2] = { 2.0, 0 };
      callback(0, NUM_FRAME_SLOTS-1-display_frame, NUM_FRAME_SLOTS-1-display_frame, value, data);
   }
#endif
}


