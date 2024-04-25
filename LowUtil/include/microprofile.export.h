#pragma once
#  ifndef MICROPROFILE_API
#    ifdef lowutil_EXPORTS
        /* We are building this library */
#      define MICROPROFILE_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define MICROPROFILE_API __declspec(dllimport)
#    endif
#  endif