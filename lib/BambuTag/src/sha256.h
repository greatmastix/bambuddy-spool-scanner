// Minimal, dependency-free SHA-256 and HMAC-SHA256.
// Kept self-contained so the tag decoder builds and tests on the host as well
// as on the ESP32 (where it is small enough not to matter next to mbedTLS).
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace bambu {

constexpr size_t SHA256_DIGEST_LEN = 32;
constexpr size_t SHA256_BLOCK_LEN = 64;

class Sha256 {
 public:
  Sha256();
  void update(const uint8_t* data, size_t len);
  void finish(uint8_t out[SHA256_DIGEST_LEN]);

 private:
  void transform(const uint8_t block[SHA256_BLOCK_LEN]);

  uint32_t state_[8];
  uint64_t bitLen_;
  uint8_t buffer_[SHA256_BLOCK_LEN];
  size_t bufferLen_;
};

void hmacSha256(const uint8_t* key, size_t keyLen, const uint8_t* msg, size_t msgLen,
                uint8_t out[SHA256_DIGEST_LEN]);

// RFC 5869 HKDF (extract + expand) with SHA-256.
// Returns false if outLen exceeds 255 * 32 bytes.
bool hkdfSha256(const uint8_t* ikm, size_t ikmLen, const uint8_t* salt, size_t saltLen,
                const uint8_t* info, size_t infoLen, uint8_t* out, size_t outLen);

}  // namespace bambu
