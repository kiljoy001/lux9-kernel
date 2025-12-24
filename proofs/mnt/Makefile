# Mount Device Verification
# Formal proofs for devmnt.c tag allocation and RPC management

COQC = coqc
COQFLAGS = -R . mnt

VFILES = types.v tag.v rpc.v
VOFILES = $(VFILES:.v=.vo)

.PHONY: all verify clean check

all: verify

verify: $(VOFILES)
	@echo "  ✅ Mount proofs verified"

types.vo: types.v
	@echo "  Compiling types.v..."
	@$(COQC) $(COQFLAGS) types.v

tag.vo: tag.v types.vo
	@echo "  Compiling tag.v..."
	@$(COQC) $(COQFLAGS) tag.v

rpc.vo: rpc.v types.vo
	@echo "  Compiling rpc.v..."
	@$(COQC) $(COQFLAGS) rpc.v

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
