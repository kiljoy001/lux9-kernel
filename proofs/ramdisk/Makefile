.PHONY: all clean verify

all: verify

Makefile.coq: _CoqProject
	coq_makefile -f _CoqProject -o Makefile.coq

verify: Makefile.coq
	$(MAKE) -f Makefile.coq

clean:
	if [ -f Makefile.coq ]; then $(MAKE) -f Makefile.coq clean; fi
	rm -f Makefile.coq
	rm -f *.vo *.vok *.vos *.glob .*.aux
