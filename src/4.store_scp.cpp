#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "cxxopts.hpp"
#include "dcmtk/dcmdata/dcdatset.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmdata/dcobject.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmnet/assoc.h"
#include "dcmtk/dcmnet/cond.h"
#include "dcmtk/dcmnet/dcasccfg.h"
#include "dcmtk/dcmnet/dicom.h"
#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/dcmtls/tlslayer.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/ofstd/oftypes.h"
#include "log.hpp"
#include "tls_helper.hpp"
#include "utility.hpp"

OFCondition process(T_ASC_Association* assoc) {
  OFCondition cond = EC_Normal;
  T_DIMSE_Message msg;
  T_ASC_PresentationContextID presentation_cxt_id = 0;
  DcmDataset* dcm_dataset = nullptr;

  while (cond == EC_Normal || cond == DIMSE_NODATAAVAILABLE || cond == DIMSE_OUTOFRESOURCES) {
    cond = DIMSE_receiveCommand(assoc, DIMSE_BLOCKING, 0, &presentation_cxt_id, &msg, &dcm_dataset);
    if (dcm_dataset) {
      std::cout << DcmObject::PrintHelper(*dcm_dataset) << '\n';
    } else {
      LOGW("No dcmdata received");
    }

    if (cond == EC_Normal) {
      switch (msg.CommandField) {
        case DIMSE_C_ECHO_RQ:
          LOGI("Received DIMSE_C_ECHO_RQ");
          break;
        case DIMSE_C_STORE_RQ:
          LOGI("Received DIMSE_C_STORE_RQ");
          break;
        default:
          OFString tmp;
          LOGW("Bad command type:{}", DIMSE_dumpMessage(tmp, msg, DIMSE_INCOMING, nullptr, presentation_cxt_id));
      }
    }
  }

  return cond;
}

OFCondition accept_association(T_ASC_Network* net, DcmAssociationConfiguration& asc_config, OFBool secure_connection) {
  const char* known_abstract_syntaxes[1] = {UID_VerificationSOPClass};
  const char* transfer_syntaxes[21] = {};

  T_ASC_Association* assoc = nullptr;
  auto cond = ASC_receiveAssociation(net, &assoc, ASC_DEFAULTMAXPDU, nullptr, nullptr, secure_connection);
  if (cond.bad()) {
    LOGW("Association received failed:{}", err_msg(cond));
    return cond;
  }
  LOGI("Association received");

  transfer_syntaxes[0] = UID_LittleEndianExplicitTransferSyntax;
  transfer_syntaxes[1] = UID_BigEndianExplicitTransferSyntax;
  transfer_syntaxes[2] = UID_LittleEndianImplicitTransferSyntax;
  auto num_transfer_syntaxes = 3;

  cond = ASC_acceptContextsWithPreferredTransferSyntaxes(assoc->params, known_abstract_syntaxes,
                                                         DIM_OF(known_abstract_syntaxes), transfer_syntaxes,
                                                         num_transfer_syntaxes);
  if (cond.bad()) {
    LOGW("ASC_acceptContextsWithPreferredTransferSyntaxes failed: {}", err_msg(cond));
    return cond;
  }

  cond = ASC_acceptContextsWithPreferredTransferSyntaxes(assoc->params, dcmAllStorageSOPClassUIDs,
                                                         numberOfDcmAllStorageSOPClassUIDs, transfer_syntaxes,
                                                         num_transfer_syntaxes);
  if (cond.bad()) {
    LOGW("ASC_acceptContextsWithPreferredTransferSyntaxes failed: {}", err_msg(cond));
    return cond;
  }

  ASC_setAPTitles(assoc->params, nullptr, nullptr, FILE_NAME);

  std::array<char, BUFSIZ> buffer;
  cond = ASC_getApplicationContextName(assoc->params, buffer.data(), buffer.size());
  if (cond.bad() || std::string_view(buffer.data()) != std::string_view(UID_StandardApplicationContext)) {
    T_ASC_RejectParameters reject{ASC_RESULT_REJECTEDPERMANENT, ASC_SOURCE_SERVICEUSER,
                                  ASC_REASON_SU_APPCONTEXTNAMENOTSUPPORTED};
    LOGW("Association rejected, bad application context name:{}", buffer.data());
    cond = ASC_rejectAssociation(assoc, &reject);
    if (cond.bad()) {
      LOGW("Association reject faild:{}", err_msg(cond));
    }

    return cond;
  }

  cond = ASC_acknowledgeAssociation(assoc);
  if (cond.bad()) {
    LOGW("ASC_acknowledgeAssociation faild:{}", err_msg(cond));
    return cond;
  }
  LOGI("Association acknowledged");

  if (ASC_countAcceptedPresentationContexts(assoc->params) == 0) {
    LOGW("No valid presentation contexts");
  }
  OFString temp;
  LOGI("Association parameters:\n{}", ASC_dumpParameters(temp, assoc->params, ASC_ASSOC_AC));

  DIC_AE calling_title;
  DIC_AE called_title;
  if (ASC_getAPTitles(assoc->params, calling_title, sizeof(calling_title), called_title, sizeof(called_title), nullptr,
                      0)
          .good()) {
  }

  return process(assoc);
}

int main(int argc, char** argv) {
  cxxopts::Options options("StoreScp", "DICOM storage (C-STORE) SCP");
  // clang-format off
  // default dicom port of orthanc
  options.add_options()
  ("p,port", "tcp/ip port to listen on", cxxopts::value<int>()->default_value("4646"))
  ("h,help", "Print usage");
  // clang-format on
  cxxopts::ParseResult args;
  try {
    args = options.parse(argc, argv);
    if (args.count("help")) {
      fmt::print(options.help());
      return EXIT_SUCCESS;
    }
  } catch (const cxxopts::OptionException& e) {
    fmt::print(options.help());
    return EXIT_SUCCESS;
  }

  OFStandard::initializeNetwork();
  if (!dcmDataDict.isDictionaryLoaded()) {
    LOGE("Load dcm dictionary failed");
  }

  if (!dcmDataDict.isDictionaryLoaded()) {
    LOGW("no data dictionary loaded, check environment variable:{}", DCM_DICT_ENVIRONMENT_VARIABLE);
  }

  T_ASC_Network* asc_net;
  const auto port = args["port"].as<int>();
  auto cond = ASC_initializeNetwork(NET_ACCEPTOR, port, 10, &asc_net);
  OFString error;
  if (cond.bad()) {
    LOGE("Association initialize network failed:{}\n", err_msg(cond));
    return EXIT_FAILURE;
  }

  tls::TslHeper tls;
  cond = tls.Init(asc_net, nullptr, res_path("server_key.pem"), res_path("server_cert.pem"));
  if (cond.bad()) {
    LOGE("Initialize TLS failed:{}\n", err_msg(cond));
    return EXIT_FAILURE;
  }

  cond = tls.AddTrustedCertificate(res_path("client_ca.pem"));
  if (cond.bad()) {
    LOGE("Add trusted certificate file failed:{}", err_msg(cond));
    return EXIT_FAILURE;
  }

  LOGI("Listening on {}", port);

  DcmAssociationConfiguration asc_config;

  while (cond.good()) {
    cond = accept_association(asc_net, asc_config, OFTrue);
    LOGE("Accept failed:{}", err_msg(cond));
  }

  cond = ASC_dropNetwork(&asc_net);
  if (cond.bad()) {
    LOGE("Drop network failed:{}", err_msg(cond));
    return EXIT_FAILURE;
  }

  OFStandard::shutdownNetwork();

  return EXIT_SUCCESS;
}