# Cache Proofs Makefile

.PHONY: all verify clean

COQC := coqc
COQFLAGS := 

VFILES := cache_state.v cache_safety.v

all: verify

verify: $(VFILES:.v=.vo)
	@echo "  ✅ cache proofs verified"

%.vo: %.v
	@echo "  Compiling $<..."
	@$(COQC) $(COQFLAGS) $<

clean:
	@rm -f *.vo *.glob .*.aux *.vok *.vos

check: verify
	@echo "Cache proofs: OK"
