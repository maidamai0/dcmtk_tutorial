#include "cxxopts.hpp"
#include "dcmtk/dcmdata/dcdatset.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmdata/dcobject.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmnet/assoc.h"
#include "dcmtk/dcmnet/cond.h"
#include "dcmtk/dcmnet/dcmtrans.h"
#include "dcmtk/dcmnet/dicom.h"
#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/oflog/logger.h"
#include "dcmtk/oflog/loglevel.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofstring.h"
#include "log.hpp"
#include "tls_helper.hpp"

namespace {
/* DICOM standard transfer syntaxes */
static const char* transfer_syntaxes[] = {
    UID_LittleEndianImplicitTransferSyntax, /* default xfer syntax first */
    UID_LittleEndianExplicitTransferSyntax,
    UID_BigEndianExplicitTransferSyntax,
};
}  // namespace

int main(int argc, char** argv) {
  cxxopts::Options options("EchoScu", "Echo Scu");
  // default dicom port of orthanc

  // clang-format off
  options.add_options()
  ("H,host", "Server address", cxxopts::value<std::string>()->default_value("localhost"))
  ("p,port", "Server port", cxxopts::value<int>()->default_value("4646"))
  ("t,title", "Server Application title", cxxopts::value<std::string>()->default_value("ANY_SCP"))
  ("h,help", "Print usage");
  // clang-format on
  cxxopts::ParseResult args;
  try {
    args = options.parse(argc, argv);
    if (args.count("help")) {
      fmt::print(options.help());
      return EXIT_SUCCESS;
    }
  } catch (cxxopts::OptionException* e) {
    fmt::print(options.help());
    return EXIT_SUCCESS;
  }

  dcmtk::log4cplus::Logger::getRoot().setLogLevel(dcmtk::log4cplus::TRACE_LOG_LEVEL);

  OFStandard::initializeNetwork();

  // socket
  constexpr auto socket_time_out = 5;
  dcmSocketSendTimeout.set(socket_time_out);
  dcmSocketReceiveTimeout.set(socket_time_out);

  // load private tags
  if (!dcmDataDict.isDictionaryLoaded()) {
    LOGD("no dictionary loaded, check environment variable:{}", DCM_DICT_ENVIRONMENT_VARIABLE);
  }

  OFString error_msg;

  T_ASC_Network* asc_network;
  constexpr auto asc_timeout = 10;
  auto cond = ASC_initializeNetwork(NET_REQUESTOR, 0, asc_timeout, &asc_network);
  if (cond.bad()) {
    LOGD("asociation initialize network failed:{}", DimseCondition::dump(error_msg, cond));
    return EXIT_FAILURE;
  }

  T_ASC_Parameters* asc_parameter;
  cond = ASC_createAssociationParameters(&asc_parameter, ASC_DEFAULTMAXPDU);
  if (cond.bad()) {
    LOGD("create association parameter failed:{}", DimseCondition::dump(error_msg, cond));
    return EXIT_FAILURE;
  }

  tls::TslHeper tls;
  cond = tls.Init(asc_network, asc_parameter, res_path("client.key"), res_path("client.crt"), tls::EndPoint::kClient);
  if (cond.bad()) {
    LOGE("Initialize TLS failed:{}", err_msg(cond));
    return EXIT_FAILURE;
  }

  cond = tls.AddTrustedCertificate(res_path("server.crt"));
  if (cond.bad()) {
    LOGE("Add trusted certificate file failed:{}", err_msg(cond));
    return EXIT_FAILURE;
  }

  constexpr auto our_app_title = "ECHOSCU";
  const auto peer_app_title = args["title"].as<std::string>();
  const auto peer_host = args["host"].as<std::string>();
  const auto peer_port = args["port"].as<int>();
  ASC_setAPTitles(asc_parameter, our_app_title, peer_app_title.c_str(), nullptr);
  ASC_setPresentationAddresses(asc_parameter, OFStandard::getHostName().c_str(),
                               fmt::format("{}:{}", peer_host, peer_port).c_str());

  constexpr auto asc_transfer_syntax_num = 1;
  cond = ASC_addPresentationContext(asc_parameter, 1, UID_VerificationSOPClass, transfer_syntaxes,
                                    asc_transfer_syntax_num);
  if (cond.bad()) {
    LOGW("Add presentation context failed:{}", DimseCondition::dump(error_msg, cond));
    return EXIT_FAILURE;
  }

  LOGI("Request parameters:\n{}", ASC_dumpParameters(error_msg, asc_parameter, ASC_ASSOC_RQ));
  LOGI("Connecting to {}:{}", peer_host, peer_port);
  T_ASC_Association* asc_association;
  cond = ASC_requestAssociation(asc_network, asc_parameter, &asc_association);
  if (cond.bad()) {
    if (cond == DUL_ASSOCIATIONREJECTED) {
      T_ASC_RejectParameters rej;
      ASC_getRejectParameters(asc_parameter, &rej);
      LOGD("Association rejected:{}", ASC_printRejectParameters(error_msg, &rej));
      return EXIT_FAILURE;
    } else {
      LOGD("Association Request failed:{}", DimseCondition::dump(error_msg, cond));
    }
  }

  LOGD("Association parameter negotiated:\n{}", ASC_dumpParameters(error_msg, asc_parameter, ASC_ASSOC_AC));
  if (ASC_countAcceptedPresentationContexts(asc_parameter) == 0) {
    LOGD("No acceptable presentation contexts");
    return EXIT_FAILURE;
  }
  LOGD("Assocation accepted, max send PDV:{}", asc_association->sendPDVLength);

  auto msg_id = asc_association->nextMsgID++;
  DIC_US status;
  DcmDataset* status_details = nullptr;
  LOGD("Sending echo request, message id:{}", msg_id);

  cond = DIMSE_echoUser(asc_association, msg_id, DIMSE_NONBLOCKING, 10, &status, &status_details);
  if (cond.good()) {
    LOGD("Received echo response:{}", DU_cechoStatusString(status));
  } else {
    OFString error_msg;
    LOGD("Echo failed:{}", DimseCondition::dump(error_msg, cond));
  }

  if (status_details) {
    // LOGD("Status details(shoud be empty):{}",
    //            DcmObject::PrintHelper(*status_details));
    delete status_details;
  }

  if (cond == EC_Normal) {
    LOGD("Release association");
    cond = ASC_releaseAssociation(asc_association);
    if (cond.bad()) {
      LOGD("Association release failed:{}", DimseCondition::dump(error_msg, cond));
      return EXIT_FAILURE;
    }
  } else if (cond == DUL_PEERREQUESTEDRELEASE) {
    LOGD("Protocol error: peer requested to release, aborting...");
    cond = ASC_abortAssociation(asc_association);
    if (cond.bad()) {
      LOGD("Association abort failed:{}", DimseCondition::dump(error_msg, cond));
      return EXIT_FAILURE;
    }
  } else if (cond == DUL_PEERABORTEDASSOCIATION) {
    LOGD("Peer aborted association");
  } else {
    LOGD("Echo scu failed:{}, aborting...", DimseCondition::dump(error_msg, cond));
    cond = ASC_abortAssociation(asc_association);
    if (cond.bad()) {
      LOGD("Association abort failed:{}", DimseCondition::dump(error_msg, cond));
      return EXIT_FAILURE;
    }
  }

  cond = ASC_dropNetwork(&asc_network);
  if (cond.bad()) {
    LOGD("Drop network failed:{}", DimseCondition::dump(error_msg, cond));
    return EXIT_FAILURE;
  }

  OFStandard::shutdownNetwork();

  return EXIT_SUCCESS;
}