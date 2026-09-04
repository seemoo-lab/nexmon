#ifndef HAVE_LINKEDLIST_H
#define HAVE_LINKEDLIST_H

#include <stdint.h>

#include "mac_addr.h"

#define SHUFFLE_DISTANCE 16 //more shuffling => more cpu

struct clist
{
  unsigned char *data;
  int data_len;
  int status;
  struct clist *next;
};

struct clistwidsap
{
  struct ether_addr bssid;
  int channel;
  uint16_t capa;
  char *ssid;
  struct clistwidsap *next;
};

struct clistwidsclient
{
  struct ether_addr mac;
  char status; //0=ready 1=authed 2=assoced
  struct clistwidsclient *next;
  unsigned char *data;
  int data_len;
  int retries;
  uint16_t seq;
  struct clistwidsap *bssid;
};

struct clistauthdos
{
  struct ether_addr ap;
  unsigned char status;
  unsigned int responses;
  unsigned int missing;
  struct clistauthdos *next;
};

//All these calls are thread-safe via a single pthread_mutex!

struct clistauthdos *add_to_clistauthdos(struct clistauthdos *c, struct ether_addr ap, unsigned char status, unsigned int responses, unsigned int missing);
struct clistauthdos *search_ap(struct clistauthdos *c, struct ether_addr ap);
struct clistauthdos *search_authdos_status(struct clistauthdos *c, int desired_status);

struct clist *add_to_clist(struct clist *c, unsigned char *data, int status, int data_len);
struct clist *search_status(struct clist *c, int desired_status);
struct clist *search_data(struct clist *c, unsigned char *desired_data, int data_len);

struct clistwidsap *add_to_clistwidsap(struct clistwidsap *c, struct ether_addr bssid, int channel, uint16_t capa, char *ssid);
struct clistwidsap *search_bssid(struct clistwidsap *c, struct ether_addr desired_bssid);
struct clistwidsap *search_bssid_on_channel(struct clistwidsap *c, int desired_channel);
struct clistwidsap *shuffle_widsaps(struct clistwidsap *c);
struct clistwidsclient *add_to_clistwidsclient(struct clistwidsclient *c, struct ether_addr mac, int status, unsigned char *data, int data_len, uint16_t sequence, struct clistwidsap *bssid);
struct clistwidsclient *search_status_widsclient(struct clistwidsclient *c, int desired_status, int desired_channel);
struct clistwidsclient *search_client(struct clistwidsclient *c, struct ether_addr mac);
struct clistwidsclient *shuffle_widsclients(struct clistwidsclient *c);

#endif
