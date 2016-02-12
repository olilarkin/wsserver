/*
 
  wsserver.cpp
  
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

//disable MSVC unknown pragma warning
#pragma warning( disable : 4068 )

#include "wsserver.h"
//#include "jit.common.h"

wsserver::wsserver(t_symbol *s, long ac, t_atom * av)
: mCtx(NULL)
{
  setupIO(1, 1);

  long port = DEFAULT_PORT;
  long numconnections = DEFAULT_CONNECTIONS;
  long updaterate = DEFAULT_UPDATE_RATE_MS;
  long debug = DEFAULT_DEBUG;
  
  if (ac && av) 
  {
    if (ac > 0)
      port = atom_getlong(&av[0]);
    
    if (ac > 1)
      numconnections = atom_getlong(&av[1]);
    
    if (ac > 2)
      updaterate = atom_getlong(&av[2]);
    
    if (ac > 3)
      debug = atom_getlong(&av[3]);
  }
  
  if (port > 0 && port < 65535)
    mPortNumber.SetFormatted(32, "%ld", port);
  
  if (numconnections > 0 && numconnections < MAX_CONNECTIONS)
    mNumConnectionsToAllocate = numconnections;
  
  if (updaterate > MIN_UPDATE_RATE && updaterate < MAX_UPDATE_RATE)
    mClientUpdateRateMs = updaterate;
  
  if (debug > 0)
    mDebug = true;
  
  mClock = clock_new(this, TO_METHOD_NONE(wsserver, onTimer));
}

wsserver::~wsserver()
{ 
  clock_unset(mClock);
  freeobject((t_object *)mClock);
  shutdown();
}

void wsserver::shutdown()
{
  if (mCtx) 
  {
    post("stopping server");
    mg_stop(mCtx);
    mConnections.Empty(true);
  }
  
  mCtx = NULL;
}

void wsserver::outputData()
{
  t_atom argv[2];
  
  for (int i=0; i<getMaxNConnections(); i++) 
  {
    wsserver::ws_connection* ws_conn = getConnection(i);
    WDL_MutexLock(&ws_conn->mutex);

    if(ws_conn->newdatatoserver && ws_conn->toserver.GetLength())
    {
      atom_setlong(&argv[0], i); 

      if (strncmp(ws_conn->toserver.Get(), "cx", 2) == 0) // connected
      {
        atom_setlong(&argv[1], 1);
        outlet_anything(m_outlets[0], gensym("cx"), 2, argv);
      }
      else if (strncmp(ws_conn->toserver.Get(), "dx", 2) == 0) // disconnected
      {
        atom_setlong(&argv[1], 0);
        outlet_anything(m_outlets[0], gensym("cx"), 2, argv);
      }
      else // message
      {
        atom_setsym(&argv[1], gensym(ws_conn->toserver.Get()));
        outlet_anything(m_outlets[0], gensym("rx"), 2, argv);
      }
          
      ws_conn->newdatatoserver = false;
    }
  }
}

void wsserver::onTimer()
{
  outputData();
  clock_fdelay(mClock, 5.); // schedule the next clock
}

//void wsserver::bang(long inlet)
//{
//  outputData();
//}

void wsserver::start(long inlet, t_symbol *s, long ac, t_atom *av)
{
  if (ac) 
  {
    if (atom_gettype(av) == A_SYM)
    {
      // TODO: check that webroot path is valid
      if (mCtx == NULL) 
      {
        const char *options[] = {
          "listening_ports", mPortNumber.Get(),
          "document_root", atom_getsym(av)->s_name,
          NULL
        };
      
        mServerName.SetFormatted(128, "ol.wsserver v. %s", VERSION);
      
        memset(&mCallbacks, 0, sizeof(mCallbacks));
        mCallbacks.websocket_connect = websocket_connect_handler;
        mCallbacks.websocket_ready = websocket_ready_handler;
        mCallbacks.websocket_data = websocket_data_handler;
        mCallbacks.connection_close = websocket_close_handler;
      
        for (int i=0; i<mNumConnectionsToAllocate; i++)
          mConnections.Add(new ws_connection());
      
        mCtx = mg_start(&mCallbacks, this /* so that we can get a pointer to this in the static mCallbacks */, options);
      
        if(mCtx)
        {
          post("%s started on port(s) %s with web root [%s]\n", mServerName.Get(), mg_get_option(mCtx, "listening_ports"), mg_get_option(mCtx, "document_root"));
          post("maximium %i clients\n", mNumConnectionsToAllocate);
        
          clock_fdelay(mClock, 0.);
        }
        else
          error("couldn't start server: perhaps port is allready being used?");
      }
      else
        error("server already started");
    }
    else
      error("missing argument for message start");
  } 
  else
    error("missing argument for message start");
}

