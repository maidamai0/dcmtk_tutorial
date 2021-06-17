# Tutorial for [DCMTK](https://dicom.offis.de/dcmtk.php.en)

![Windows](https://github.com/maidamai0/dcmtk_tutorial/actions/workflows/windows.yml/badge.svg)
![Linux](https://github.com/maidamai0/dcmtk_tutorial/actions/workflows/linux.yml/badge.svg)

## Generate SSL certificate and key

Generate certificates in [res](./res) directory.

```shell
cd res
openssl req -x509 -days 3650 -newkey rsa:2048 -keyout key.pem -out cert.pem -subj '/CN=localhost'
openssl pkcs12 -export -in cert.pem -inkey key.pem -out cert.pfx
openssl pkcs12 -in cert.pfx -clcerts -nokeys -out cert_public.pem
```

:bulb: Use `1234` as password when openssl requested otherwise mofify or comment [tls_helper.hpp:48](src/tls_helper.hpp)

```cpp
tls_layer_->setPrivateKeyPasswd("1234");
```
