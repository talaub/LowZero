#pragma once

#define LOWR_ASSERT_RETURN(cond, text)                               \
  {                                                                  \
    if (!cond) {                                                     \
      LOW_LOG_ERROR << text << LOW_LOG_END;                          \
      return false;                                                  \
    }                                                                \
  }
