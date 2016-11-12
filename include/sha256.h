#ifndef SHA256_H
#define SHA256_H

#ifdef __cplusplus
extern "C" {
#endif

  // use old api
/**
 * \brief          Output = SHA-256( input buffer )
 *
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 * \param output   SHA-224/256 checksum result
 * \param is224    0 = use SHA256, 1 = use SHA224
 */
void sha256( const unsigned char *input, size_t ilen,
           unsigned char output[32], int is224 );

/**
 * \brief          Output = HMAC-SHA-256( hmac key, input buffer )
 *
 * \param key      HMAC secret key
 * \param keylen   length of the HMAC key
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 * \param output   HMAC-SHA-224/256 result
 * \param is224    0 = use SHA256, 1 = use SHA224
 */
void sha256_hmac( const unsigned char *key, size_t keylen,
                  const unsigned char *input, size_t ilen,
                  unsigned char output[32], int is224 );

void hmac_256(const unsigned char *key, size_t keylen,
                  const unsigned char *input, size_t ilen,
                  unsigned char output[32]);

// provide these for direct access as a lib
typedef struct SHA256Context {
  uint32_t state[8];
  uint32_t count[2];
  unsigned char buf[64];
} SHA256_CTX;

void SHA256_Final(unsigned char digest[32], SHA256_CTX * ctx);
void SHA256_Update(SHA256_CTX * ctx, const void *in, size_t len);
void SHA256_Init(SHA256_CTX * ctx);

int hkdf_sha256( uint8_t *salt, uint32_t salt_len, uint8_t *ikm, uint32_t ikm_len, uint8_t *info, uint32_t info_len, uint8_t *okm, uint32_t okm_len);

#ifdef __cplusplus
}
#endif

#endif /* sha256.h */
