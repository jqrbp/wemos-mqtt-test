#ifndef _CERT_H_
#define _CERT_H_

//openssl x509 -in <x509 crt file> -out <out file> -outform DER
//openssl rsa -in <rsa key file> -out <out file> -outform DER
//xxd -i <input file cert file>

extern unsigned int ca_crt_der_len;
extern unsigned char* ca_crt_der;

extern unsigned int wemos_crt_der_len;
extern unsigned char *wemos_crt_der;

extern unsigned int wemos_key_der_len;
extern unsigned char *wemos_key_der;

#endif
