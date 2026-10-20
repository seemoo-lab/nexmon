/* Output of mkstrtable.awk.  DO NOT EDIT.  */

/* err-codes.h - List of error codes and their description.
   Copyright (C) 2003, 2004 g10 Code GmbH

   This file is part of libgpg-error.

   libgpg-error is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   libgpg-error is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with libgpg-error; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */


/* The purpose of this complex string table is to produce
   optimal code with a minimum of relocations.  */

static const char msgstr[] = 
  gettext_noop ("Success") "\0"
  gettext_noop ("General error") "\0"
  gettext_noop ("Unknown packet") "\0"
  gettext_noop ("Unknown version in packet") "\0"
  gettext_noop ("Invalid public key algorithm") "\0"
  gettext_noop ("Invalid digest algorithm") "\0"
  gettext_noop ("Bad public key") "\0"
  gettext_noop ("Bad secret key") "\0"
  gettext_noop ("Bad signature") "\0"
  gettext_noop ("No public key") "\0"
  gettext_noop ("Checksum error") "\0"
  gettext_noop ("Bad passphrase") "\0"
  gettext_noop ("Invalid cipher algorithm") "\0"
  gettext_noop ("Cannot open keyring") "\0"
  gettext_noop ("Invalid packet") "\0"
  gettext_noop ("Invalid armor") "\0"
  gettext_noop ("No user ID") "\0"
  gettext_noop ("No secret key") "\0"
  gettext_noop ("Wrong secret key used") "\0"
  gettext_noop ("Bad session key") "\0"
  gettext_noop ("Unknown compression algorithm") "\0"
  gettext_noop ("Number is not prime") "\0"
  gettext_noop ("Invalid encoding method") "\0"
  gettext_noop ("Invalid encryption scheme") "\0"
  gettext_noop ("Invalid signature scheme") "\0"
  gettext_noop ("Invalid attribute") "\0"
  gettext_noop ("No value") "\0"
  gettext_noop ("Not found") "\0"
  gettext_noop ("Value not found") "\0"
  gettext_noop ("Syntax error") "\0"
  gettext_noop ("Bad MPI value") "\0"
  gettext_noop ("Invalid passphrase") "\0"
  gettext_noop ("Invalid signature class") "\0"
  gettext_noop ("Resources exhausted") "\0"
  gettext_noop ("Invalid keyring") "\0"
  gettext_noop ("Trust DB error") "\0"
  gettext_noop ("Bad certificate") "\0"
  gettext_noop ("Invalid user ID") "\0"
  gettext_noop ("Unexpected error") "\0"
  gettext_noop ("Time conflict") "\0"
  gettext_noop ("Keyserver error") "\0"
  gettext_noop ("Wrong public key algorithm") "\0"
  gettext_noop ("Tribute to D. A.") "\0"
  gettext_noop ("Weak encryption key") "\0"
  gettext_noop ("Invalid key length") "\0"
  gettext_noop ("Invalid argument") "\0"
  gettext_noop ("Syntax error in URI") "\0"
  gettext_noop ("Invalid URI") "\0"
  gettext_noop ("Network error") "\0"
  gettext_noop ("Unknown host") "\0"
  gettext_noop ("Selftest failed") "\0"
  gettext_noop ("Data not encrypted") "\0"
  gettext_noop ("Data not processed") "\0"
  gettext_noop ("Unusable public key") "\0"
  gettext_noop ("Unusable secret key") "\0"
  gettext_noop ("Invalid value") "\0"
  gettext_noop ("Bad certificate chain") "\0"
  gettext_noop ("Missing certificate") "\0"
  gettext_noop ("No data") "\0"
  gettext_noop ("Bug") "\0"
  gettext_noop ("Not supported") "\0"
  gettext_noop ("Invalid operation code") "\0"
  gettext_noop ("Timeout") "\0"
  gettext_noop ("Internal error") "\0"
  gettext_noop ("EOF (gcrypt)") "\0"
  gettext_noop ("Invalid object") "\0"
  gettext_noop ("Provided object is too short") "\0"
  gettext_noop ("Provided object is too large") "\0"
  gettext_noop ("Missing item in object") "\0"
  gettext_noop ("Not implemented") "\0"
  gettext_noop ("Conflicting use") "\0"
  gettext_noop ("Invalid cipher mode") "\0"
  gettext_noop ("Invalid flag") "\0"
  gettext_noop ("Invalid handle") "\0"
  gettext_noop ("Result truncated") "\0"
  gettext_noop ("Incomplete line") "\0"
  gettext_noop ("Invalid response") "\0"
  gettext_noop ("No agent running") "\0"
  gettext_noop ("Agent error") "\0"
  gettext_noop ("Invalid data") "\0"
  gettext_noop ("Unspecific Assuan server fault") "\0"
  gettext_noop ("General Assuan error") "\0"
  gettext_noop ("Invalid session key") "\0"
  gettext_noop ("Invalid S-expression") "\0"
  gettext_noop ("Unsupported algorithm") "\0"
  gettext_noop ("No pinentry") "\0"
  gettext_noop ("pinentry error") "\0"
  gettext_noop ("Bad PIN") "\0"
  gettext_noop ("Invalid name") "\0"
  gettext_noop ("Bad data") "\0"
  gettext_noop ("Invalid parameter") "\0"
  gettext_noop ("Wrong card") "\0"
  gettext_noop ("No dirmngr") "\0"
  gettext_noop ("dirmngr error") "\0"
  gettext_noop ("Certificate revoked") "\0"
  gettext_noop ("No CRL known") "\0"
  gettext_noop ("CRL too old") "\0"
  gettext_noop ("Line too long") "\0"
  gettext_noop ("Not trusted") "\0"
  gettext_noop ("Operation cancelled") "\0"
  gettext_noop ("Bad CA certificate") "\0"
  gettext_noop ("Certificate expired") "\0"
  gettext_noop ("Certificate too young") "\0"
  gettext_noop ("Unsupported certificate") "\0"
  gettext_noop ("Unknown S-expression") "\0"
  gettext_noop ("Unsupported protection") "\0"
  gettext_noop ("Corrupted protection") "\0"
  gettext_noop ("Ambiguous name") "\0"
  gettext_noop ("Card error") "\0"
  gettext_noop ("Card reset required") "\0"
  gettext_noop ("Card removed") "\0"
  gettext_noop ("Invalid card") "\0"
  gettext_noop ("Card not present") "\0"
  gettext_noop ("No PKCS15 application") "\0"
  gettext_noop ("Not confirmed") "\0"
  gettext_noop ("Configuration error") "\0"
  gettext_noop ("No policy match") "\0"
  gettext_noop ("Invalid index") "\0"
  gettext_noop ("Invalid ID") "\0"
  gettext_noop ("No SmartCard daemon") "\0"
  gettext_noop ("SmartCard daemon error") "\0"
  gettext_noop ("Unsupported protocol") "\0"
  gettext_noop ("Bad PIN method") "\0"
  gettext_noop ("Card not initialized") "\0"
  gettext_noop ("Unsupported operation") "\0"
  gettext_noop ("Wrong key usage") "\0"
  gettext_noop ("Nothing found") "\0"
  gettext_noop ("Wrong blob type") "\0"
  gettext_noop ("Missing value") "\0"
  gettext_noop ("Hardware problem") "\0"
  gettext_noop ("PIN blocked") "\0"
  gettext_noop ("Conditions of use not satisfied") "\0"
  gettext_noop ("PINs are not synced") "\0"
  gettext_noop ("Invalid CRL") "\0"
  gettext_noop ("BER error") "\0"
  gettext_noop ("Invalid BER") "\0"
  gettext_noop ("Element not found") "\0"
  gettext_noop ("Identifier not found") "\0"
  gettext_noop ("Invalid tag") "\0"
  gettext_noop ("Invalid length") "\0"
  gettext_noop ("Invalid key info") "\0"
  gettext_noop ("Unexpected tag") "\0"
  gettext_noop ("Not DER encoded") "\0"
  gettext_noop ("No CMS object") "\0"
  gettext_noop ("Invalid CMS object") "\0"
  gettext_noop ("Unknown CMS object") "\0"
  gettext_noop ("Unsupported CMS object") "\0"
  gettext_noop ("Unsupported encoding") "\0"
  gettext_noop ("Unsupported CMS version") "\0"
  gettext_noop ("Unknown algorithm") "\0"
  gettext_noop ("Invalid crypto engine") "\0"
  gettext_noop ("Public key not trusted") "\0"
  gettext_noop ("Decryption failed") "\0"
  gettext_noop ("Key expired") "\0"
  gettext_noop ("Signature expired") "\0"
  gettext_noop ("Encoding problem") "\0"
  gettext_noop ("Invalid state") "\0"
  gettext_noop ("Duplicated value") "\0"
  gettext_noop ("Missing action") "\0"
  gettext_noop ("ASN.1 module not found") "\0"
  gettext_noop ("Invalid OID string") "\0"
  gettext_noop ("Invalid time") "\0"
  gettext_noop ("Invalid CRL object") "\0"
  gettext_noop ("Unsupported CRL version") "\0"
  gettext_noop ("Invalid certificate object") "\0"
  gettext_noop ("Unknown name") "\0"
  gettext_noop ("A locale function failed") "\0"
  gettext_noop ("Not locked") "\0"
  gettext_noop ("Protocol violation") "\0"
  gettext_noop ("Invalid MAC") "\0"
  gettext_noop ("Invalid request") "\0"
  gettext_noop ("Unknown extension") "\0"
  gettext_noop ("Unknown critical extension") "\0"
  gettext_noop ("Locked") "\0"
  gettext_noop ("Unknown option") "\0"
  gettext_noop ("Unknown command") "\0"
  gettext_noop ("Not operational") "\0"
  gettext_noop ("No passphrase given") "\0"
  gettext_noop ("No PIN given") "\0"
  gettext_noop ("Not enabled") "\0"
  gettext_noop ("No crypto engine") "\0"
  gettext_noop ("Missing key") "\0"
  gettext_noop ("Too many objects") "\0"
  gettext_noop ("Limit reached") "\0"
  gettext_noop ("Not initialized") "\0"
  gettext_noop ("Missing issuer certificate") "\0"
  gettext_noop ("No keyserver available") "\0"
  gettext_noop ("Invalid elliptic curve") "\0"
  gettext_noop ("Unknown elliptic curve") "\0"
  gettext_noop ("Duplicated key") "\0"
  gettext_noop ("Ambiguous result") "\0"
  gettext_noop ("No crypto context") "\0"
  gettext_noop ("Wrong crypto context") "\0"
  gettext_noop ("Bad crypto context") "\0"
  gettext_noop ("Conflict in the crypto context") "\0"
  gettext_noop ("Broken public key") "\0"
  gettext_noop ("Broken secret key") "\0"
  gettext_noop ("Invalid MAC algorithm") "\0"
  gettext_noop ("Operation fully cancelled") "\0"
  gettext_noop ("Operation not yet finished") "\0"
  gettext_noop ("Buffer too short") "\0"
  gettext_noop ("Invalid length specifier in S-expression") "\0"
  gettext_noop ("String too long in S-expression") "\0"
  gettext_noop ("Unmatched parentheses in S-expression") "\0"
  gettext_noop ("S-expression not canonical") "\0"
  gettext_noop ("Bad character in S-expression") "\0"
  gettext_noop ("Bad quotation in S-expression") "\0"
  gettext_noop ("Zero prefix in S-expression") "\0"
  gettext_noop ("Nested display hints in S-expression") "\0"
  gettext_noop ("Unmatched display hints") "\0"
  gettext_noop ("Unexpected reserved punctuation in S-expression") "\0"
  gettext_noop ("Bad hexadecimal character in S-expression") "\0"
  gettext_noop ("Odd hexadecimal numbers in S-expression") "\0"
  gettext_noop ("Bad octal character in S-expression") "\0"
  gettext_noop ("Non-compliant public key algorithm") "\0"
  gettext_noop ("Non-compliant cipher algorithm") "\0"
  gettext_noop ("Unexpected packet") "\0"
  gettext_noop ("All subkeys are expired or revoked") "\0"
  gettext_noop ("Database is corrupted") "\0"
  gettext_noop ("Server indicated a failure") "\0"
  gettext_noop ("No name") "\0"
  gettext_noop ("No key") "\0"
  gettext_noop ("Legacy key") "\0"
  gettext_noop ("Request too short") "\0"
  gettext_noop ("Request too long") "\0"
  gettext_noop ("Object is in termination state") "\0"
  gettext_noop ("No certificate chain") "\0"
  gettext_noop ("Certificate is too large") "\0"
  gettext_noop ("Invalid record") "\0"
  gettext_noop ("The MAC does not verify") "\0"
  gettext_noop ("Unexpected message") "\0"
  gettext_noop ("Compression or decompression failed") "\0"
  gettext_noop ("A counter would wrap") "\0"
  gettext_noop ("Fatal alert message received") "\0"
  gettext_noop ("No cipher algorithm") "\0"
  gettext_noop ("Missing client certificate") "\0"
  gettext_noop ("Close notification received") "\0"
  gettext_noop ("Ticket expired") "\0"
  gettext_noop ("Bad ticket") "\0"
  gettext_noop ("Unknown identity") "\0"
  gettext_noop ("Bad certificate message in handshake") "\0"
  gettext_noop ("Bad certificate request message in handshake") "\0"
  gettext_noop ("Bad certificate verify message in handshake") "\0"
  gettext_noop ("Bad change cipher message in handshake") "\0"
  gettext_noop ("Bad client hello message in handshake") "\0"
  gettext_noop ("Bad server hello message in handshake") "\0"
  gettext_noop ("Bad server hello done message in handshake") "\0"
  gettext_noop ("Bad finished message in handshake") "\0"
  gettext_noop ("Bad server key exchange message in handshake") "\0"
  gettext_noop ("Bad client key exchange message in handshake") "\0"
  gettext_noop ("Bogus string") "\0"
  gettext_noop ("Forbidden") "\0"
  gettext_noop ("Key disabled") "\0"
  gettext_noop ("Not possible with a card based key") "\0"
  gettext_noop ("Invalid lock object") "\0"
  gettext_noop ("True") "\0"
  gettext_noop ("False") "\0"
  gettext_noop ("General IPC error") "\0"
  gettext_noop ("IPC accept call failed") "\0"
  gettext_noop ("IPC connect call failed") "\0"
  gettext_noop ("Invalid IPC response") "\0"
  gettext_noop ("Invalid value passed to IPC") "\0"
  gettext_noop ("Incomplete line passed to IPC") "\0"
  gettext_noop ("Line passed to IPC too long") "\0"
  gettext_noop ("Nested IPC commands") "\0"
  gettext_noop ("No data callback in IPC") "\0"
  gettext_noop ("No inquire callback in IPC") "\0"
  gettext_noop ("Not an IPC server") "\0"
  gettext_noop ("Not an IPC client") "\0"
  gettext_noop ("Problem starting IPC server") "\0"
  gettext_noop ("IPC read error") "\0"
  gettext_noop ("IPC write error") "\0"
  gettext_noop ("Too much data for IPC layer") "\0"
  gettext_noop ("Unexpected IPC command") "\0"
  gettext_noop ("Unknown IPC command") "\0"
  gettext_noop ("IPC syntax error") "\0"
  gettext_noop ("IPC call has been cancelled") "\0"
  gettext_noop ("No input source for IPC") "\0"
  gettext_noop ("No output source for IPC") "\0"
  gettext_noop ("IPC parameter error") "\0"
  gettext_noop ("Unknown IPC inquire") "\0"
  gettext_noop ("Crypto engine too old") "\0"
  gettext_noop ("Screen or window too small") "\0"
  gettext_noop ("Screen or window too large") "\0"
  gettext_noop ("Required environment variable not set") "\0"
  gettext_noop ("User ID already exists") "\0"
  gettext_noop ("Name already exists") "\0"
  gettext_noop ("Duplicated name") "\0"
  gettext_noop ("Object is too young") "\0"
  gettext_noop ("Object is too old") "\0"
  gettext_noop ("Unknown flag") "\0"
  gettext_noop ("Invalid execution order") "\0"
  gettext_noop ("Already fetched") "\0"
  gettext_noop ("Try again later") "\0"
  gettext_noop ("Wrong name") "\0"
  gettext_noop ("Not authenticated") "\0"
  gettext_noop ("Bad authentication") "\0"
  gettext_noop ("No Keybox daemon running") "\0"
  gettext_noop ("Keybox daemon error") "\0"
  gettext_noop ("Service is not running") "\0"
  gettext_noop ("Service error") "\0"
  gettext_noop ("Bad PUK") "\0"
  gettext_noop ("No reset code") "\0"
  gettext_noop ("Bad reset code") "\0"
  gettext_noop ("Non-compliant digest algorithm") "\0"
  gettext_noop ("System bug detected") "\0"
  gettext_noop ("Unknown DNS error") "\0"
  gettext_noop ("Invalid DNS section") "\0"
  gettext_noop ("Invalid textual address form") "\0"
  gettext_noop ("Missing DNS query packet") "\0"
  gettext_noop ("Missing DNS answer packet") "\0"
  gettext_noop ("Connection closed in DNS") "\0"
  gettext_noop ("Verification failed in DNS") "\0"
  gettext_noop ("DNS Timeout") "\0"
  gettext_noop ("General LDAP error") "\0"
  gettext_noop ("General LDAP attribute error") "\0"
  gettext_noop ("General LDAP name error") "\0"
  gettext_noop ("General LDAP security error") "\0"
  gettext_noop ("General LDAP service error") "\0"
  gettext_noop ("General LDAP update error") "\0"
  gettext_noop ("Experimental LDAP error code") "\0"
  gettext_noop ("Private LDAP error code") "\0"
  gettext_noop ("Other general LDAP error") "\0"
  gettext_noop ("LDAP connecting failed (X)") "\0"
  gettext_noop ("LDAP referral limit exceeded") "\0"
  gettext_noop ("LDAP client loop") "\0"
  gettext_noop ("No LDAP results returned") "\0"
  gettext_noop ("LDAP control not found") "\0"
  gettext_noop ("Not supported by LDAP") "\0"
  gettext_noop ("LDAP connect error") "\0"
  gettext_noop ("Out of memory in LDAP") "\0"
  gettext_noop ("Bad parameter to an LDAP routine") "\0"
  gettext_noop ("User cancelled LDAP operation") "\0"
  gettext_noop ("Bad LDAP search filter") "\0"
  gettext_noop ("Unknown LDAP authentication method") "\0"
  gettext_noop ("Timeout in LDAP") "\0"
  gettext_noop ("LDAP decoding error") "\0"
  gettext_noop ("LDAP encoding error") "\0"
  gettext_noop ("LDAP local error") "\0"
  gettext_noop ("Cannot contact LDAP server") "\0"
  gettext_noop ("LDAP success") "\0"
  gettext_noop ("LDAP operations error") "\0"
  gettext_noop ("LDAP protocol error") "\0"
  gettext_noop ("Time limit exceeded in LDAP") "\0"
  gettext_noop ("Size limit exceeded in LDAP") "\0"
  gettext_noop ("LDAP compare false") "\0"
  gettext_noop ("LDAP compare true") "\0"
  gettext_noop ("LDAP authentication method not supported") "\0"
  gettext_noop ("Strong(er) LDAP authentication required") "\0"
  gettext_noop ("Partial LDAP results+referral received") "\0"
  gettext_noop ("LDAP referral") "\0"
  gettext_noop ("Administrative LDAP limit exceeded") "\0"
  gettext_noop ("Critical LDAP extension is unavailable") "\0"
  gettext_noop ("Confidentiality required by LDAP") "\0"
  gettext_noop ("LDAP SASL bind in progress") "\0"
  gettext_noop ("No such LDAP attribute") "\0"
  gettext_noop ("Undefined LDAP attribute type") "\0"
  gettext_noop ("Inappropriate matching in LDAP") "\0"
  gettext_noop ("Constraint violation in LDAP") "\0"
  gettext_noop ("LDAP type or value exists") "\0"
  gettext_noop ("Invalid syntax in LDAP") "\0"
  gettext_noop ("No such LDAP object") "\0"
  gettext_noop ("LDAP alias problem") "\0"
  gettext_noop ("Invalid DN syntax in LDAP") "\0"
  gettext_noop ("LDAP entry is a leaf") "\0"
  gettext_noop ("LDAP alias dereferencing problem") "\0"
  gettext_noop ("LDAP proxy authorization failure (X)") "\0"
  gettext_noop ("Inappropriate LDAP authentication") "\0"
  gettext_noop ("Invalid LDAP credentials") "\0"
  gettext_noop ("Insufficient access for LDAP") "\0"
  gettext_noop ("LDAP server is busy") "\0"
  gettext_noop ("LDAP server is unavailable") "\0"
  gettext_noop ("LDAP server is unwilling to perform") "\0"
  gettext_noop ("Loop detected by LDAP") "\0"
  gettext_noop ("LDAP naming violation") "\0"
  gettext_noop ("LDAP object class violation") "\0"
  gettext_noop ("LDAP operation not allowed on non-leaf") "\0"
  gettext_noop ("LDAP operation not allowed on RDN") "\0"
  gettext_noop ("Already exists (LDAP)") "\0"
  gettext_noop ("Cannot modify LDAP object class") "\0"
  gettext_noop ("LDAP results too large") "\0"
  gettext_noop ("LDAP operation affects multiple DSAs") "\0"
  gettext_noop ("Virtual LDAP list view error") "\0"
  gettext_noop ("Other LDAP error") "\0"
  gettext_noop ("Resources exhausted in LCUP") "\0"
  gettext_noop ("Security violation in LCUP") "\0"
  gettext_noop ("Invalid data in LCUP") "\0"
  gettext_noop ("Unsupported scheme in LCUP") "\0"
  gettext_noop ("Reload required in LCUP") "\0"
  gettext_noop ("LDAP cancelled") "\0"
  gettext_noop ("No LDAP operation to cancel") "\0"
  gettext_noop ("Too late to cancel LDAP") "\0"
  gettext_noop ("Cannot cancel LDAP") "\0"
  gettext_noop ("LDAP assertion failed") "\0"
  gettext_noop ("Proxied authorization denied by LDAP") "\0"
  gettext_noop ("User defined error code 1") "\0"
  gettext_noop ("User defined error code 2") "\0"
  gettext_noop ("User defined error code 3") "\0"
  gettext_noop ("User defined error code 4") "\0"
  gettext_noop ("User defined error code 5") "\0"
  gettext_noop ("User defined error code 6") "\0"
  gettext_noop ("User defined error code 7") "\0"
  gettext_noop ("User defined error code 8") "\0"
  gettext_noop ("User defined error code 9") "\0"
  gettext_noop ("User defined error code 10") "\0"
  gettext_noop ("User defined error code 11") "\0"
  gettext_noop ("User defined error code 12") "\0"
  gettext_noop ("User defined error code 13") "\0"
  gettext_noop ("User defined error code 14") "\0"
  gettext_noop ("User defined error code 15") "\0"
  gettext_noop ("User defined error code 16") "\0"
  gettext_noop ("SQL success") "\0"
  gettext_noop ("SQL error") "\0"
  gettext_noop ("Internal logic error in SQL library") "\0"
  gettext_noop ("Access permission denied (SQL)") "\0"
  gettext_noop ("SQL abort was requested") "\0"
  gettext_noop ("SQL database file is locked") "\0"
  gettext_noop ("An SQL table in the database is locked") "\0"
  gettext_noop ("SQL library ran out of core") "\0"
  gettext_noop ("Attempt to write a readonly SQL database") "\0"
  gettext_noop ("SQL operation terminated by interrupt") "\0"
  gettext_noop ("I/O error during SQL operation") "\0"
  gettext_noop ("SQL database disk image is malformed") "\0"
  gettext_noop ("Unknown opcode in SQL file control") "\0"
  gettext_noop ("Insertion failed because SQL database is full") "\0"
  gettext_noop ("Unable to open the SQL database file") "\0"
  gettext_noop ("SQL database lock protocol error") "\0"
  gettext_noop ("(internal SQL code: empty)") "\0"
  gettext_noop ("SQL database schema changed") "\0"
  gettext_noop ("String or blob exceeds size limit (SQL)") "\0"
  gettext_noop ("SQL abort due to constraint violation") "\0"
  gettext_noop ("Data type mismatch (SQL)") "\0"
  gettext_noop ("SQL library used incorrectly") "\0"
  gettext_noop ("SQL library uses unsupported OS features") "\0"
  gettext_noop ("Authorization denied (SQL)") "\0"
  gettext_noop ("(unused SQL code: format)") "\0"
  gettext_noop ("SQL bind parameter out of range") "\0"
  gettext_noop ("File opened that is not an SQL database file") "\0"
  gettext_noop ("Notifications from SQL logger") "\0"
  gettext_noop ("Warnings from SQL logger") "\0"
  gettext_noop ("SQL has another row ready") "\0"
  gettext_noop ("SQL has finished executing") "\0"
  gettext_noop ("System error w/o errno") "\0"
  gettext_noop ("Unknown system error") "\0"
  gettext_noop ("End of file") "\0"
  gettext_noop ("Unknown error code");

