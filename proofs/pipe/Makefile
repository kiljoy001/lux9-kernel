# Pipe Device Verification
# Formal proofs for devpipe.c reference counting and safety

COQC = coqc
COQFLAGS = -R . pipe

VFILES = types.v conservation.v safety.v
VOFILES = $(VFILES:.v=.vo)

.PHONY: all verify clean check

all: verify

verify: $(VOFILES)
	@echo "  ✅ Pipe proofs verified"

types.vo: types.v
	@echo "  Compiling types.v..."
	@$(COQC) $(COQFLAGS) types.v

conservation.vo: conservation.v types.vo
	@echo "  Compiling conservation.v..."
	@$(COQC) $(COQFLAGS) conservation.v

safety.vo: safety.v types.vo conservation.vo
	@echo "  Compiling safety.v..."
	@$(COQC) $(COQFLAGS) safety.v

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
