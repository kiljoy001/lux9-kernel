resource_algebra.vo resource_algebra.glob resource_algebra.v.beautified resource_algebra.required_vo: resource_algebra.v 
resource_algebra.vos resource_algebra.vok resource_algebra.required_vos: resource_algebra.v 
router_template.vo router_template.glob router_template.v.beautified router_template.required_vo: router_template.v resource_algebra.vo
router_template.vos router_template.vok router_template.required_vos: router_template.v resource_algebra.vos
router_model.vo router_model.glob router_model.v.beautified router_model.required_vo: router_model.v resource_algebra.vo router_template.vo
router_model.vos router_model.vok router_model.required_vos: router_model.v resource_algebra.vos router_template.vos
router_safety.vo router_safety.glob router_safety.v.beautified router_safety.required_vo: router_safety.v router_model.vo
router_safety.vos router_safety.vok router_safety.required_vos: router_safety.v router_model.vos
