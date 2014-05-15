mlboehm:
	cd .. && \
	gmake -f Makefile.CHERI ml-all
	cat testall-template.sh | sed s/PROG/\\/mnt\\/mbv21zfs\\/cloud2mltest-boehm/g > testall.sh && \
	cd .. && \
	gmake -f Makefile.testall push-testall run && \
	cp objects/run_output saved_test_output/tmp/run_outputM-boehm && \
	cd test

mlnogc:
	cd .. && \
	gmake -f Makefile.CHERI ml-all
	cat testall-template.sh | sed s/PROG/\\/mnt\\/mbv21zfs\\/cloud2mltest-no-gc/g > testall.sh && \
	cd .. && \
	gmake -f Makefile.testall push-testall run && \
	cp objects/run_output saved_test_output/tmp/run_outputM-no-gc && \
	cd test

mlnocap:
	cd .. && \
	gmake -f Makefile.CHERI ml-all
	cat testall-template.sh | sed s/PROG/\\/mnt\\/mbv21zfs\\/cloud2mltest-no-cap/g > testall.sh && \
	cd .. && \
	gmake -f Makefile.testall push-testall run && \
	cp objects/run_output saved_test_output/tmp/run_outputM-no-cap && \
	cd test

mlcheri:
	cd .. && \
	gmake -f Makefile.CHERI ml-all
	cat testall-template.sh | sed s/PROG/\\/mnt\\/mbv21zfs\\/cloud2mltest/g > testall.sh && \
	cd .. && \
	gmake -f Makefile.testall push-testall run && \
	cp objects/run_output saved_test_output/tmp/run_outputM && \
	cd test
