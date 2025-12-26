# Mount Device Verification
# Formal proofs for devmnt.c tag allocation and RPC management

COQC = coqc
COQFLAGS = -R . mnt

VFILES = types.v tag.v rpc.v tag_queue_safety.v mntchk_safety.v channel_model.v message_routing.v
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

tag_queue_safety.vo: tag_queue_safety.v types.vo tag.vo rpc.vo
	@echo "  Compiling tag_queue_safety.v..."
	@$(COQC) $(COQFLAGS) tag_queue_safety.v

mntchk_safety.vo: mntchk_safety.v
	@echo "  Compiling mntchk_safety.v..."
	@$(COQC) $(COQFLAGS) mntchk_safety.v

channel_model.vo: channel_model.v
	@echo "  Compiling channel_model.v..."
	@$(COQC) $(COQFLAGS) channel_model.v

message_routing.vo: message_routing.v types.vo channel_model.vo
	@echo "  Compiling message_routing.v..."
	@$(COQC) $(COQFLAGS) message_routing.v

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
