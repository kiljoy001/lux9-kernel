# Makefile for pow_gate proofs

COQC := coqc
COQDEP := coqdep

VFILES := pow_gate_model.v pow_gate_safety.v
VOFILES := $(VFILES:.v=.vo)

.PHONY: all clean

all: $(VOFILES)

pow_gate_model.vo: pow_gate_model.v
	$(COQC) -R . pow_gate $<

pow_gate_safety.vo: pow_gate_safety.v pow_gate_model.vo
	$(COQC) -R . pow_gate $<

clean:
	rm -f *.vo *.glob *.vok *.vos .*.aux

verify: all
	@echo "✅ pow_gate proofs verified"