void wsserver::stop(long inlet)
{
  clock_unset(mClock);
  shutdown();
}

void wsserver::tx(long inlet, t_symbol *s, long ac, t_atom *av)
{ 
  if (ac) 
  {
    if (ac == 1 && atom_gettype(av) == A_SYM)
    {      
      for (int i=0; i<getMaxNConnections(); i++) 
      {
        ws_connection* ws_conn = getConnection(i);
        WDL_MutexLock(&ws_conn->mutex);
        ws_conn->fromserver.SetFormatted(MAX_STRING, "rx %s", atom_getsym(av)->s_name);
        ws_conn->newdatafromserver = true;
      }
    }
    else if (ac == 2 && atom_gettype(av) == A_LONG)
    {      
      ws_connection* ws_conn = getConnection(atom_getlong(av));
      if (ws_conn) {
        WDL_MutexLock(&ws_conn->mutex);
        ws_conn->fromserver.SetFormatted(MAX_STRING, "rx %s", atom_getsym(av+1)->s_name);
        ws_conn->newdatafromserver = true;
      }
      else
        error("invalid client for message tx");
    }
    else
      error("missing argument for message tx");
  } 
  else
    error("missing argument for message tx");
}

void wsserver::debug(long inlet, long v)
{
  mDebug = v > 0;
}

void wsserver::assist(void *b, long m, long a, char *s)
{
  if (m == ASSIST_INLET) 
    sprintf(s, "messages to clients");
  else 
    sprintf(s, "messages from clients");
}

//void wsserver::jit_matrix(long inlet, t_symbol *s, long ac, t_atom *av)
//{
//  void *matrix = NULL;
//  
//  if (ac && av) 
//  {
//    matrix = jit_object_findregistered(jit_atom_getsym(av));
//
//    if (matrix && jit_object_method(matrix, _jit_sym_class_jit_matrix)) 
//    {
//      long dimcount, savelock;
//      t_jit_matrix_info info;
//      char *bp;
//
//      savelock = (long) jit_object_method(matrix, _jit_sym_lock, 1);
//
//      jit_object_method(matrix, _jit_sym_getinfo, &info);
//      jit_object_method(matrix, _jit_sym_getdata, &bp);
//
//      if (!bp) 
//      {
//        jit_error_sym(this, _jit_sym_err_calculate);
//        jit_object_method(matrix, _jit_sym_lock, savelock);
//        return;
//      }
//
//      dimcount = info.dimcount;
//
//      if (info.type == _jit_sym_char) 
//      {
//      // handle char
//      } 
//      else if (info.type == _jit_sym_long) 
//      {
//      // handle long
//      } 
//      else if (info.type == _jit_sym_float32) 
//      {
//      // handle float32
//      } 
//      else if (info.type == _jit_sym_float64) 
//      {
//      // handle float64
//      }
//      
//      jit_object_method(matrix, _jit_sym_lock, savelock);
//    }
//  }
//}

#pragma mark -
#pragma mark civetweb callbacks

