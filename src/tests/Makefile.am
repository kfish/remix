noinst_PROGRAMS = sndfiletest

INCLUDES = -I$(top_srcdir)/include

REMIX_LIBS = ../ctxdata/libctxdata.la ../libremix/libremix.la -ldl

sndfiletest_SOURCES = sndfiletest.c
sndfiletest_LDADD = $(REMIX_LIBS) @SNDFILE_LIBS@


check: start-test sndfile-test end-test

start-test:
	@echo "----------------------------------------------------------------------"
	@echo "Running tests"
	@echo "----------------------------------------------------------------------"

end-test:
	@echo "----------------------------------------------------------------------"
	@echo "All tests completed."
	@echo "----------------------------------------------------------------------"

sndfile-test: sndfiletest
	./sndfiletest
