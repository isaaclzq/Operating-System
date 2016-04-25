#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include "kvconstants.h"
#include "kvmessage.h"
#include "index.h"
#include "md5.h"
#include "socket_server.h"
#include "time.h"
#include "tpcleader.h"

/* Initializes a tpcleader. Will return 0 if successful, or a negative error
 * code if not. FOLLOWER_CAPACITY indicates the maximum number of followers that
 * the leader will support. REDUNDANCY is the number of replicas (followers)
 * that
 * each key will be stored in. */
int tpcleader_init(tpcleader_t *leader, unsigned int follower_capacity, unsigned int redundancy) {
  int ret;
  ret = pthread_rwlock_init(&leader->follower_lock, NULL);
  if (ret < 0)
    return ret;

  leader->follower_count = 0;
  leader->follower_capacity = follower_capacity;
  if (redundancy > follower_capacity) {
    leader->redundancy = follower_capacity;
  } else {
    leader->redundancy = redundancy;
  }
  leader->followers_head = NULL;
  return 0;
}

/* Handles an incoming kvrequest REQ, and populates RES as a response. REQ and
 * RES both must point to valid kvrequest_t and kvrespont_t structs,
 * respectively. Assigns an ID to the follower by hashing a string in the format
 * PORT:HOSTNAME, then tries to add its info to the LEADER's list of followers.
 * If
 * the follower is already in the list, do nothing (success).  There can never
 * be
 * more followers than the LEADER's follower_capacity.  RES will be a SUCCESS if
 * registration succeeds, or an error otherwise.
 */
void tpcleader_register(tpcleader_t *leader, kvrequest_t *req, kvresponse_t *res) {
  if (leader->follower_count == leader->follower_capacity) {
    res->type = ERROR;
    strcpy(res->body, ERRMSG_FOLLOWER_CAPACITY);
    return;
  }

  follower_t *new_follower = malloc(sizeof(tpcfollower_t));
  if (!new_follower)
    fatal_malloc();

  new_follower->host = malloc(strlen(req->key) + 1);
  if (!new_follower->host)
    fatal_malloc();
  strcpy(new_follower->host, req->key);
  new_follower->port = atoi(req->val);
  char address[strlen(new_follower->host) + strlen(req->val)];
  sprintf(address, "%s:%s", req->val, new_follower->host);
  new_follower->id = strhash64(address);
  new_follower->prev = new_follower;
  new_follower->next = new_follower;

  res->type = SUCCESS;
  pthread_rwlock_wrlock(&leader->follower_lock);
  if (!leader->followers_head) {
    leader->followers_head = new_follower;
    leader->follower_count++;
    goto end;
  }
  follower_t *first_follower = leader->followers_head;
  follower_t *curr_follower = first_follower;
  do {
    if (curr_follower->id > new_follower->id) {
      new_follower->next = curr_follower;
      new_follower->prev = curr_follower->prev;
      curr_follower->prev = new_follower;
      new_follower->prev->next = new_follower;
      if (curr_follower == first_follower)
        leader->followers_head = new_follower;
      leader->follower_count++;
      goto end;
    } else if (curr_follower->id == new_follower->id) {
      goto end;
    }
    curr_follower = curr_follower->next;
  } while (curr_follower != first_follower);
  /* New follower has highest ID. */
  new_follower->next = leader->followers_head;
  new_follower->prev = leader->followers_head->prev;
  leader->followers_head->prev = new_follower;
  new_follower->prev->next = new_follower;
  leader->follower_count++;

end:
  pthread_rwlock_unlock(&leader->follower_lock);
  return;
}

/* Hashes KEY and finds the first follower that should contain it.
 * It should return the first follower whose ID is greater than the
 * KEY's hash, and the one with lowest ID if none matches the
 * requirement. Returns NULL if we're not yet at our follower capacity.
 */
follower_t *tpcleader_get_primary(tpcleader_t *leader, char *key) {
  if (leader->follower_count < leader->follower_capacity)
    return NULL;
  uint64_t hash = strhash64(key);
  pthread_rwlock_wrlock(&leader->follower_lock);
  follower_t *curr_follower = leader->followers_head;
  do {
    if (curr_follower->id > hash) {
      pthread_rwlock_unlock(&leader->follower_lock);
      return curr_follower;
    }
    curr_follower = curr_follower->next;
  } while (curr_follower != leader->followers_head);
  curr_follower = leader->followers_head;
  pthread_rwlock_unlock(&leader->follower_lock);
  return curr_follower;
}

/* Returns the follower whose ID comes after PREDECESSOR's, sorted
 * in increasing order.
 */
follower_t *tpcleader_get_successor(tpcleader_t *leader, follower_t *predecessor) {
  pthread_rwlock_wrlock(&leader->follower_lock);
  follower_t *result = predecessor->next;
  pthread_rwlock_unlock(&leader->follower_lock);
  return result;
}

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

