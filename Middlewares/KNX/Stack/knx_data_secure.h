/**
 * @file    knx_data_secure.h
 * @brief   KNX Data Secure protocol: encryption, authentication, key management.
 */

#ifndef KNX_DATA_SECURE_H
#define KNX_DATA_SECURE_H

#include <stdint.h>
#include <stdbool.h>

#define KNX_SECURE_KEY_LEN      16U
#define KNX_SECURE_NONCE_LEN    12U
#define KNX_SECURE_MAC_LEN      4U

bool knx_secure_init(const uint8_t *tool_key, const uint8_t *device_key);

bool knx_secure_encrypt_apdu(const uint8_t *plaintext, uint8_t plain_len,
                             uint8_t *ciphertext, uint8_t *cipher_len);

bool knx_secure_decrypt_apdu(const uint8_t *ciphertext, uint8_t cipher_len,
                             uint8_t *plaintext, uint8_t *plain_len);

bool knx_secure_verify_mac(const uint8_t *data, uint8_t len,
                           const uint8_t *mac);

#endif
