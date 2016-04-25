#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include "kvconstants.h"
#include "kvstore.h"
#include "kvmessage.h"
#include "tpcfollower.h"
#include "index.h"
#include "tpclog.h"
#include "socket_server.h"

/* Initializes a tpcfollower. Will return 0 if successful, or a negative error
 * code if not. DIRNAME is the directory which should be used to store entries
 * for this server.  HOSTNAME and PORT indicate where SERVER will be
 * made available for requests. */
int tpcfollower_init(tpcfollower_t *server, char *dirname, unsigned int max_threads,
                     const char *hostname, int port) {
  int ret;
  ret = kvstore_init(&server->store, dirname);
  if (ret < 0)
    return ret;
  ret = tpclog_init(&server->log, dirname);
  if (ret < 0)
    return ret;
  strcpy(server->hostname, hostname);
  server->port = port;
  server->max_threads = max_threads;

  server->state = TPC_INIT;

  /* Rebuild TPC state. */
  tpcfollower_rebuild_state(server);
  return 0;
}

/* Sends a message to register SERVER with a TPCLeader over a socket located at
 * SOCKFD which has previously been connected. Does not close the socket when
 * done. Returns false if an error was encountered.
 */
bool tpcfollower_register_leader(tpcfollower_t *server, int sockfd) {
  kvrequest_t register_req;

  register_req.type = REGISTER;
  strcpy(register_req.key, server->hostname);
  sprintf(register_req.val, "%d", server->port);

  kvrequest_send(&register_req, sockfd);

  kvresponse_t res;
  kvresponse_receive(&res, sockfd);

  return res.type == SUCCESS;
}

/* Attempts to get KEY from SERVER. Returns 0 if successful, else a negative
 * error code.  If successful, VALUE will point to a string which should later
 * be free()d.  */
int tpcfollower_get(tpcfollower_t *server, char *key, char *value) {
  int ret;
  if (strlen(key) > MAX_KEYLEN)
    return ERR_KEYLEN;
  ret = kvstore_get(&server->store, key, value);
  return ret;
}

/* Checks if the given KEY, VALUE pair can be inserted into this server's
 * store. Returns 0 if it can, else a negative error code. */
int tpcfollower_put_check(tpcfollower_t *server, char *key, char *value) {
  int check;
  if (strlen(key) > MAX_KEYLEN || strlen(key) == 0)
    return ERR_KEYLEN;
  if (strlen(value) > MAX_VALLEN)
    return ERR_VALLEN;
  if ((check = kvstore_put_check(&server->store, key, value)) < 0)
    return check;
  return 0;
}

/* Inserts the given KEY, VALUE pair into this server's store
 * Returns 0 if successful, else a negative error code. */
int tpcfollower_put(tpcfollower_t *server, char *key, char *value) {
  int ret;
  if ((ret = tpcfollower_put_check(server, key, value)) < 0)
    return ret;
  ret = kvstore_put(&server->store, key, value);
  return ret;
}

/* Checks if the given KEY can be deleted from this server's store.
 * Returns 0 if it can, else a negative error code. */
int tpcfollower_del_check(tpcfollower_t *server, char *key) {
  int check;
  if (strlen(key) > MAX_KEYLEN || strlen(key) == 0)
    return ERR_KEYLEN;
  if ((check = kvstore_del_check(&server->store, key)) < 0)
    return check;
  return 0;
}

/* Removes the given KEY from this server's store. Returns
 * 0 if successful, else a negative error code. */
int tpcfollower_del(tpcfollower_t *server, char *key) {
  int ret;
  if ((ret = tpcfollower_del_check(server, key)) < 0)
    return ret;
  ret = kvstore_del(&server->store, key);
  return ret;
}

/* Handles an incoming kvrequest REQ, and populates RES as a response.  REQ and
 * RES both must point to valid kvrequest_t and kvrespont_t structs,
 * respectively. Assumes that the request should be handled as a TPC
 * message. This should also log enough information in the server's TPC log to
 * be able to recreate the current state of the server upon recovering from
 * failure. See the spec for details on logic and error messages.
 */

/*
typedef struct {
  msgtype_t type;
  char key[MAX_KEYLEN + 1]; // May be NULL, depending on type.
  char val[MAX_VALLEN + 1]; // May be NULL, depending on type.
} kvrequest_t;

typedef struct {
  msgtype_t type;
  char body[KVRES_BODY_MAX_SIZE + 1]; // May be NULL, depending on type.
} kvresponse_t;
*/

// typedef struct tpcfollower {
//   kvstore_t store; /* The store this server will use. */
//   tpclog_t log;    /* The log this server will use. */
//   tpc_state_t state;
//   msgtype_t pending_msg;
//   char pending_key[MAX_KEYLEN + 1];
//   char pending_value[MAX_VALLEN + 1];
//   int max_threads;   /* The max threads this server will run on. */
//   int listening;     /* 1 if this server is currently listening for requests, else 0. */
//   int sockfd;        /* The socket fd this server is currently listening on (if any).  */
//   int port;          /* The port this server should listen on. */
//   char hostname[64]; /* The host this server should listen on. */
// } tpcfollower_t; 

