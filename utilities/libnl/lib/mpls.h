#ifndef MPLS_H_
#define MPLS_H_

extern const char *mpls_ntop(int af, const void *addr, char *buf, size_t buflen);
extern int mpls_pton(int af, const char *src, void *addr, size_t alen);

#endif
