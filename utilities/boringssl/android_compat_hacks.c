/* Copyright (c) 2014, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

/* This file contains odd functions that are needed, temporarily, on Android
 * because wpa_supplicant will take a little while to sync with upstream.
 *
 * The declarations for these functions are in the main BoringSSL headers but
 * the only definitions are here, in Android land. */

#include <openssl/ssl.h>

#include <stdlib.h>


int SSL_set_session_ticket_ext(SSL *s, void *ext_data, int ext_len) {
  return 0;
}

int SSL_set_session_ticket_ext_cb(SSL *s, void *cb, void *arg) {
  return 0;
}

int SSL_set_session_secret_cb(SSL *s, void *cb, void *arg) {
  return 0;
}

int SSL_set_ssl_method(SSL *s, const SSL_METHOD *method) {
  /* This is only called when EAP-FAST is being used, which is not supported. */
  abort();
}
