# Environment Device Verification
# Formal proofs for devenv.c reference counting, allocation, and hash table

COQC = coqc
COQFLAGS = -R . env

VFILES = types.v conservation.v hash.v
VOFILES = $(VFILES:.v=.vo)

.PHONY: all verify clean check

all: verify

verify: $(VOFILES)
	@echo "  ✅ Environment proofs verified"

types.vo: types.v
	@echo "  Compiling types.v..."
	@$(COQC) $(COQFLAGS) types.v

conservation.vo: conservation.v types.vo
	@echo "  Compiling conservation.v..."
	@$(COQC) $(COQFLAGS) conservation.v

hash.vo: hash.v types.vo
	@echo "  Compiling hash.v..."
	@$(COQC) $(COQFLAGS) hash.v

check: verify
	@echo "Checking for incomplete proofs..."
	@if grep -l "Admitted\." *.v 2>/dev/null; then \
		echo "  ⚠️  Some proofs are incomplete"; \
		exit 1; \
	else \
		echo "  ✅ All proofs complete"; \
	fi

clean:
	rm -f *.vo *.vok *.vos *.glob .*.aux
