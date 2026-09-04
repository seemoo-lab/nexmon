#ifndef SHA256
#define SHA256

#include "iw.h"
#define SHA256_BLOCK_SIZE 64

#define LOAD32B(addr) \
		((__u32)((addr)[0] << 24) | ((addr)[1] << 16) | \
		((addr)[2] << 8) | (addr)[3])

#define STORE64B(addr, data) \
do { (addr)[0] = (__u8)((data) >> 56); (addr)[1] = (__u8)((data) >> 48);	\
	 (addr)[2] = (__u8)((data) >> 40); (addr)[3] = (__u8)((data) >> 32);	\
	 (addr)[4] = (__u8)((data) >> 24); (addr)[5] = (__u8)((data) >> 16);	\
	 (addr)[6] = (__u8)((data) >> 8);  (addr)[7] = (__u8)((data) & 0xff);	\
	} while (0)

#define STORE32B(addr, data) \
do { (addr)[0] = (__u8)(((data) >> 24) & 0xff); \
	 (addr)[1] = (__u8)(((data) >> 16) & 0xff); \
	 (addr)[2] = (__u8)(((data) >> 8) & 0xff); \
	 (addr)[3] = (__u8)((data) & 0xff); } while (0)

struct sha256_state {
	__u64 length;
	__u32 state[8], curlen;
	__u8 buf[SHA256_BLOCK_SIZE];
};

/**
 * SHA256 Hashing
 * @addr: pointers to the data area
 * @len: Lengths of the data block
 * @res: Buffer for the digest
 * Returns: 0 on success, -1 of failure
 */
int sha256(const unsigned char *addr, const size_t len,
		   unsigned char *res);

/* Initialize the hash state */
void sha256_init(struct sha256_state *md);

/**
 * Process a block of memory though the hash
 * @param md     The hash state
 * @param in     The data to hash
 * @param inlen  The length of the data (octets)
 * @return CRYPT_OK if successful
*/
int sha256_process(struct sha256_state *md, const unsigned char *in,
				   unsigned long inlen);

/**
 * Terminate the hash to get the digest
 * @param md  The hash state
 * @param out [out] The destination of the hash (32 bytes)
 * @return CRYPT_OK if successful
*/
int sha256_done(struct sha256_state *md, unsigned char *out);

#endif
