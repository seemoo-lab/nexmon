#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "linkedlist.h"

pthread_mutex_t clist_mutex = PTHREAD_MUTEX_INITIALIZER;

struct clist *search_status(struct clist *c, int desired_status)
{
  if (!c) return NULL;
  
  pthread_mutex_lock(&clist_mutex);
  struct clist *first = c;

  do {
    if (c->status == desired_status) {
      pthread_mutex_unlock(&clist_mutex);
      return c;
    }
    c = c->next;
  } while (c != first);

  pthread_mutex_unlock(&clist_mutex);
  return NULL;
}

struct clistwidsclient *search_status_widsclient(struct clistwidsclient *c, int desired_status, int desired_channel)
{
  if (!c) return NULL;
  
  pthread_mutex_lock(&clist_mutex);
  struct clistwidsclient *first = c;

  do {
    if ((c->status == desired_status) && ((c->bssid->channel == desired_channel) || (desired_channel = -1))) {
      pthread_mutex_unlock(&clist_mutex);
      return c;
    }
    c = c->next;
  } while (c != first);
  
  pthread_mutex_unlock(&clist_mutex);
  return NULL;
}

struct clist *search_data(struct clist *c, unsigned char *desired_data, int data_len)
{
  if (!c) return NULL;
  
  pthread_mutex_lock(&clist_mutex);
  struct clist *first = c;

  do {
    if (data_len <= c->data_len) {
      if (! (memcmp(c->data, desired_data, data_len))) {
	pthread_mutex_unlock(&clist_mutex);
	return c;
      }
    }
    c = c->next;
  } while (c != first);

  pthread_mutex_unlock(&clist_mutex);
  return NULL;
}

struct clistwidsap *search_bssid(struct clistwidsap *c, struct ether_addr desired_bssid)
{
  if (!c) return NULL;
  
  pthread_mutex_lock(&clist_mutex);
  struct clistwidsap *first = c;

  do {
    if (MAC_MATCHES(c->bssid, desired_bssid)) {
      pthread_mutex_unlock(&clist_mutex);
      return c;
    }
    c = c->next;
  } while (c != first);

  pthread_mutex_unlock(&clist_mutex);
  return NULL;
}

struct clistwidsclient *search_client(struct clistwidsclient *c, struct ether_addr mac)
{
  if (!c) return NULL;
  
  pthread_mutex_lock(&clist_mutex);
  struct clistwidsclient *first = c;

  do {
    if (MAC_MATCHES(c->mac, mac)) {
      pthread_mutex_unlock(&clist_mutex);
      return c;
    }
    c = c->next;
  } while (c != first);

  pthread_mutex_unlock(&clist_mutex);
  return NULL;
}

struct clistauthdos *search_ap(struct clistauthdos *c, struct ether_addr ap)
{
  if (!c) return NULL;
  
  pthread_mutex_lock(&clist_mutex);
  struct clistauthdos *first = c;

  do {
    if (MAC_MATCHES(c->ap, ap)) {
      pthread_mutex_unlock(&clist_mutex);
      return c;
    }
    c = c->next;
  } while (c != first);

  pthread_mutex_unlock(&clist_mutex);
  return NULL;
}

struct clist *add_to_clist(struct clist *c, unsigned char *data, int status, int data_len)
{
  pthread_mutex_lock(&clist_mutex);
  
  struct clist *new_item = (struct clist *) malloc(sizeof(struct clist));
  new_item->data = malloc(data_len);
  new_item->data_len = data_len;

  if (c) {
    new_item->next = c->next;
    c->next = new_item;
  } else {
    new_item->next = new_item;
  }
  
  memcpy(new_item->data, data, data_len);
  new_item->status = status;

  pthread_mutex_unlock(&clist_mutex);
  return new_item;
}

struct clistwidsap *add_to_clistwidsap(struct clistwidsap *c, struct ether_addr bssid, int channel, uint16_t capa, char *ssid)
{
  pthread_mutex_lock(&clist_mutex);
  
  struct clistwidsap *new_item = (struct clistwidsap *) malloc(sizeof(struct clistwidsap));
  new_item->bssid = bssid;

  if (c) {
    new_item->next = c->next;
    c->next = new_item;
  } else {
    new_item->next = new_item;
  }
  
  new_item->channel = channel;
  new_item->capa = capa;
  new_item->ssid = malloc(strlen(ssid) + 1);
  strcpy(new_item->ssid, ssid);

  pthread_mutex_unlock(&clist_mutex);
  return new_item;
}

struct clistwidsclient *add_to_clistwidsclient(struct clistwidsclient *c, struct ether_addr mac, int status, unsigned char *data, int data_len, uint16_t sequence, struct clistwidsap *bssid)
{
  pthread_mutex_lock(&clist_mutex);
  
  struct clistwidsclient *new_item = (struct clistwidsclient *) malloc(sizeof(struct clistwidsclient));
  new_item->mac = mac;
  new_item->data = malloc(data_len);

  if (c) {
    new_item->next = c->next;
    c->next = new_item;
  } else {
    new_item->next = new_item;
  }
  
  memcpy(new_item->data, data, data_len);
  new_item->status = status;
  new_item->data_len = data_len;
  new_item->bssid = bssid;
  new_item->retries = 0;
  new_item->seq = sequence;

  pthread_mutex_unlock(&clist_mutex);
  return new_item;
}

struct clistauthdos *add_to_clistauthdos(struct clistauthdos *c, struct ether_addr ap, unsigned char status, unsigned int responses, unsigned int missing)
{
  pthread_mutex_lock(&clist_mutex);
  
  struct clistauthdos *new_item = (struct clistauthdos *) malloc(sizeof(struct clistauthdos));
  new_item->ap = ap;

  if (c) {
    new_item->next = c->next;
    c->next = new_item;
  } else {
    new_item->next = new_item;
  }
  
  new_item->status = status;
  new_item->responses = responses;
  new_item->missing = missing;

  pthread_mutex_unlock(&clist_mutex);
  return new_item;
}

struct clistauthdos *search_authdos_status(struct clistauthdos *c, int desired_status) {
  if (!c) return NULL;
  
  pthread_mutex_lock(&clist_mutex);
  struct clistauthdos *first = c;

  do {
    if (c->status == desired_status) {
      pthread_mutex_unlock(&clist_mutex);
      return c;
    }
    c = c->next;
  } while (c != first);

  pthread_mutex_unlock(&clist_mutex);
  return NULL;
}

struct clistwidsap *search_bssid_on_channel(struct clistwidsap *c, int desired_channel) {
  if (!c) return NULL;

  pthread_mutex_lock(&clist_mutex);
  struct clistwidsap *first = c;

  do {
    if (desired_channel == c->channel) {
      pthread_mutex_unlock(&clist_mutex);
      return c;
    }
    c = c->next;
  } while (c != first);

  pthread_mutex_unlock(&clist_mutex);
  return NULL;
}

struct clistwidsap *shuffle_widsaps(struct clistwidsap *c) {
  int i, rnd = random() % SHUFFLE_DISTANCE;
  struct clistwidsap *r = c;

  for(i=0; i<rnd; i++) r = r->next;

  return r;
}

struct clistwidsclient *shuffle_widsclients(struct clistwidsclient *c) {
  int i, rnd = random() % SHUFFLE_DISTANCE;
  struct clistwidsclient *r = c;

  for(i=0; i<rnd; i++) r = r->next;

  return r;
}
