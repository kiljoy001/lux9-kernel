# Relooper Proofs Makefile
# Verifies CFG construction and Ramsey algorithm correctness

COQC = coqc

.PHONY: all verify clean

all: verify

# Proof files in dependency order
verify: cfg_spec.vo domtree_spec.vo ramsey_spec.vo
	@echo "✅ All relooper proofs verified!"

cfg_spec.vo: cfg_spec.v
	$(COQC) cfg_spec.v

domtree_spec.vo: domtree_spec.v
	$(COQC) domtree_spec.v

ramsey_spec.vo: ramsey_spec.v
	$(COQC) ramsey_spec.v

clean:
	rm -f *.vo *.vok *.vos *.glob .*.aux
