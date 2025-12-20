ledger_crypto.vo ledger_crypto.glob ledger_crypto.v.beautified ledger_crypto.required_vo: ledger_crypto.v 
ledger_crypto.vos ledger_crypto.vok ledger_crypto.required_vos: ledger_crypto.v 
ledger_state.vo ledger_state.glob ledger_state.v.beautified ledger_state.required_vo: ledger_state.v ledger_crypto.vo
ledger_state.vos ledger_state.vok ledger_state.required_vos: ledger_state.v ledger_crypto.vos
ledger_merkle.vo ledger_merkle.glob ledger_merkle.v.beautified ledger_merkle.required_vo: ledger_merkle.v ledger_crypto.vo
ledger_merkle.vos ledger_merkle.vok ledger_merkle.required_vos: ledger_merkle.v ledger_crypto.vos
ledger_epoch.vo ledger_epoch.glob ledger_epoch.v.beautified ledger_epoch.required_vo: ledger_epoch.v ledger_crypto.vo ledger_state.vo
ledger_epoch.vos ledger_epoch.vok ledger_epoch.required_vos: ledger_epoch.v ledger_crypto.vos ledger_state.vos
ledger_implementation.vo ledger_implementation.glob ledger_implementation.v.beautified ledger_implementation.required_vo: ledger_implementation.v ledger_crypto.vo ledger_state.vo
ledger_implementation.vos ledger_implementation.vok ledger_implementation.required_vos: ledger_implementation.v ledger_crypto.vos ledger_state.vos