static const int msgidx[] =
  {
    0,
    8,
    22,
    37,
    63,
    92,
    117,
    132,
    147,
    161,
    175,
    190,
    205,
    230,
    250,
    265,
    279,
    290,
    304,
    326,
    342,
    372,
    392,
    416,
    442,
    467,
    485,
    494,
    504,
    520,
    533,
    547,
    566,
    590,
    610,
    626,
    641,
    657,
    673,
    690,
    704,
    720,
    747,
    764,
    784,
    803,
    820,
    840,
    852,
    866,
    879,
    895,
    914,
    933,
    953,
    973,
    987,
    1009,
    1029,
    1037,
    1041,
    1055,
    1078,
    1086,
    1101,
    1114,
    1129,
    1158,
    1187,
    1210,
    1226,
    1242,
    1262,
    1275,
    1290,
    1307,
    1323,
    1340,
    1357,
    1369,
    1382,
    1413,
    1434,
    1454,
    1475,
    1497,
    1509,
    1524,
    1532,
    1545,
    1554,
    1572,
    1583,
    1594,
    1608,
    1628,
    1641,
    1653,
    1667,
    1679,
    1699,
    1718,
    1738,
    1760,
    1784,
    1805,
    1828,
    1849,
    1864,
    1875,
    1895,
    1908,
    1921,
    1938,
    1960,
    1974,
    1994,
    2010,
    2024,
    2035,
    2055,
    2078,
    2099,
    2114,
    2135,
    2157,
    2173,
    2187,
    2203,
    2217,
    2234,
    2246,
    2278,
    2298,
    2310,
    2320,
    2332,
    2350,
    2371,
    2383,
    2398,
    2415,
    2430,
    2446,
    2460,
    2479,
    2498,
    2521,
    2542,
    2566,
    2584,
    2606,
    2629,
    2647,
    2659,
    2677,
    2694,
    2708,
    2725,
    2740,
    2763,
    2782,
    2795,
    2814,
    2838,
    2865,
    2878,
    2903,
    2914,
    2933,
    2945,
    2961,
    2979,
    3006,
    3013,
    3028,
    3044,
    3060,
    3080,
    3093,
    3105,
    3122,
    3134,
    3151,
    3165,
    3181,
    3208,
    3231,
    3254,
    3277,
    3292,
    3309,
    3327,
    3348,
    3367,
    3398,
    3416,
    3434,
    3456,
    3482,
    3509,
    3526,
    3567,
    3599,
    3637,
    3664,
    3694,
    3724,
    3752,
    3789,
    3813,
    3861,
    3903,
    3943,
    3979,
    4014,
    4045,
    4063,
    4098,
    4120,
    4147,
    4155,
    4162,
    4173,
    4191,
    4208,
    4239,
    4260,
    4285,
    4300,
    4324,
    4343,
    4379,
    4400,
    4429,
    4449,
    4476,
    4504,
    4519,
    4530,
    4547,
    4584,
    4629,
    4673,
    4712,
    4750,
    4788,
    4831,
    4865,
    4910,
    4955,
    4968,
    4978,
    4991,
    5026,
    5046,
    5051,
    5057,
    5075,
    5098,
    5122,
    5143,
    5171,
    5201,
    5229,
    5249,
    5273,
    5300,
    5318,
    5336,
    5364,
    5379,
    5395,
    5423,
    5446,
    5466,
    5483,
    5511,
    5535,
    5560,
    5580,
    5600,
    5622,
    5649,
    5676,
    5714,
    5737,
    5757,
    5773,
    5793,
    5811,
    5824,
    5848,
    5864,
    5880,
    5891,
    5909,
    5928,
    5953,
    5973,
    5996,
    6010,
    6018,
    6032,
    6047,
    6078,
    6098,
    6116,
    6136,
    6165,
    6190,
    6216,
    6241,
    6268,
    6280,
    6299,
    6328,
    6352,
    6380,
    6407,
    6433,
    6462,
    6486,
    6511,
    6538,
    6567,
    6584,
    6609,
    6632,
    6654,
    6673,
    6695,
    6728,
    6758,
    6781,
    6816,
    6832,
    6852,
    6872,
    6889,
    6916,
    6929,
    6951,
    6971,
    6999,
    7027,
    7046,
    7064,
    7105,
    7145,
    7184,
    7198,
    7233,
    7272,
    7305,
    7332,
    7355,
    7385,
    7416,
    7445,
    7471,
    7494,
    7514,
    7533,
    7559,
    7580,
    7613,
    7650,
    7684,
    7709,
    7738,
    7758,
    7785,
    7821,
    7843,
    7865,
    7893,
    7932,
    7966,
    7988,
    8020,
    8043,
    8080,
    8109,
    8126,
    8154,
    8181,
    8202,
    8229,
    8253,
    8268,
    8296,
    8320,
    8339,
    8361,
    8398,
    8424,
    8450,
    8476,
    8502,
    8528,
    8554,
    8580,
    8606,
    8632,
    8659,
    8686,
    8713,
    8740,
    8767,
    8794,
    8821,
    8833,
    8843,
    8879,
    8910,
    8934,
    8962,
    9001,
    9029,
    9070,
    9108,
    9139,
    9176,
    9211,
    9257,
    9294,
    9327,
    9354,
    9382,
    9422,
    9460,
    9485,
    9514,
    9555,
    9582,
    9608,
    9640,
    9685,
    9715,
    9740,
    9766,
    9793,
    9816,
    9837,
    9849
  };

