#pragma once

#include <utility>

#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmnet/assoc.h"
#include "dcmtk/dcmtls/tlsciphr.h"
#include "dcmtk/dcmtls/tlslayer.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/ofstd/oftypes.h"
#include "log.hpp"
#include "utility.hpp"

namespace tls {

enum class EndPoint { kClient, kServer };

#ifndef WITH_OPENSSL
namespace details {

class None {
 public:
  OFCondition Init(T_ASC_Network* net, T_ASC_Parameters* param, std::string&& key_path, std::string&& cert_path,
                   EndPoint end_point = EndPoint::kServer) {
    return OFCondition(EC_Normal);
  }
  auto AddTrustedCertificate(std::string&& path) { return OFCondition(EC_Normal); }
};
}  // namespace details

using TslHeper = details::None;
#endif

#ifdef WITH_OPENSSL
namespace details {
class DcmTsl {
 public:
  DcmTsl() = default;
  OFCondition Init(T_ASC_Network* net, T_ASC_Parameters* param, std::string&& key_path, std::string&& cert_path,
                   EndPoint end_point = EndPoint::kServer) {
    LOGI("OpenSSL version: {}", DcmTLSTransportLayer::getOpenSSLVersionName());
    tls_layer_ =
        new DcmTLSTransportLayer(end_point == EndPoint::kServer ? NET_ACCEPTOR : NET_REQUESTOR, nullptr, OFTrue);
    if (!tls_layer_) {
      LOGE("Create TLS failed");
      return OFCondition(EC_IllegalCall);
    }
    tls_layer_->setPrivateKeyPasswd("1234");
    auto cond = tls_layer_->setPrivateKeyFile(key_path.c_str(), DCF_Filetype_PEM);
    if (cond.bad()) {
      LOGE("Load private key file[{}] failed:{}", key_path, err_msg(cond));
      return cond;
    }

    cond = tls_layer_->setCertificateFile(cert_path.c_str(), DCF_Filetype_PEM);
    if (cond.bad()) {
      LOGE("Load certificate file[{}] failed:{}", cert_path, err_msg(cond));
      return cond;
    }

    // TODO (tonghao): 2021-06-17
    // may need change
    cond = tls_layer_->setTLSProfile(TSP_Profile_BCP195);
    if (cond.bad()) {
      LOGE("Set TLS profile failed:{}", cert_path, err_msg(cond));
      return cond;
    }

    cond = tls_layer_->activateCipherSuites();
    if (cond.bad()) {
      LOGE("Activate cipher suites failed:{}", cert_path, err_msg(cond));
      return cond;
    }

    // TODO (tonghao): 2021-06-17
    // not recommend
    // tls_layer_->setCertificateVerification(DCV_ignoreCertificate);

    cond = ASC_setTransportLayer(net, tls_layer_, 1);
    if (param) {
      cond = ASC_setTransportLayerType(param, OFTrue);
    }

    return cond;
  }

  auto AddTrustedCertificate(std::string&& path) {
    return tls_layer_->addTrustedCertificateFile(path.c_str(), DCF_Filetype_PEM);
  }

 private:
  DcmTLSTransportLayer* tls_layer_ = nullptr;
};
}  // namespace details

using TslHeper = details::DcmTsl;
#endif

}  // namespace tls