void tpcfollower_handle_tpc(tpcfollower_t *server, kvrequest_t *req, kvresponse_t *res) {
  /* TODO: Implement me! */
  msgtype_t service_type = req->type;
  if (service_type == PUTREQ) 
  {
    tpclog_log(&(server->log), service_type, req->key, req->val);
    if (tpcfollower_put_check(server, req->key, req->val) == 0 && server->state != TPC_READY) 
    {
      strcpy(server->pending_key, req->key);
      strcpy(server->pending_value, req->val);
      server->pending_msg = service_type;
      server->state = TPC_READY;
      res->type = VOTE;
      strcpy(res->body, MSG_COMMIT);
    }
    else 
    {
      res->type = ERROR;
      strcpy(res->body, ERRMSG_INVALID_REQUEST);
    }
  } 
  else if (service_type == GETREQ)
  {
    tpclog_log(&(server->log), service_type, req->key, NULL);
    if (tpcfollower_get(server, req->key, req->val) == 0)
    {
      res->type = GETRESP;
      strcpy(res->body, req->val);
    }
    else 
    {
      res->type = ERROR;
      strcpy(res->body, ERRMSG_NO_KEY);
    }
  }
  else if (service_type == COMMIT)
  {
    tpclog_log(&(server->log), service_type, NULL, NULL);
    if (server->state == TPC_READY && server->pending_msg == PUTREQ)
    {
      if (tpcfollower_put(server, server->pending_key, server->pending_value) == 0)
      {
        memset(server->pending_value, 0, MAX_VALLEN+1);
        memset(server->pending_key, 0, MAX_KEYLEN+1);
        server->state = TPC_COMMIT;
        res->type = ACK;
      }
      else 
      {
        res->type = ERROR;
        strcpy(res->body, ERRMSG_GENERIC_ERROR);
      }
    }
    else if (server->state == TPC_COMMIT)
    {
      res->type = ACK;
    }
    else if (server->state == TPC_READY && server->pending_msg == DELREQ)
    {
      if (tpcfollower_del(server, server->pending_key) == 0)
      {
        memset(server->pending_value, 0, MAX_VALLEN+1);
        server->state = TPC_COMMIT;
        res->type = ACK;
      }
      else 
      {
        res->type = ERROR;
        strcpy(res->body, ERRMSG_NO_KEY);
      }
    }
  }
  else if (service_type == ABORT)
  {
    tpclog_log(&(server->log), service_type, NULL, NULL);
    memset(server->pending_value, 0, MAX_VALLEN+1);
    memset(server->pending_key, 0, MAX_KEYLEN+1);
    server->state = TPC_ABORT;
    res->type = ACK;
  }
  else if (service_type == DELREQ)
  {
    tpclog_log(&(server->log), service_type, req->key, NULL);
    if (tpcfollower_del_check(server, req->key) == 0 && server->state != TPC_READY) 
    {
      strcpy(server->pending_key, req->key);
      memset(server->pending_value, 0, MAX_VALLEN+1);
      server->pending_msg = service_type;
      server->state = TPC_READY;
      res->type = VOTE;
      strcpy(res->body, MSG_COMMIT);
    }
    else 
    {
      res->type = ERROR;
      strcpy(res->body, ERRMSG_INVALID_REQUEST);
    }
  }
}

/* Generic entrypoint for this SERVER. Takes in a socket on SOCKFD, which
 * should already be connected to an incoming request. Processes the request
 * and sends back a response message.  This should call out to the appropriate
 * internal handler. */
void tpcfollower_handle(tpcfollower_t *server, int sockfd) {
  kvrequest_t req;
  kvresponse_t res;
  bool success = kvrequest_receive(&req, sockfd);
  do {
    if (!success) {
      res.type = ERROR;
      strcpy(res.body, ERRMSG_INVALID_REQUEST);
    } else if (req.type == INDEX) {
      index_send(sockfd, 0);
      break;
    } else {
      tpcfollower_handle_tpc(server, &req, &res);
    }
    kvresponse_send(&res, sockfd);
  } while (0);
}

/* Restore SERVER back to the state it should be in, according to the
 * associated LOG.  Must be called on an initialized  SERVER. Only restores the
 * state of the most recent TPC transaction, assuming that all previous actions
 * have been written to persistent storage. Should restore SERVER to its exact
 * state; e.g. if SERVER had written into its log that it received a PUTREQ but
 * no corresponding COMMIT/ABORT, after calling this function SERVER should
 * again be waiting for a COMMIT/ABORT.  This should also ensure that as soon
 * as a server logs a COMMIT, even if it crashes immediately after (before the
 * KVStore has a chance to write to disk), the COMMIT will be finished upon
 * rebuild.
 */
int tpcfollower_rebuild_state(tpcfollower_t *server) {
  /* TODO: Implement me! */
  return -1;
}

/* Deletes all current entries in SERVER's store and removes the store
 * directory.  Also cleans the associated log. Note that you will be required
 * to reinitialize SERVER following this action. */
int tpcfollower_clean(tpcfollower_t *server) { return kvstore_clean(&server->store); }