// typedef struct tpcleader {
//   unsigned int follower_capacity; /* The number of followers this leader will use. */
//   unsigned int follower_count;    /* The current number of followers this leader is aware of. */
//   unsigned int redundancy;        /* The number of followers a single value will be stored on. */
//   follower_t *followers_head;     /* The head of the list of followers. */
//   pthread_rwlock_t follower_lock; /* A lock used to protect the list of followers. */
// } tpcleader_t;


/* Handles an incoming GET request REQ, and populates response RES. REQ and
 * RES both must point to valid kvrequest_t and kvrespont_t structs,
 * respectively.
 */
void tpcleader_handle_get(tpcleader_t *leader, kvrequest_t *req, kvresponse_t *res) {
  /* TODO: Implement me! */
  msgtype_t service_type = req->type;
  if (service_type == GETREQ)
  {
    int i = 0;
    int follower_fd;
    int byte;
    bool received;
    follower_t *follower = tpcleader_get_primary(leader, req->key);
    for (; i < leader->redundancy-1; i++)
    {
      follower_fd = connect_to(follower->host, follower->port, TIMEOUT);
      if (follower_fd != -1)
      {
        byte = kvrequest_send(req, follower_fd);
      // if (byte == 1)
      // {
      //   res->type = ERROR;
      //   strcpy(res->body, )
      // }
        if (kvresponse_receive(res, follower_fd) && res->body == VOTE)
        {
          break;
        }
        follower = tpcleader_get_successor(leader, follower);  
      }    
    }
  }
  else 
  {
    res->type = ERROR;
    strcpy(res->body, ERRMSG_INVALID_REQUEST);  
  }
}

/* Handles an incoming TPC request REQ, and populates RES as a response.
 * REQ and RES both must point to valid kvrequest_t and kvrespont_t structs,
 * respectively.
 *
 * Implements the TPC algorithm, polling all the followers for a vote first and
 * sending a COMMIT or ABORT message in the second phase.  Must wait for an ACK
 * from every follower after sending the second phase messages.
 */
void tpcleader_handle_tpc(tpcleader_t *leader, kvrequest_t *req, kvresponse_t *res) {
  if (leader->follower_count != leader->follower_capacity) {
    res->type = ERROR;
    strcpy(res->body, ERRMSG_NOT_AT_CAPACITY);
    return;
  }
  /* TODO: Implement me! */
  int i = 0;
  int follower_fd;
  bool received;
  msgtype_t phase1_result = COMMIT;
  follower_t *follower = tpcleader_get_primary(leader, req->key);
  for (; i < leader->redundancy-1; i++)
  {
    follower_fd = connect_to(follower->host, follower->port, TIMEOUT);
    if (follower_fd != -1)
    {
      kvrequest_send(req, follower_fd);
      kvresponse_receive(res, follower_fd);
      if (res->type == ABORT)
      {
        req->type = phase1_result = ABORT;
        break;
      }
      follower = tpcleader_get_successor(leader, follower);
    } 
    else 
    {
      req->type = phase1_result = ABORT;
      break;
    }
  }
  if (phase1_result != ABORT){ req->type = phase1_result;}

  follower = tpcleader_get_primary(leader, req->key);
  for (i = 0; i < leader->redundancy-1; i++)
  {
    while (true)
    {
      follower_fd = connect_to(follower->host, follower->port, TIMEOUT);
      if (follower_fd != -1)
      {
        break;
      }
    }
    while (true)
    {
      kvrequest_send(req, follower_fd);
      kvresponse_receive(res, follower_fd);
      if (res->type == ACK) { break; }
    }
    follower = tpcleader_get_successor(leader, follower); 
  }
  res->type = SUCCESS;
}

/* Generic entrypoint for this LEADER. Takes in a socket on SOCKFD, which
 * should already be connected to an incoming request. Processes the request
 * and sends back a response message.  This should call out to the appropriate
 * internal handler. */
void tpcleader_handle(tpcleader_t *leader, int sockfd) {
  kvresponse_t res;
  kvrequest_t req;
  bool success = kvrequest_receive(&req, sockfd);

  do {
    if (!success) {
      res.type = ERROR;
      strcpy(res.body, ERRMSG_INVALID_REQUEST);
    } else if (req.type == INDEX) {
      index_send(sockfd, 1);
      break;
    } else if (req.type == REGISTER) {
      tpcleader_register(leader, &req, &res);
    } else if (req.type == GETREQ) {
      tpcleader_handle_get(leader, &req, &res);
    } else {
      tpcleader_handle_tpc(leader, &req, &res);
    }
    kvresponse_send(&res, sockfd);
  } while (0);
}
