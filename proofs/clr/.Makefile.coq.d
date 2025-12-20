il_types.vo il_types.glob il_types.v.beautified il_types.required_vo: il_types.v 
il_types.vos il_types.vok il_types.required_vos: il_types.v 
stack_machine.vo stack_machine.glob stack_machine.v.beautified stack_machine.required_vo: stack_machine.v 
stack_machine.vos stack_machine.vok stack_machine.required_vos: stack_machine.v 
il_semantics.vo il_semantics.glob il_semantics.v.beautified il_semantics.required_vo: il_semantics.v 
il_semantics.vos il_semantics.vok il_semantics.required_vos: il_semantics.v 
fruity_semantics.vo fruity_semantics.glob fruity_semantics.v.beautified fruity_semantics.required_vo: fruity_semantics.v 
fruity_semantics.vos fruity_semantics.vok fruity_semantics.required_vos: fruity_semantics.v 
il_to_fruity_correct.vo il_to_fruity_correct.glob il_to_fruity_correct.v.beautified il_to_fruity_correct.required_vo: il_to_fruity_correct.v il_semantics.vo fruity_semantics.vo
il_to_fruity_correct.vos il_to_fruity_correct.vok il_to_fruity_correct.required_vos: il_to_fruity_correct.v il_semantics.vos fruity_semantics.vos
pebble_clr.vo pebble_clr.glob pebble_clr.v.beautified pebble_clr.required_vo: pebble_clr.v fruity_semantics.vo il_to_fruity_correct.vo
pebble_clr.vos pebble_clr.vok pebble_clr.required_vos: pebble_clr.v fruity_semantics.vos il_to_fruity_correct.vos
il_tests.vo il_tests.glob il_tests.v.beautified il_tests.required_vo: il_tests.v il_semantics.vo fruity_semantics.vo il_to_fruity_correct.vo
il_tests.vos il_tests.vok il_tests.required_vos: il_tests.v il_semantics.vos fruity_semantics.vos il_to_fruity_correct.vos