#ifdef _WIN32
//https://gist.github.com/ngryman/6482577
void usleep(DWORD waitTime)
{
  LARGE_INTEGER perfCnt, start, now;
 
  QueryPerformanceFrequency(&perfCnt);
  QueryPerformanceCounter(&start);
 
  do {
    QueryPerformanceCounter((LARGE_INTEGER*) &now);
  } while ((now.QuadPart - start.QuadPart) / float(perfCnt.QuadPart) * 1000 * 1000 < waitTime);
}
#endif

void *ws_server_thread(void *parm)
{
  wsserver::ws_connection* ws_conn = static_cast<wsserver::ws_connection*>(parm);
  mg_connection *conn = ws_conn->conn;
  wsserver* object = static_cast<wsserver*>(mg_get_request_info(conn)->user_data);

  int timer = 0;
  
  if(object->getDebug()) post("ws_server_thread %d\n", ws_conn->index);
  
  /* While the connection is open, send periodic updates */
  while(!ws_conn->closing) {
    usleep(object->mClientUpdateRateMs * 1000);
    timer++;

    WDL_MutexLock(&ws_conn->mutex);

    if (ws_conn->update && ws_conn->newdatafromserver) 
    {
      if (!ws_conn->closing) 
      {        
        if (ws_conn->fromserver.GetLength())
          mg_websocket_write(conn, WEBSOCKET_OPCODE_TEXT, ws_conn->fromserver.Get(), ws_conn->fromserver.GetLength());
        
        ws_conn->newdatafromserver = false;
      }
    }
    
    /* Send periodic PING to assure websocket remains connected, except if we are closing */
    if (timer%100 == 0 && !ws_conn->closing)
      mg_websocket_write(conn, WEBSOCKET_OPCODE_PING, NULL, 0);
  }
  
  if(object->getDebug()) post("ws_server_thread %d exiting\n", ws_conn->index);
  
  WDL_MutexLock(&ws_conn->mutex);

  // reset connection information to allow reuse by new client
  ws_conn->update = 0;
  ws_conn->index = -1;
  ws_conn->newdatafromserver = false;
  ws_conn->newdatatoserver = true;
  ws_conn->fromserver.Set("");
  if(ws_conn->toserver.Get()) ws_conn->toserver.SetFormatted(MAX_STRING, "dx");
  ws_conn->conn = NULL;
  ws_conn->closing = 2;
  return NULL;
}

// On new client connection, find next available server connection and store
// new connection information. If no more server connections are available
// tell civetweb to not accept the client request.
int websocket_connect_handler(const struct mg_connection *conn)
{
  int i;
  wsserver* object = static_cast<wsserver*>(mg_get_request_info((mg_connection*)conn /* de-const */)->user_data);

  if(object->getDebug()) post("connect handler\n");
 
  for(i=0; i < object->getMaxNConnections(); ++i) 
  {
    wsserver::ws_connection* ws_conn = object->getConnection(i);
    
    WDL_MutexLock(&ws_conn->mutex);

    if (ws_conn->conn == NULL) 
    {
      if(object->getDebug()) post("...prep for server %d\n", i);
      ws_conn->conn = (struct mg_connection *)conn;
      ws_conn->closing = 0;
      ws_conn->update = 0;
      ws_conn->index = i;
      ws_conn->toserver.SetFormatted(MAX_STRING, "cx");
      ws_conn->newdatatoserver = true;
      break;
    }
  }
  
  if (i >= object->getMaxNConnections()) 
  {
    if(object->getDebug()) post("Refused connection: Max connections exceeded\n");
    return 1;
  }
  
  return 0;
}

// Once websocket negotiation is complete, start a server for the connection
void websocket_ready_handler(struct mg_connection *conn)
{ 
  wsserver* object = static_cast<wsserver*>(mg_get_request_info(conn)->user_data);

  if(object->getDebug()) post("ready handler\n");
  
  for(int i=0; i < object->getMaxNConnections(); ++i) 
  {
    if (object->getConnection(i)->conn == conn) 
    {
      if(object->getDebug()) post("...start server %d\n", i);
      mg_start_thread(ws_server_thread, (void *) object->getConnection(i));
      break;
    }
  }
}

