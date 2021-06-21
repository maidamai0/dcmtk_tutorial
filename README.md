# Tutorial for [DCMTK](https://dicom.offis.de/dcmtk.php.en)

![Windows](https://github.com/maidamai0/dcmtk_tutorial/actions/workflows/windows.yml/badge.svg)
![Linux](https://github.com/maidamai0/dcmtk_tutorial/actions/workflows/linux.yml/badge.svg)

## Generate SSL certificate and key

Generate certificates in [res](./res) directory.

```shell
cd res
openssl req -x509 -days 3650 -newkey rsa:2048 -keyout server_key.pem -out server_cert.pem -subj '/CN=localhost'
openssl pkcs12 -export -in server_cert.pem -inkey server_key.pem -out cert.pfx
openssl pkcs12 -in cert.pfx -clcerts -nokeys -out server_ca.pem
rm cert.pfx

openssl req -x509 -days 3650 -newkey rsa:2048 -keyout client_key.pem -out client_cert.pem -subj '/CN=localhost'
openssl pkcs12 -export -in client_cert.pem -inkey client_key.pem -out cert.pfx
openssl pkcs12 -in cert.pfx -clcerts -nokeys -out client_ca.pem
rm cert.pfx
```

:bulb: Use `1234` as password when openssl requested otherwise mofify or comment [tls_helper.hpp:48](src/tls_helper.hpp)

```cpp
tls_layer_->setPrivateKeyPasswd("1234");
```
