proc_state_dag.vo proc_state_dag.glob proc_state_dag.v.beautified proc_state_dag.required_vo: proc_state_dag.v 
proc_state_dag.vos proc_state_dag.vok proc_state_dag.required_vos: proc_state_dag.v 
proc_wasm.vo proc_wasm.glob proc_wasm.v.beautified proc_wasm.required_vo: proc_wasm.v proc_state_dag.vo
proc_wasm.vos proc_wasm.vok proc_wasm.required_vos: proc_wasm.v proc_state_dag.vos