// When websocket is closed, tell the associated server to shut down
void websocket_close_handler(const struct mg_connection *conn)
{
  wsserver* object = static_cast<wsserver*>(mg_get_request_info(conn)->user_data);

  if(object->getDebug()) post("close handler\n");   /* called for every close, not just websockets */
 
  for(int i=0; i < object->getMaxNConnections(); ++i) 
  {
    wsserver::ws_connection* ws_conn = object->getConnection(i);
    
    WDL_MutexLock(&ws_conn->mutex);
    
    if (ws_conn->conn == conn)
    {
      if(object->getDebug()) post("...close server %d\n", i);
      
      ws_conn->closing = 1;
    }
  }
}

// Arguments:
//   flags: first byte of websocket frame, see websocket RFC,
//          http://tools.ietf.org/html/rfc6455, section 5.2
//   data, data_len: payload data. Mask, if any, is already applied.
int websocket_data_handler(struct mg_connection *conn, int flags, char *data, size_t data_len)
{
  int i;
  
  wsserver* object = static_cast<wsserver*>(mg_get_request_info(conn)->user_data);
  wsserver::ws_connection* ws_conn;

  for(i=0; i < object->getMaxNConnections(); ++i) 
  {
    if (object->getConnection(i)->conn == conn) 
    {
      ws_conn = object->getConnection(i);
      break;
    }
  }
  
  if (i >= object->getMaxNConnections()) 
  {
    if(object->getDebug()) post("Received websocket data from unknown connection\n");
    return 1;
  }
  
  if (flags & 0x80) {
    flags &= 0x7f;
    
    WDL_MutexLock(&ws_conn->mutex);

    switch (flags) {
      case WEBSOCKET_OPCODE_CONTINUATION:
        if(object->getDebug()) post("CONTINUATION...\n");
        break;
      case WEBSOCKET_OPCODE_TEXT:
        
        if (strncmp("update on", data, data_len) == 0) {
          /* turn on updates */
          ws_conn->update = 1;
          /* echo back */
          mg_websocket_write(conn, WEBSOCKET_OPCODE_TEXT, data, data_len);
        } else if (strncmp("update off", data, data_len)== 0) {
          /* turn off updates */
          ws_conn->update = 0;
          /* echo back */
          mg_websocket_write(conn, WEBSOCKET_OPCODE_TEXT, data, data_len);
        }
        else {
          ws_conn->newdatatoserver = true;
          ws_conn->toserver.SetFormatted(MAX_STRING, "%-.*s\n", (int)data_len, data);
        }

        break;
      case WEBSOCKET_OPCODE_BINARY:
        if(object->getDebug()) post("BINARY...\n");
        break;
      case WEBSOCKET_OPCODE_CONNECTION_CLOSE:
        if(object->getDebug()) post("CLOSE...\n");
        /* If client initiated close, respond with close message in acknowlegement */
        if (!ws_conn->closing) {
          mg_websocket_write(conn, WEBSOCKET_OPCODE_CONNECTION_CLOSE, data, data_len);
          ws_conn->closing = 1; /* we should not send addional messages when close requested/acknowledged */
        }
        return 0; /* time to close the connection */
        break;
      case WEBSOCKET_OPCODE_PING:
        /* client sent PING, respond with PONG */
        mg_websocket_write(conn, WEBSOCKET_OPCODE_PONG, data, data_len);
        break;
      case WEBSOCKET_OPCODE_PONG:
        /* received PONG to our PING, no action */
        break;
      default:
        if(object->getDebug()) post("Unknown flags: %02x\n", flags);
        break;
    }
  }
  
  return 1;   /* keep connection open */
}