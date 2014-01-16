/*
 
  wsserver.h
  
  ol.wsserver embedded websocket/http server for max based on civetweb
 
  v 0.1
  
  -- licence --
 
  ol.wsserver Copyright (c) 2014 Oliver Larkin
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
 
*/

#include "../../maxcpp/maxcpp/maxcpp6.h"

#include "wdl/mutex.h"
#include "wdl/wdlstring.h"
#include "wdl/ptrlist.h"

#include "civetweb/include/civetweb.h"

#define VERSION "0.1"
#define DEFAULT_CONNECTIONS 4
#define MAX_STRING 8192
#define DEFAULT_UPDATE_RATE_MS 50
#define DEFAULT_PORT 8080
#define MAX_CONNECTIONS 32
#define MAX_UPDATE_RATE 60000
#define MIN_UPDATE_RATE 10
#define DEFAULT_DEBUG 0

class wsserver : public MaxCpp6<wsserver> 
{
public:
  struct ws_connection 
  {
    mg_connection *conn;
    int update;
    int closing;
    int index;
    bool newdatafromserver;
    bool newdatatoserver;
    WDL_FastString fromserver;
    WDL_FastString toserver;
    WDL_Mutex mutex;
    
    ws_connection()
    {
      conn = NULL;
      fromserver.Set("", MAX_STRING);
      toserver.Set("", MAX_STRING);
    }
  };

  long mClientUpdateRateMs;
  long mNumConnectionsToAllocate;

private:
  WDL_FastString mServerName;
  WDL_FastString mPortNumber;
  WDL_PtrList<ws_connection> mConnections;
  mg_context* mCtx;
  mg_callbacks mCallbacks;
  void *mClock;
  bool mDebug;

public:
  wsserver(t_symbol *s, long ac, t_atom * av);
  ~wsserver(); 
  
  ws_connection* getConnection(int idx){ return mConnections.Get(idx); }
  int getMaxNConnections() { return mConnections.GetSize(); }
  bool getDebug() { return mDebug; };
  
  void shutdown();
  void onTimer();
  void outputData();
  
//  void bang(long inlet);
  void start(long inlet, t_symbol *s, long ac, t_atom *av);
  void stop(long inlet);
  void tx(long inlet, t_symbol *s, long ac, t_atom *av);
  void debug(long inlet, long v);
  void assist(void *b, long m, long a, char *s);
//  void jit_matrix(long inlet, t_symbol *s, long ac, t_atom *av);
};

extern "C" int C74_EXPORT main(void) 
{
  wsserver::makeMaxClass("ol.wsserver");
  REGISTER_METHOD_GIMME(wsserver, start);
  REGISTER_METHOD(wsserver, stop);
  REGISTER_METHOD_GIMME(wsserver, tx);
//  REGISTER_METHOD(wsserver, bang);
  REGISTER_METHOD_LONG(wsserver, debug);
  REGISTER_METHOD_ASSIST(wsserver, assist);
//  REGISTER_METHOD_GIMME(wsserver, jit_matrix);
}

static void *ws_server_thread(void *parm);
static int websocket_connect_handler(const struct mg_connection *conn);
static void websocket_ready_handler(struct mg_connection *conn);
static void websocket_close_handler(struct mg_connection *conn);
static int websocket_data_handler(struct mg_connection *conn, int flags, char *data, size_t data_len);