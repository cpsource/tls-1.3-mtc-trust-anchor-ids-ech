lib_LIBRARIES = libmtc.a

libmtc_a_SOURCES = mtc.c mtc.h
libmtc_a_CFLAGS = $(CURL_CFLAGS) $(JSONC_CFLAGS) $(OPENSSL_CFLAGS)

bin_PROGRAMS = mtc-example
mtc_example_SOURCES = example.c
mtc_example_LDADD = libmtc.a $(CURL_LIBS) $(JSONC_LIBS) $(OPENSSL_LIBS)
mtc_example_CFLAGS = $(CURL_CFLAGS) $(JSONC_CFLAGS) $(OPENSSL_CFLAGS)

include_HEADERS = mtc.h
