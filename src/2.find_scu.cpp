#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmdata/dcxfer.h"
#include "dcmtk/dcmnet/cond.h"
#include "dcmtk/dcmnet/dfindscu.h"
#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/oftypes.h"
#include "log.hpp"

int main(int argc, char **argv) {
  // 1. initialize underline network
  OFStandard::initializeNetwork();

  // 2. create DcmFindSCU
  DcmFindSCU find_scu;

  // 3 initialize association
  auto cond = find_scu.initializeNetwork(10);
  OFString error;
  if (cond.bad()) {
    LOGE("Initialize association network failed:{}", DimseCondition::dump(error, cond));
    return EXIT_FAILURE;
  }

  // 4. find
  OFList<OFString> override_keys;
  cond =
      find_scu.performQuery("localhost", 4243, "FINDSCU", "ANY-SCP", UID_FINDPatientRootQueryRetrieveInformationModel,
                            EXS_LittleEndianExplicit, DIMSE_BLOCKING, 0, ASC_DEFAULTMAXPDU, OFFalse, OFFalse, 1,
                            FEM_dicomFile, 100, &override_keys, nullptr, nullptr, nullptr, nullptr);
  if (cond.bad()) {
    LOGE("Query failed:{}", DimseCondition::dump(error, cond));
    return EXIT_FAILURE;
  }

  // close connection
  find_scu.dropNetwork();
  OFStandard::shutdownNetwork();

  return EXIT_SUCCESS;
}