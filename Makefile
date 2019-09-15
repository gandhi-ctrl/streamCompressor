#-------------- User settings ------------
LIBRARIES         = argEval

INCLUDES          = argEval

PROGRAM_NAMES     = sc

ADDITIONAL_LDFLAGS= 

ifndef STATICLIBNAME
STATICLIBNAME     = unknown
endif
#-----------------------------------------

include Makefile.include