static GPG_ERR_INLINE int
msgidxof (int code)
{
  return (0 ? 0
  : ((code >= 0) && (code <= 271)) ? (code - 0)
  : ((code >= 273) && (code <= 281)) ? (code - 1)
  : ((code >= 300) && (code <= 323)) ? (code - 19)
  : ((code >= 666) && (code <= 666)) ? (code - 361)
  : ((code >= 711) && (code <= 718)) ? (code - 405)
  : ((code >= 721) && (code <= 729)) ? (code - 407)
  : ((code >= 750) && (code <= 752)) ? (code - 427)
  : ((code >= 754) && (code <= 782)) ? (code - 428)
  : ((code >= 784) && (code <= 789)) ? (code - 429)
  : ((code >= 800) && (code <= 804)) ? (code - 439)
  : ((code >= 815) && (code <= 822)) ? (code - 449)
  : ((code >= 832) && (code <= 839)) ? (code - 458)
  : ((code >= 844) && (code <= 844)) ? (code - 462)
  : ((code >= 848) && (code <= 848)) ? (code - 465)
  : ((code >= 881) && (code <= 891)) ? (code - 497)
  : ((code >= 1024) && (code <= 1039)) ? (code - 629)
  : ((code >= 1500) && (code <= 1528)) ? (code - 1089)
  : ((code >= 1600) && (code <= 1601)) ? (code - 1160)
  : ((code >= 16381) && (code <= 16383)) ? (code - 15939)
  : 16384 - 15939);
}
