#pragma once

#include <string>

#include "dcmtk/dcmnet/cond.h"
#include "dcmtk/ofstd/ofcond.h"

inline auto err_msg(OFCondition& cond) {
  OFString msg;
  return DimseCondition::dump(msg, cond);
}

inline auto res_path(std::string name) {
  return std::string(RES_PATH) + name;
}