/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.dumpkey;

import org.bouncycastle.jce.provider.BouncyCastleProvider;

import java.io.FileInputStream;
import java.math.BigInteger;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.security.KeyStore;
import java.security.Key;
import java.security.PublicKey;
import java.security.Security;
import java.security.interfaces.ECPublicKey;
import java.security.interfaces.RSAPublicKey;
import java.security.spec.ECPoint;

/**
 * Command line tool to extract RSA public keys from X.509 certificates
 * and output source code with data initializers for the keys.
 * @hide
 */
class DumpPublicKey {
    /**
     * @param key to perform sanity checks on
     * @return version number of key.  Supported versions are:
     *     1: 2048-bit RSA key with e=3 and SHA-1 hash
     *     2: 2048-bit RSA key with e=65537 and SHA-1 hash
     *     3: 2048-bit RSA key with e=3 and SHA-256 hash
     *     4: 2048-bit RSA key with e=65537 and SHA-256 hash
     * @throws Exception if the key has the wrong size or public exponent
     */
    static int checkRSA(RSAPublicKey key, boolean useSHA256) throws Exception {
        BigInteger pubexp = key.getPublicExponent();
        BigInteger modulus = key.getModulus();
        int version;

        if (pubexp.equals(BigInteger.valueOf(3))) {
            version = useSHA256 ? 3 : 1;
        } else if (pubexp.equals(BigInteger.valueOf(65537))) {
            version = useSHA256 ? 4 : 2;
        } else {
            throw new Exception("Public exponent should be 3 or 65537 but is " +
                                pubexp.toString(10) + ".");
        }

        if (modulus.bitLength() != 2048) {
             throw new Exception("Modulus should be 2048 bits long but is " +
                        modulus.bitLength() + " bits.");
        }

        return version;
    }

    /**
     * @param key to perform sanity checks on
     * @return version number of key.  Supported versions are:
     *     5: 256-bit EC key with curve NIST P-256
     * @throws Exception if the key has the wrong size or public exponent
     */
    static int checkEC(ECPublicKey key) throws Exception {
        if (key.getParams().getCurve().getField().getFieldSize() != 256) {
            throw new Exception("Curve must be NIST P-256");
        }

        return 5;
    }

    /**
     * Perform sanity check on public key.
     */
    static int check(PublicKey key, boolean useSHA256) throws Exception {
        if (key instanceof RSAPublicKey) {
            return checkRSA((RSAPublicKey) key, useSHA256);
        } else if (key instanceof ECPublicKey) {
            if (!useSHA256) {
                throw new Exception("Must use SHA-256 with EC keys!");
            }
            return checkEC((ECPublicKey) key);
        } else {
            throw new Exception("Unsupported key class: " + key.getClass().getName());
        }
    }

    /**
     * @param key to output
     * @return a String representing this public key.  If the key is a
     *    version 1 key, the string will be a C initializer; this is
     *    not true for newer key versions.
     */
    static String printRSA(RSAPublicKey key, boolean useSHA256) throws Exception {
        int version = check(key, useSHA256);

        BigInteger N = key.getModulus();

        StringBuilder result = new StringBuilder();

        int nwords = N.bitLength() / 32;    // # of 32 bit integers in modulus

        if (version > 1) {
            result.append("v");
            result.append(Integer.toString(version));
            result.append(" ");
        }

        result.append("{");
        result.append(nwords);

        BigInteger B = BigInteger.valueOf(0x100000000L);  // 2^32
        BigInteger N0inv = B.subtract(N.modInverse(B));   // -1 / N[0] mod 2^32

        result.append(",0x");
        result.append(N0inv.toString(16));

        BigInteger R = BigInteger.valueOf(2).pow(N.bitLength());
        BigInteger RR = R.multiply(R).mod(N);    // 2^4096 mod N

        // Write out modulus as little endian array of integers.
        result.append(",{");
        for (int i = 0; i < nwords; ++i) {
            long n = N.mod(B).longValue();
            result.append(n);

            if (i != nwords - 1) {
                result.append(",");
            }

            N = N.divide(B);
        }
        result.append("}");

        // Write R^2 as little endian array of integers.
        result.append(",{");
        for (int i = 0; i < nwords; ++i) {
            long rr = RR.mod(B).longValue();
            result.append(rr);

            if (i != nwords - 1) {
                result.append(",");
            }

            RR = RR.divide(B);
        }
        result.append("}");

        result.append("}");
        return result.toString();
    }

    /**
     * @param key to output
     * @return a String representing this public key.  If the key is a
     *    version 1 key, the string will be a C initializer; this is
     *    not true for newer key versions.
     */
    static String printEC(ECPublicKey key) throws Exception {
        int version = checkEC(key);

        StringBuilder result = new StringBuilder();

        result.append("v");
        result.append(Integer.toString(version));
        result.append(" ");

        BigInteger X = key.getW().getAffineX();
        BigInteger Y = key.getW().getAffineY();
        int nbytes = key.getParams().getCurve().getField().getFieldSize() / 8;    // # of 32 bit integers in X coordinate

        result.append("{");
        result.append(nbytes);

        BigInteger B = BigInteger.valueOf(0x100L);  // 2^8

        // Write out Y coordinate as array of characters.
        result.append(",{");
        for (int i = 0; i < nbytes; ++i) {
            long n = X.mod(B).longValue();
            result.append(n);

            if (i != nbytes - 1) {
                result.append(",");
            }

            X = X.divide(B);
        }
        result.append("}");

        // Write out Y coordinate as array of characters.
        result.append(",{");
        for (int i = 0; i < nbytes; ++i) {
            long n = Y.mod(B).longValue();
            result.append(n);

            if (i != nbytes - 1) {
                result.append(",");
            }

            Y = Y.divide(B);
        }
        result.append("}");

        result.append("}");
        return result.toString();
    }

    static String print(PublicKey key, boolean useSHA256) throws Exception {
        if (key instanceof RSAPublicKey) {
            return printRSA((RSAPublicKey) key, useSHA256);
        } else if (key instanceof ECPublicKey) {
            return printEC((ECPublicKey) key);
        } else {
            throw new Exception("Unsupported key class: " + key.getClass().getName());
        }
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: DumpPublicKey certfile ... > source.c");
            System.exit(1);
        }
        Security.addProvider(new BouncyCastleProvider());
        try {
            for (int i = 0; i < args.length; i++) {
                FileInputStream input = new FileInputStream(args[i]);
                CertificateFactory cf = CertificateFactory.getInstance("X.509");
                X509Certificate cert = (X509Certificate) cf.generateCertificate(input);

                boolean useSHA256 = false;
                String sigAlg = cert.getSigAlgName();
                if ("SHA1withRSA".equals(sigAlg) || "MD5withRSA".equals(sigAlg)) {
                    // SignApk has historically accepted "MD5withRSA"
                    // certificates, but treated them as "SHA1withRSA"
                    // anyway.  Continue to do so for backwards
                    // compatibility.
                  useSHA256 = false;
                } else if ("SHA256withRSA".equals(sigAlg) || "SHA256withECDSA".equals(sigAlg)) {
                  useSHA256 = true;
                } else {
                  System.err.println(args[i] + ": unsupported signature algorithm \"" +
                                     sigAlg + "\"");
                  System.exit(1);
                }

                PublicKey key = cert.getPublicKey();
                check(key, useSHA256);
                System.out.print(print(key, useSHA256));
                System.out.println(i < args.length - 1 ? "," : "");
            }
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
        System.exit(0);
    }
}
