PermsBitmask.vo PermsBitmask.glob PermsBitmask.v.beautified PermsBitmask.required_vo: PermsBitmask.v 
PermsBitmask.vos PermsBitmask.vok PermsBitmask.required_vos: PermsBitmask.v 
CapabilityModel.vo CapabilityModel.glob CapabilityModel.v.beautified CapabilityModel.required_vo: CapabilityModel.v PermsBitmask.vo
CapabilityModel.vos CapabilityModel.vok CapabilityModel.required_vos: CapabilityModel.v PermsBitmask.vos
DerivationChain.vo DerivationChain.glob DerivationChain.v.beautified DerivationChain.required_vo: DerivationChain.v PermsBitmask.vo CapabilityModel.vo
DerivationChain.vos DerivationChain.vok DerivationChain.required_vos: DerivationChain.v PermsBitmask.vos CapabilityModel.vos
ChainDecidability.vo ChainDecidability.glob ChainDecidability.v.beautified ChainDecidability.required_vo: ChainDecidability.v PermsBitmask.vo CapabilityModel.vo DerivationChain.vo Decidability.vo
ChainDecidability.vos ChainDecidability.vok ChainDecidability.required_vos: ChainDecidability.v PermsBitmask.vos CapabilityModel.vos DerivationChain.vos Decidability.vos
LedgerInvariants.vo LedgerInvariants.glob LedgerInvariants.v.beautified LedgerInvariants.required_vo: LedgerInvariants.v PermsBitmask.vo CapabilityModel.vo DerivationChain.vo ChainDecidability.vo
LedgerInvariants.vos LedgerInvariants.vok LedgerInvariants.required_vos: LedgerInvariants.v PermsBitmask.vos CapabilityModel.vos DerivationChain.vos ChainDecidability.vos
Decidability.vo Decidability.glob Decidability.v.beautified Decidability.required_vo: Decidability.v PermsBitmask.vo CapabilityModel.vo
Decidability.vos Decidability.vok Decidability.required_vos: Decidability.v PermsBitmask.vos CapabilityModel.vos
