#ifndef AMALGAMATION
  #include "config.h"
  #include "State.h"
  #include "Collection.h"
  #include "Document.h"
  #include "parson.h"
  #include "http/http.h"

  #include "palloc/sstream.h"
  #include <palloc/palloc.h>
#endif

#include <time.h>
#include <string.h>

struct bgState *bg;

void bgUpdate()
{
  /* For updating interval */
  size_t i = 0;
  time_t tNow = time(NULL);

  /* For serializing documents if needed */
  sstream *ser = sstream_new();
  struct bgCollection* c = NULL;
  JSON_Value* v = NULL;
  size_t j = 0;

  /* Updating Interval */
  bg->intervalTimer -= (tNow - bg->t)*1000;
  bg->t = tNow;

  /* Polling collections http connections to push through data */
  for(i = 0; i < vector_size(bg->collections); i++)
  {
    if(vector_at(bg->collections, i))
    {
      HttpRequestComplete(vector_at(bg->collections, i)->http);
    }
  }

  /* Pushing data if interval is done */
  if(bg->intervalTimer <= 0)
  {
    /* Upload collections */
    for(i = 0; i < vector_size(bg->collections); i++)
    {
      c = vector_at(bg->collections, i);
      if(c)
      {
        if(HttpRequestComplete(c->http))
        {
          /* Serializing documents of collection */
           //Concatenating strings onto ser - dangerous?
          for(j = 0; j < vector_size(c->documents); j++)
          {
            if(vector_at(c->documents, j))
            {
              v = vector_at(c->documents, j)->rootVal;
              //strcat(ser, json_serialize_to_string_pretty(v));
              sstream_push_cstr(ser, json_serialize_to_string_pretty(v));
            }
          }  
          HttpRequest(c->http, sstream_cstr(bg->fullUrl), sstream_cstr(ser)); 
          //pfree(ser);
          sstream_clear(ser);
          bgCollectionDestroy(c);
          c = NULL;
        }
      }
    }

    /* Resetting intervalTimer */
    bg->intervalTimer = bg->interval;

    /* Cleaning up sstream */
    sstream_delete(ser);
  }
}

void bgAuth(char *guid, char *key)
{
  bg = palloc(struct bgState);
  bg->collections = vector_new(struct bgCollection *);
  bg->interval = 2000;

  bg->url = BG_URL;
  bg->path = BG_PATH;
  
  bg->fullUrl = sstream_new();
  sstream_push_cstr(bg->fullUrl, bg->url);
  sstream_push_cstr(bg->fullUrl, bg->path);

  bg->guid = guid;
  bg->key = key;

  bg->updateFunc = bgUpdate;
}

void bgCleanup()
{
  /*
  Looping throough and calling 'destructor'
  and then deleting remnants with vector_delete
  */
  size_t i = 0;
  for(i = 0; i < vector_size(bg->collections); i ++)
  {
    /*NULLS pointer in function*/
    bgCollectionDestroy(vector_at(bg->collections, i));
  }
  vector_delete(bg->collections);
  
  pfree(bg);
}

void bgInterval(int milli)
{
  bg->interval = milli;
}

void bgErrorFunc(void (*errorFunc)(char *cln, int code))
{
  bg->errorFunc = errorFunc;
}

void bgSuccessFunc(void (*successFunc)(char *cln, int count))
{
  bg->successFunc = successFunc;
}

