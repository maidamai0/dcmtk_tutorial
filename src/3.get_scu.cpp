#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmnet/assoc.h"
#include "dcmtk/dcmnet/cond.h"
#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/dcmnet/scu.h"
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/ofstd/oftypes.h"
#include "log.hpp"

int main(int argc, char** argv) {
  DcmSCU scu;
  scu.setMaxReceivePDULength(ASC_DEFAULTMAXPDU);
  scu.setACSETimeout(10);
  scu.setDIMSEBlockingMode(DIMSE_NONBLOCKING);
  scu.setDIMSETimeout(10);
  scu.setAETitle("GETSCU");
  scu.setPeerHostName("localhost");
  scu.setPeerPort(4243);
  scu.setPeerAETitle("ANY_SCP");
  scu.setVerbosePCMode(OFTrue);

  OFList<OFString> syntaxes;
  syntaxes.push_back(UID_LittleEndianExplicitTransferSyntax);
  syntaxes.push_back(UID_LittleEndianImplicitTransferSyntax);
  syntaxes.push_back(UID_BigEndianExplicitTransferSyntax);

  scu.addPresentationContext(UID_GETPatientRootQueryRetrieveInformationModel, syntaxes);
  for (int i = 0; i < numberOfDcmLongSCUStorageSOPClassUIDs; ++i) {
    scu.addPresentationContext(dcmLongSCUStorageSOPClassUIDs[i], syntaxes, ASC_SC_ROLE_SCP);
  }

  scu.setStorageMode(DCMSCU_STORAGE_DISK);
  scu.setStorageDir("get_scu");

  auto cond = scu.initNetwork();
  OFString error;
  if (cond.bad()) {
    LOGE("Initialize network failed:{}\n", DimseCondition::dump(error, cond));
    exit(EXIT_FAILURE);
  }

  cond = scu.negotiateAssociation();
  if (cond.bad()) {
    LOGE("Negotiate association failed:{}\n", DimseCondition::dump(error, cond));
    exit(EXIT_FAILURE);
  }

  auto presentation_ctx_id = scu.findPresentationContextID(UID_GETStudyRootQueryRetrieveInformationModel, "");
  if (presentation_ctx_id == 0) {
    LOGE("No adequate presentation context for sending C_GET");
    exit(EXIT_FAILURE);
  }

  DcmFileFormat file_format;
  auto* data_set = file_format.getDataset();
  OFListConstIterator(OFString) it;

  OFList<RetrieveResponse*> responses;
  cond = scu.sendCGETRequest(presentation_ctx_id, data_set, &responses);
  if (cond.bad()) {
    LOGE("Send C-Get failed:{}\n", DimseCondition::dump(error, cond));
    exit(EXIT_FAILURE);
  }

  if (!responses.empty()) {
    LOGI("Final status report from last C-GET message:");
    responses.back()->print();
    for (auto* res : responses) {
      delete res;
    }
    responses.clear();
  }

  if (cond == EC_Normal) {
    scu.releaseAssociation();
  } else if (cond == DUL_PEERREQUESTEDRELEASE) {
    scu.closeAssociation(DCMSCU_PEER_REQUESTED_RELEASE);
    return EXIT_FAILURE;
  } else if (cond == DUL_PEERABORTEDASSOCIATION) {
    scu.closeAssociation(DCMSCU_PEER_ABORTED_ASSOCIATION);
    return EXIT_FAILURE;
  } else {
    LOGE("Get SCU failed:{}", DimseCondition::dump(error, cond));
    return EXIT_FAILURE;
    scu.abortAssociation();
  }

  return EXIT_SUCCESS;
}