namespace LeanProofs
namespace MsgOrd

abbrev MsgId := Nat
abbrev ResourceKey := Nat

def maxParents : Nat := 8
def maxResourceKeys : Nat := 4
def defaultKParam : Nat := 3

inductive Color where
  | blue
  | red
  deriving DecidableEq, Repr, Inhabited

inductive MsgState where
  | pending
  | ordered
  | delivered
  | complete
  deriving DecidableEq, Repr, Inhabited

structure MsgOrdSpec where
  resourceKeys : List ResourceKey := []
  deriving Repr, Inhabited

structure OrdMsg where
  gm_id : MsgId
  gm_timestamp : Nat := 0
  gm_parents : List MsgId := []
  gm_resource_keys : List ResourceKey := []
  gm_color : Color := Color.blue
  gm_state : MsgState := MsgState.pending
  gm_anticone_size : Nat := 0
  deriving DecidableEq, Repr, Inhabited

abbrev TipTable := List (ResourceKey × MsgId)

structure MsgOrd where
  msgs : List OrdMsg := []
  globalTip : Option MsgId := none
  barrierTip : Option MsgId := none
  tipTable : TipTable := []
  tipOverflow : Bool := false
  kParam : Nat := defaultKParam
  deriving Repr, Inhabited

def dedupKeys : List ResourceKey → List ResourceKey
  | [] => []
  | key :: rest =>
      let tail := dedupKeys rest
      if key ∈ tail then tail else key :: tail

def normalizeKeys (keys : List ResourceKey) : List ResourceKey :=
  (dedupKeys keys).take maxResourceKeys

def addParentId (parents : List MsgId) (parentId : MsgId) : List MsgId :=
  if parentId = 0 then
    parents
  else if parentId ∈ parents then
    parents
  else if parents.length < maxParents then
    parents ++ [parentId]
  else
    parents

def injectParent (parents : List MsgId) : Option MsgId → List MsgId
  | some parentId => addParentId parents parentId
  | none => parents

def tipLookup : TipTable → ResourceKey → Option MsgId
  | [], _ => none
  | (entryKey, entryId) :: rest, key =>
      if entryKey = key then some entryId else tipLookup rest key

def tipRemove (table : TipTable) (key : ResourceKey) : TipTable :=
  table.filter (fun entry => entry.fst ≠ key)

def tipUpdate (table : TipTable) (key : ResourceKey) (msgId : MsgId) : TipTable :=
  (key, msgId) :: tipRemove table key

def applySpec (msg : OrdMsg) (spec : MsgOrdSpec) : OrdMsg :=
  { msg with gm_resource_keys := normalizeKeys spec.resourceKeys }

def baseParents (dag : MsgOrd) (msg : OrdMsg) : List MsgId :=
  let parents := injectParent [] dag.barrierTip
  let parents :=
    if msg.gm_resource_keys.isEmpty then
      injectParent parents dag.globalTip
    else
      parents
  let parents :=
    if dag.tipOverflow then
      injectParent parents dag.globalTip
    else
      parents
  parents

def addResourceTipParent
    (selfId : MsgId)
    (tipTable : TipTable)
    (parents : List MsgId)
    (key : ResourceKey) : List MsgId :=
  match tipLookup tipTable key with
  | none => parents
  | some parentId =>
      if parentId = selfId then parents else addParentId parents parentId

def extendWithResourceTips
    (selfId : MsgId)
    (tipTable : TipTable)
    (parents : List MsgId) : List ResourceKey → List MsgId
  | [] => parents
  | key :: rest =>
      extendWithResourceTips selfId tipTable
        (addResourceTipParent selfId tipTable parents key) rest

def selectParents (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) : OrdMsg :=
  let msg' := applySpec msg spec
  let parents :=
    extendWithResourceTips msg'.gm_id dag.tipTable (baseParents dag msg') msg'.gm_resource_keys
  { msg' with gm_parents := parents }

def publishTipTable (table : TipTable) (msgId : MsgId) : List ResourceKey → TipTable
  | [] => table
  | key :: rest => publishTipTable (tipUpdate table key msgId) msgId rest

def publishTips (dag : MsgOrd) (msg : OrdMsg) : MsgOrd :=
  let barrierTip :=
    if msg.gm_resource_keys.isEmpty then
      some msg.gm_id
    else
      dag.barrierTip
  let tipTable := publishTipTable dag.tipTable msg.gm_id msg.gm_resource_keys
  { dag with
      globalTip := some msg.gm_id
      barrierTip := barrierTip
      tipTable := tipTable }

def lookupMsg (dag : MsgOrd) (msgId : MsgId) : Option OrdMsg :=
  dag.msgs.find? (fun msg => msg.gm_id = msgId)

def messagesConflict (lhs rhs : OrdMsg) : Prop :=
  lhs.gm_resource_keys = [] ∨
  rhs.gm_resource_keys = [] ∨
  ∃ key, key ∈ lhs.gm_resource_keys ∧ key ∈ rhs.gm_resource_keys

instance (lhs rhs : OrdMsg) : Decidable (messagesConflict lhs rhs) := by
  unfold messagesConflict
  infer_instance

def isAnticoneMember (msg candidate : OrdMsg) : Prop :=
  candidate.gm_state = MsgState.pending ∧
  candidate.gm_id ≠ msg.gm_id ∧
  candidate.gm_id ∉ msg.gm_parents ∧
  messagesConflict candidate msg

instance (msg candidate : OrdMsg) : Decidable (isAnticoneMember msg candidate) := by
  unfold isAnticoneMember
  infer_instance

def anticoneMsgs (dag : MsgOrd) (msg : OrdMsg) : List OrdMsg :=
  dag.msgs.filter (fun candidate => decide (isAnticoneMember msg candidate))

def anticone (dag : MsgOrd) (msg : OrdMsg) : Nat :=
  (anticoneMsgs dag msg).length

def deliveredOrComplete (state : MsgState) : Prop :=
  state = MsgState.delivered ∨ state = MsgState.complete

def parentReady (dag : MsgOrd) (parentId : MsgId) : Prop :=
  match lookupMsg dag parentId with
  | none => True
  | some parent => deliveredOrComplete parent.gm_state

def canDeliver (dag : MsgOrd) (msg : OrdMsg) : Prop :=
  msg.gm_color = Color.blue ∧
  ∀ parentId, parentId ∈ msg.gm_parents → parentReady dag parentId

def Inv_BlueOnly (dag : MsgOrd) : Prop :=
  ∀ msg, msg ∈ dag.msgs → msg.gm_color = Color.blue

def classifyColor (dag : MsgOrd) (msg : OrdMsg) : Color :=
  if anticone dag msg ≤ dag.kParam then Color.blue else Color.red

def assessCandidate (dag : MsgOrd) (msg : OrdMsg) : OrdMsg :=
  let anticoneSize := anticone dag msg
  { msg with
      gm_anticone_size := anticoneSize
      gm_color := if anticoneSize ≤ dag.kParam then Color.blue else Color.red }

def enqueueMsg (dag : MsgOrd) (msg : OrdMsg) : MsgOrd :=
  { dag with msgs := dag.msgs ++ [msg] }

def prepareCandidate (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) : OrdMsg :=
  assessCandidate dag (selectParents dag msg spec)

def acceptCandidate (dag : MsgOrd) (candidate : OrdMsg) : MsgOrd :=
  if candidate.gm_color = Color.blue then
    enqueueMsg (publishTips dag candidate) candidate
  else
    dag

def submitCandidate (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) : MsgOrd × OrdMsg :=
  let candidate := prepareCandidate dag msg spec
  (acceptCandidate dag candidate, candidate)

theorem normalizeKeys_length_le_maxResourceKeys (keys : List ResourceKey) :
    (normalizeKeys keys).length ≤ maxResourceKeys := by
  unfold normalizeKeys
  exact List.length_take_le maxResourceKeys (dedupKeys keys)

theorem dedupKeys_nodup : ∀ keys : List ResourceKey, (dedupKeys keys).Nodup
  | [] => by simp [dedupKeys]
  | key :: rest => by
      have ih := dedupKeys_nodup rest
      by_cases hmem : key ∈ dedupKeys rest
      · simp [dedupKeys, hmem, ih]
      · simp [dedupKeys, hmem, ih]

theorem normalizeKeys_nodup (keys : List ResourceKey) :
    (normalizeKeys keys).Nodup := by
  unfold normalizeKeys
  exact (List.take_sublist maxResourceKeys (dedupKeys keys)).nodup (dedupKeys_nodup keys)

theorem enqueueMsg_contains (dag : MsgOrd) (msg : OrdMsg) :
    msg ∈ (enqueueMsg dag msg).msgs := by
  simp [enqueueMsg]

theorem enqueueMsg_mem_of_mem (dag : MsgOrd) (msg existing : OrdMsg)
    (hmem : existing ∈ dag.msgs) :
    existing ∈ (enqueueMsg dag msg).msgs := by
  simp [enqueueMsg, hmem]

theorem publishTips_preserves_msgs (dag : MsgOrd) (msg : OrdMsg) :
    (publishTips dag msg).msgs = dag.msgs := by
  simp [publishTips]

theorem enqueueMsg_preserves_blueOnly (dag : MsgOrd) (msg : OrdMsg)
    (hinv : Inv_BlueOnly dag)
    (hblue : msg.gm_color = Color.blue) :
    Inv_BlueOnly (enqueueMsg dag msg) := by
  intro m hm
  simp [enqueueMsg] at hm
  rcases hm with hm | hm
  · exact hinv m hm
  · rcases hm with rfl
    exact hblue

theorem publishTips_preserves_blueOnly (dag : MsgOrd) (msg : OrdMsg)
    (hinv : Inv_BlueOnly dag) :
    Inv_BlueOnly (publishTips dag msg) := by
  intro m hm
  simpa [publishTips] using hinv m hm

theorem addParentId_mem_of_mem (parents : List MsgId) (parentId existingId : MsgId)
    (hmem : existingId ∈ parents) :
    existingId ∈ addParentId parents parentId := by
  by_cases hzero : parentId = 0
  · simp [addParentId, hzero, hmem]
  · by_cases hparent : parentId ∈ parents
    · simp [addParentId, hzero, hparent, hmem]
    · by_cases hroom : parents.length < maxParents
      · simp [addParentId, hzero, hparent, hroom, hmem]
      · simp [addParentId, hzero, hparent, hroom, hmem]

theorem addParentId_contains (parents : List MsgId) (parentId : MsgId)
    (hnz : parentId ≠ 0)
    (hroom : parents.length < maxParents ∨ parentId ∈ parents) :
    parentId ∈ addParentId parents parentId := by
  by_cases hparent : parentId ∈ parents
  · simp [addParentId, hnz, hparent]
  · cases hroom with
    | inl hlt =>
        simp [addParentId, hnz, hparent, hlt]
    | inr hmem =>
        contradiction

theorem addParentId_length_le_succ (parents : List MsgId) (parentId : MsgId) :
    (addParentId parents parentId).length ≤ parents.length + 1 := by
  by_cases hzero : parentId = 0
  · simp [addParentId, hzero]
  · by_cases hparent : parentId ∈ parents
    · simp [addParentId, hzero, hparent]
    · by_cases hroom : parents.length < maxParents
      · simp [addParentId, hzero, hparent, hroom]
      · simp [addParentId, hzero, hparent, hroom]

theorem injectParent_mem_of_mem (parents : List MsgId) (optParent : Option MsgId) (existingId : MsgId)
    (hmem : existingId ∈ parents) :
    existingId ∈ injectParent parents optParent := by
  cases optParent with
  | none =>
      simpa [injectParent] using hmem
  | some parentId =>
      simpa [injectParent] using addParentId_mem_of_mem parents parentId existingId hmem

theorem injectParent_length_le_succ (parents : List MsgId) (optParent : Option MsgId) :
    (injectParent parents optParent).length ≤ parents.length + 1 := by
  cases optParent with
  | none =>
      simp [injectParent]
  | some parentId =>
      simpa [injectParent] using addParentId_length_le_succ parents parentId

theorem addResourceTipParent_mem_of_mem
    (selfId : MsgId)
    (tipTable : TipTable)
    (parents : List MsgId)
    (key : ResourceKey)
    (existingId : MsgId)
    (hmem : existingId ∈ parents) :
    existingId ∈ addResourceTipParent selfId tipTable parents key := by
  unfold addResourceTipParent
  cases hlookup : tipLookup tipTable key with
  | none =>
      simpa [hlookup]
  | some parentId =>
      by_cases hself : parentId = selfId
      · simp [hself, hmem]
      · simp [hself]
        exact addParentId_mem_of_mem parents parentId existingId hmem

theorem addResourceTipParent_contains_lookup
    (selfId : MsgId)
    (tipTable : TipTable)
    (parents : List MsgId)
    (key : ResourceKey)
    (tipId : MsgId)
    (htip : tipLookup tipTable key = some tipId)
    (hself : tipId ≠ selfId)
    (hnz : tipId ≠ 0)
    (hroom : parents.length < maxParents ∨ tipId ∈ parents) :
    tipId ∈ addResourceTipParent selfId tipTable parents key := by
  unfold addResourceTipParent
  simp [htip, hself]
  exact addParentId_contains parents tipId hnz hroom

theorem addResourceTipParent_length_le_succ
    (selfId : MsgId)
    (tipTable : TipTable)
    (parents : List MsgId)
    (key : ResourceKey) :
    (addResourceTipParent selfId tipTable parents key).length ≤ parents.length + 1 := by
  unfold addResourceTipParent
  cases hlookup : tipLookup tipTable key with
  | none =>
      simp
  | some parentId =>
      by_cases hself : parentId = selfId
      · simp [hself]
      · simp [hself]
        exact addParentId_length_le_succ parents parentId

theorem extendWithResourceTips_mem_of_mem
    (selfId : MsgId)
    (tipTable : TipTable)
    (parents : List MsgId)
    (keys : List ResourceKey)
    (existingId : MsgId)
    (hmem : existingId ∈ parents) :
    existingId ∈ extendWithResourceTips selfId tipTable parents keys := by
  induction keys generalizing parents with
  | nil =>
      simpa [extendWithResourceTips] using hmem
  | cons key rest ih =>
      simp [extendWithResourceTips]
      exact ih _ (addResourceTipParent_mem_of_mem selfId tipTable parents key existingId hmem)

theorem extendWithResourceTips_length_le
    (selfId : MsgId)
    (tipTable : TipTable)
    (parents : List MsgId)
    (keys : List ResourceKey) :
    (extendWithResourceTips selfId tipTable parents keys).length ≤ parents.length + keys.length := by
  induction keys generalizing parents with
  | nil =>
      simp [extendWithResourceTips]
  | cons key rest ih =>
      calc
        (extendWithResourceTips selfId tipTable parents (key :: rest)).length
            = (extendWithResourceTips selfId tipTable
                (addResourceTipParent selfId tipTable parents key) rest).length := by
                  simp [extendWithResourceTips]
        _ ≤ (addResourceTipParent selfId tipTable parents key).length + rest.length := ih _
        _ ≤ (parents.length + 1) + rest.length := by
              exact Nat.add_le_add_right
                (addResourceTipParent_length_le_succ selfId tipTable parents key) rest.length
        _ = parents.length + (key :: rest).length := by
              simp [Nat.add_left_comm, Nat.add_comm]

theorem baseParents_length_le_three (dag : MsgOrd) (msg : OrdMsg) :
    (baseParents dag msg).length ≤ 3 := by
  let p0 := injectParent [] dag.barrierTip
  have hp0 : p0.length ≤ 1 := by
    have h := injectParent_length_le_succ ([] : List MsgId) dag.barrierTip
    simpa [p0] using h
  let p1 := if msg.gm_resource_keys.isEmpty then injectParent p0 dag.globalTip else p0
  have hp1 : p1.length ≤ 2 := by
    by_cases hEmpty : msg.gm_resource_keys.isEmpty
    · calc
        p1.length = (injectParent p0 dag.globalTip).length := by simp [p1, hEmpty]
        _ ≤ p0.length + 1 := injectParent_length_le_succ p0 dag.globalTip
        _ ≤ 1 + 1 := Nat.add_le_add_right hp0 1
        _ = 2 := by decide
    · calc
        p1.length = p0.length := by simp [p1, hEmpty]
        _ ≤ 1 := hp0
        _ ≤ 2 := by decide
  have hp2 :
      (if dag.tipOverflow then injectParent p1 dag.globalTip else p1).length ≤ 3 := by
    by_cases hOverflow : dag.tipOverflow
    · calc
        (if dag.tipOverflow then injectParent p1 dag.globalTip else p1).length
            = (injectParent p1 dag.globalTip).length := by simp [hOverflow]
        _ ≤ p1.length + 1 := injectParent_length_le_succ p1 dag.globalTip
        _ ≤ 2 + 1 := Nat.add_le_add_right hp1 1
        _ = 3 := by decide
    · calc
        (if dag.tipOverflow then injectParent p1 dag.globalTip else p1).length
            = p1.length := by simp [hOverflow]
        _ ≤ 2 := hp1
        _ ≤ 3 := by decide
  simpa [baseParents, p0, p1] using hp2

theorem selectParents_has_room (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (baseParents dag (applySpec msg spec)).length +
      (applySpec msg spec).gm_resource_keys.length ≤ maxParents := by
  have hbase : (baseParents dag (applySpec msg spec)).length ≤ 3 :=
    baseParents_length_le_three dag (applySpec msg spec)
  have hkeys : (applySpec msg spec).gm_resource_keys.length ≤ maxResourceKeys := by
    simpa [applySpec] using normalizeKeys_length_le_maxResourceKeys spec.resourceKeys
  have hsum :
      (baseParents dag (applySpec msg spec)).length +
        (applySpec msg spec).gm_resource_keys.length ≤ 3 + maxResourceKeys := by
    exact Nat.add_le_add hbase hkeys
  exact Nat.le_trans hsum (by decide : 3 + maxResourceKeys ≤ maxParents)

theorem baseParents_includes_barrierTip
    (dag : MsgOrd)
    (msg : OrdMsg)
    (parentId : MsgId)
    (hbar : dag.barrierTip = some parentId)
    (hnz : parentId ≠ 0) :
    parentId ∈ baseParents dag msg := by
  let p0 := injectParent [] dag.barrierTip
  have hp0 : parentId ∈ p0 := by
    simp [p0, hbar, injectParent, addParentId, hnz, maxParents]
  let p1 := if msg.gm_resource_keys.isEmpty then injectParent p0 dag.globalTip else p0
  have hp1 : parentId ∈ p1 := by
    by_cases hEmpty : msg.gm_resource_keys.isEmpty
    · have hmem : parentId ∈ injectParent p0 dag.globalTip :=
          injectParent_mem_of_mem p0 dag.globalTip parentId hp0
      simpa [p1, hEmpty] using hmem
    · simpa [p1, hEmpty] using hp0
  have hp2 : parentId ∈ (if dag.tipOverflow then injectParent p1 dag.globalTip else p1) := by
    by_cases hOverflow : dag.tipOverflow
    · have hmem : parentId ∈ injectParent p1 dag.globalTip :=
          injectParent_mem_of_mem p1 dag.globalTip parentId hp1
      simpa [hOverflow] using hmem
    · simpa [hOverflow] using hp1
  simpa [baseParents, p0, p1] using hp2

theorem extendWithResourceTips_includes_lookup
    (selfId : MsgId)
    (tipTable : TipTable)
    (parents : List MsgId)
    (keys : List ResourceKey)
    (key : ResourceKey)
    (tipId : MsgId)
    (hkey : key ∈ keys)
    (htip : tipLookup tipTable key = some tipId)
    (hself : tipId ≠ selfId)
    (hnz : tipId ≠ 0)
    (hroom : parents.length + keys.length ≤ maxParents) :
    tipId ∈ extendWithResourceTips selfId tipTable parents keys := by
  induction keys generalizing parents key tipId with
  | nil =>
      cases hkey
  | cons hd tl ih =>
      simp at hkey
      simp [extendWithResourceTips]
      cases hkey with
      | inl hhd =>
          subst hhd
          have hone : parents.length + 1 ≤ parents.length + List.length (key :: tl) := by
            have hlen : 1 ≤ List.length (key :: tl) := by simp
            exact Nat.add_le_add_left hlen parents.length
          have hsucc : parents.length + 1 ≤ maxParents := Nat.le_trans hone hroom
          have hlt : parents.length < maxParents := by
            exact Nat.lt_of_succ_le (by simpa [Nat.succ_eq_add_one] using hsucc)
          have hadded :
              tipId ∈ addResourceTipParent selfId tipTable parents key := by
            exact addResourceTipParent_contains_lookup
              selfId tipTable parents key tipId htip hself hnz (Or.inl hlt)
          exact extendWithResourceTips_mem_of_mem
            selfId tipTable (addResourceTipParent selfId tipTable parents key) tl tipId hadded
      | inr htail =>
          have hroom' :
              (addResourceTipParent selfId tipTable parents hd).length + tl.length ≤ maxParents := by
            calc
              (addResourceTipParent selfId tipTable parents hd).length + tl.length
                  ≤ (parents.length + 1) + tl.length := by
                      exact Nat.add_le_add_right
                        (addResourceTipParent_length_le_succ selfId tipTable parents hd) tl.length
              _ = parents.length + List.length (hd :: tl) := by
                    simp [Nat.add_left_comm, Nat.add_comm]
              _ ≤ maxParents := hroom
          exact ih (parents := addResourceTipParent selfId tipTable parents hd)
            key tipId htail htip hself hnz hroom'

theorem tipLookup_tipUpdate_eq
    (table : TipTable)
    (key : ResourceKey)
    (msgId : MsgId) :
    tipLookup (tipUpdate table key msgId) key = some msgId := by
  simp [tipUpdate, tipLookup]

theorem tipLookup_tipRemove_of_ne
    (table : TipTable)
    (updateKey : ResourceKey)
    (lookupKey : ResourceKey)
    (hneq : lookupKey ≠ updateKey) :
    tipLookup (tipRemove table updateKey) lookupKey = tipLookup table lookupKey := by
  induction table with
  | nil =>
      simp [tipRemove, tipLookup]
  | cons entry rest ih =>
      cases entry with
      | mk entryKey entryId =>
          by_cases hdrop : entryKey = updateKey
          · subst hdrop
            have hneq' : entryKey ≠ lookupKey := by
              intro heq
              exact hneq heq.symm
            simpa [tipRemove, tipLookup, hneq, hneq'] using ih
          · by_cases hlook : entryKey = lookupKey
            · subst hlook
              simp [tipRemove, tipLookup, hdrop]
            · simpa [tipRemove, tipLookup, hdrop, hlook] using ih

theorem tipLookup_tipUpdate_of_ne
    (table : TipTable)
    (updateKey : ResourceKey)
    (msgId : MsgId)
    (lookupKey : ResourceKey)
    (hneq : lookupKey ≠ updateKey) :
    tipLookup (tipUpdate table updateKey msgId) lookupKey = tipLookup table lookupKey := by
  have hneq' : updateKey ≠ lookupKey := by
    intro heq
    exact hneq heq.symm
  have hremove := tipLookup_tipRemove_of_ne table updateKey lookupKey hneq
  by_cases heq : updateKey = lookupKey
  · contradiction
  · simpa [tipUpdate, tipLookup, heq] using hremove

theorem publishTipTable_preserves_lookup_of_not_mem
    (table : TipTable)
    (msgId : MsgId)
    (keys : List ResourceKey)
    (lookupKey : ResourceKey)
    (hnot : lookupKey ∉ keys) :
    tipLookup (publishTipTable table msgId keys) lookupKey = tipLookup table lookupKey := by
  induction keys generalizing table with
  | nil =>
      simp [publishTipTable]
  | cons hd tl ih =>
      have hneq : lookupKey ≠ hd := by
        intro heq
        apply hnot
        simp [heq]
      have hnotTl : lookupKey ∉ tl := by
        intro hmem
        apply hnot
        simp [hmem]
      calc
        tipLookup (publishTipTable (tipUpdate table hd msgId) msgId tl) lookupKey
            = tipLookup (tipUpdate table hd msgId) lookupKey := ih _ hnotTl
        _ = tipLookup table lookupKey := tipLookup_tipUpdate_of_ne table hd msgId lookupKey hneq

theorem publishTipTable_sets_tip_of_mem
    (table : TipTable)
    (msgId : MsgId)
    (keys : List ResourceKey)
    (key : ResourceKey)
    (hkey : key ∈ keys)
    (hnodup : keys.Nodup) :
    tipLookup (publishTipTable table msgId keys) key = some msgId := by
  induction keys generalizing table with
  | nil =>
      cases hkey
  | cons hd tl ih =>
      simp at hkey
      simp at hnodup
      rcases hnodup with ⟨hnotin, hnodupTl⟩
      cases hkey with
      | inl hhd =>
          subst hhd
          calc
            tipLookup (publishTipTable (tipUpdate table key msgId) msgId tl) key
                = tipLookup (tipUpdate table key msgId) key := by
                    exact publishTipTable_preserves_lookup_of_not_mem
                      (tipUpdate table key msgId) msgId tl key hnotin
            _ = some msgId := tipLookup_tipUpdate_eq table key msgId
      | inr htl =>
          simpa [publishTipTable] using ih (tipUpdate table hd msgId) htl hnodupTl

theorem messagesConflict_barrier_left (lhs rhs : OrdMsg)
    (h : lhs.gm_resource_keys = []) : messagesConflict lhs rhs := by
  exact Or.inl h

theorem messagesConflict_barrier_right (lhs rhs : OrdMsg)
    (h : rhs.gm_resource_keys = []) : messagesConflict lhs rhs := by
  exact Or.inr (Or.inl h)

theorem messagesConflict_comm (lhs rhs : OrdMsg) :
    messagesConflict lhs rhs ↔ messagesConflict rhs lhs := by
  constructor
  · intro h
    rcases h with hleft | hright | hshare
    · exact Or.inr (Or.inl hleft)
    · exact Or.inl hright
    · rcases hshare with ⟨key, hkeyL, hkeyR⟩
      exact Or.inr (Or.inr ⟨key, hkeyR, hkeyL⟩)
  · intro h
    rcases h with hleft | hright | hshare
    · exact Or.inr (Or.inl hleft)
    · exact Or.inl hright
    · rcases hshare with ⟨key, hkeyR, hkeyL⟩
      exact Or.inr (Or.inr ⟨key, hkeyL, hkeyR⟩)

theorem applySpec_uses_normalizedKeys (msg : OrdMsg) (spec : MsgOrdSpec) :
    (applySpec msg spec).gm_resource_keys = normalizeKeys spec.resourceKeys := by
  simp [applySpec]

theorem selectParents_uses_normalizedKeys (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (selectParents dag msg spec).gm_resource_keys = normalizeKeys spec.resourceKeys := by
  simp [selectParents, applySpec]

theorem selectParents_preserves_id (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (selectParents dag msg spec).gm_id = msg.gm_id := by
  simp [selectParents, applySpec]

theorem selectParents_preserves_state (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (selectParents dag msg spec).gm_state = msg.gm_state := by
  simp [selectParents, applySpec]

theorem prepareCandidate_uses_normalizedKeys (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (prepareCandidate dag msg spec).gm_resource_keys = normalizeKeys spec.resourceKeys := by
  simp [prepareCandidate, assessCandidate, selectParents, applySpec]

theorem prepareCandidate_preserves_id (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (prepareCandidate dag msg spec).gm_id = msg.gm_id := by
  simp [prepareCandidate, assessCandidate, selectParents, applySpec]

theorem prepareCandidate_preserves_state (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (prepareCandidate dag msg spec).gm_state = msg.gm_state := by
  simp [prepareCandidate, assessCandidate, selectParents, applySpec]

theorem prepareCandidate_preserves_parents (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (prepareCandidate dag msg spec).gm_parents = (selectParents dag msg spec).gm_parents := by
  simp [prepareCandidate, assessCandidate]

theorem prepareCandidate_nodup_keys (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (prepareCandidate dag msg spec).gm_resource_keys.Nodup := by
  simpa [prepareCandidate, assessCandidate, selectParents, applySpec] using
    normalizeKeys_nodup spec.resourceKeys

theorem prepareCandidate_sets_anticone (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (prepareCandidate dag msg spec).gm_anticone_size =
      anticone dag (selectParents dag msg spec) := by
  simp [prepareCandidate, assessCandidate]

theorem prepareCandidate_blue_iff (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (prepareCandidate dag msg spec).gm_color = Color.blue ↔
      anticone dag (selectParents dag msg spec) ≤ dag.kParam := by
  unfold prepareCandidate assessCandidate
  by_cases h : anticone dag (selectParents dag msg spec) ≤ dag.kParam
  · simp [h]
  · simp [h]

theorem prepareCandidate_red_iff (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (prepareCandidate dag msg spec).gm_color = Color.red ↔
      dag.kParam < anticone dag (selectParents dag msg spec) := by
  unfold prepareCandidate assessCandidate
  constructor
  · intro hred
    have hnot : ¬ anticone dag (selectParents dag msg spec) ≤ dag.kParam := by
      intro hle
      simp [hle] at hred
    exact Nat.lt_of_not_ge hnot
  · intro hgt
    have hnot : ¬ anticone dag (selectParents dag msg spec) ≤ dag.kParam := Nat.not_le_of_gt hgt
    simp [hnot]

theorem selectParents_includes_barrierTip
    (dag : MsgOrd)
    (msg : OrdMsg)
    (spec : MsgOrdSpec)
    (parentId : MsgId)
    (hbar : dag.barrierTip = some parentId)
    (hnz : parentId ≠ 0) :
    parentId ∈ (selectParents dag msg spec).gm_parents := by
  let msg' := applySpec msg spec
  have hbase : parentId ∈ baseParents dag msg' :=
    baseParents_includes_barrierTip dag msg' parentId hbar hnz
  have hmem :
      parentId ∈ extendWithResourceTips msg'.gm_id dag.tipTable (baseParents dag msg') msg'.gm_resource_keys :=
    extendWithResourceTips_mem_of_mem msg'.gm_id dag.tipTable (baseParents dag msg') msg'.gm_resource_keys parentId hbase
  simpa [selectParents, msg'] using hmem

theorem prepareCandidate_includes_barrierTip
    (dag : MsgOrd)
    (msg : OrdMsg)
    (spec : MsgOrdSpec)
    (parentId : MsgId)
    (hbar : dag.barrierTip = some parentId)
    (hnz : parentId ≠ 0) :
    parentId ∈ (prepareCandidate dag msg spec).gm_parents := by
  simpa [prepareCandidate, assessCandidate] using
    selectParents_includes_barrierTip dag msg spec parentId hbar hnz

theorem selectParents_includes_resource_tip
    (dag : MsgOrd)
    (msg : OrdMsg)
    (spec : MsgOrdSpec)
    (key : ResourceKey)
    (tipId : MsgId)
    (hkey : key ∈ normalizeKeys spec.resourceKeys)
    (htip : tipLookup dag.tipTable key = some tipId)
    (hself : tipId ≠ msg.gm_id)
    (hnz : tipId ≠ 0) :
    tipId ∈ (selectParents dag msg spec).gm_parents := by
  let msg' := applySpec msg spec
  have hkey' : key ∈ msg'.gm_resource_keys := by
    simpa [msg'] using hkey
  have hself' : tipId ≠ msg'.gm_id := by
    simpa [msg'] using hself
  have hroom :
      (baseParents dag msg').length + msg'.gm_resource_keys.length ≤ maxParents := by
    simpa [msg'] using selectParents_has_room dag msg spec
  have hmem :
      tipId ∈ extendWithResourceTips msg'.gm_id dag.tipTable (baseParents dag msg') msg'.gm_resource_keys :=
    extendWithResourceTips_includes_lookup
      msg'.gm_id dag.tipTable (baseParents dag msg') msg'.gm_resource_keys
      key tipId hkey' htip hself' hnz hroom
  simpa [selectParents, msg'] using hmem

theorem prepareCandidate_includes_resource_tip
    (dag : MsgOrd)
    (msg : OrdMsg)
    (spec : MsgOrdSpec)
    (key : ResourceKey)
    (tipId : MsgId)
    (hkey : key ∈ normalizeKeys spec.resourceKeys)
    (htip : tipLookup dag.tipTable key = some tipId)
    (hself : tipId ≠ msg.gm_id)
    (hnz : tipId ≠ 0) :
    tipId ∈ (prepareCandidate dag msg spec).gm_parents := by
  simpa [prepareCandidate, assessCandidate] using
    selectParents_includes_resource_tip dag msg spec key tipId hkey htip hself hnz

theorem publishTips_sets_globalTip (dag : MsgOrd) (msg : OrdMsg) :
    (publishTips dag msg).globalTip = some msg.gm_id := by
  simp [publishTips]

theorem publishTips_sets_barrierTip_of_barrier (dag : MsgOrd) (msg : OrdMsg)
    (h : msg.gm_resource_keys = []) :
    (publishTips dag msg).barrierTip = some msg.gm_id := by
  simp [publishTips, h]

theorem publishTips_preserves_barrierTip_of_nonbarrier (dag : MsgOrd) (msg : OrdMsg)
    (h : msg.gm_resource_keys ≠ []) :
    (publishTips dag msg).barrierTip = dag.barrierTip := by
  cases hKeys : msg.gm_resource_keys with
  | nil =>
      contradiction
  | cons head tail =>
      simp [publishTips, hKeys]

theorem publishTips_sets_resource_tip
    (dag : MsgOrd)
    (msg : OrdMsg)
    (key : ResourceKey)
    (hkey : key ∈ msg.gm_resource_keys)
    (hnodup : msg.gm_resource_keys.Nodup) :
    tipLookup (publishTips dag msg).tipTable key = some msg.gm_id := by
  unfold publishTips
  simpa using publishTipTable_sets_tip_of_mem dag.tipTable msg.gm_id msg.gm_resource_keys key hkey hnodup

theorem publishTips_preserves_resource_tip_of_not_mem
    (dag : MsgOrd)
    (msg : OrdMsg)
    (key : ResourceKey)
    (hnot : key ∉ msg.gm_resource_keys) :
    tipLookup (publishTips dag msg).tipTable key = tipLookup dag.tipTable key := by
  unfold publishTips
  simpa using
    publishTipTable_preserves_lookup_of_not_mem dag.tipTable msg.gm_id msg.gm_resource_keys key hnot

theorem publishTips_sets_normalized_resource_tip
    (dag : MsgOrd)
    (msg : OrdMsg)
    (spec : MsgOrdSpec)
    (key : ResourceKey)
    (hkey : key ∈ normalizeKeys spec.resourceKeys) :
    tipLookup (publishTips dag (applySpec msg spec)).tipTable key = some msg.gm_id := by
  have hnodup : (applySpec msg spec).gm_resource_keys.Nodup := by
    simpa [applySpec] using normalizeKeys_nodup spec.resourceKeys
  have hkey' : key ∈ (applySpec msg spec).gm_resource_keys := by
    simpa [applySpec] using hkey
  simpa [applySpec] using
    publishTips_sets_resource_tip dag (applySpec msg spec) key hkey' hnodup

theorem classifyColor_blue_iff (dag : MsgOrd) (msg : OrdMsg) :
    classifyColor dag msg = Color.blue ↔ anticone dag msg ≤ dag.kParam := by
  unfold classifyColor
  by_cases h : anticone dag msg ≤ dag.kParam
  · simp [h]
  · simp [h]

theorem classifyColor_red_iff (dag : MsgOrd) (msg : OrdMsg) :
    classifyColor dag msg = Color.red ↔ dag.kParam < anticone dag msg := by
  constructor
  · intro hred
    have hnot : ¬ anticone dag msg ≤ dag.kParam := by
      intro hle
      simp [classifyColor, hle] at hred
    exact Nat.lt_of_not_ge hnot
  · intro hgt
    have hnot : ¬ anticone dag msg ≤ dag.kParam := Nat.not_le_of_gt hgt
    simp [classifyColor, hnot]

theorem assessCandidate_sets_anticone (dag : MsgOrd) (msg : OrdMsg) :
    (assessCandidate dag msg).gm_anticone_size = anticone dag msg := by
  simp [assessCandidate]

theorem assessCandidate_blue_iff (dag : MsgOrd) (msg : OrdMsg) :
    (assessCandidate dag msg).gm_color = Color.blue ↔ anticone dag msg ≤ dag.kParam := by
  unfold assessCandidate
  by_cases h : anticone dag msg ≤ dag.kParam
  · simp [h]
  · simp [h]

theorem assessCandidate_red_iff (dag : MsgOrd) (msg : OrdMsg) :
    (assessCandidate dag msg).gm_color = Color.red ↔ dag.kParam < anticone dag msg := by
  constructor
  · intro hred
    have hnot : ¬ anticone dag msg ≤ dag.kParam := by
      intro hle
      simp [assessCandidate, hle] at hred
    exact Nat.lt_of_not_ge hnot
  · intro hgt
    have hnot : ¬ anticone dag msg ≤ dag.kParam := Nat.not_le_of_gt hgt
    simp [assessCandidate, hnot]

theorem acceptCandidate_blue_eq (dag : MsgOrd) (candidate : OrdMsg)
    (hblue : candidate.gm_color = Color.blue) :
    acceptCandidate dag candidate = enqueueMsg (publishTips dag candidate) candidate := by
  simp [acceptCandidate, hblue]

theorem acceptCandidate_red_eq (dag : MsgOrd) (candidate : OrdMsg)
    (hred : candidate.gm_color = Color.red) :
    acceptCandidate dag candidate = dag := by
  simp [acceptCandidate, hred]

theorem acceptCandidate_blue_contains (dag : MsgOrd) (candidate : OrdMsg)
    (hblue : candidate.gm_color = Color.blue) :
    candidate ∈ (acceptCandidate dag candidate).msgs := by
  simp [acceptCandidate, hblue, enqueueMsg]

theorem acceptCandidate_blue_mem_of_mem (dag : MsgOrd) (candidate existing : OrdMsg)
    (hblue : candidate.gm_color = Color.blue)
    (hmem : existing ∈ dag.msgs) :
    existing ∈ (acceptCandidate dag candidate).msgs := by
  simp [acceptCandidate, hblue, enqueueMsg, publishTips, hmem]

theorem acceptCandidate_blue_sets_globalTip (dag : MsgOrd) (candidate : OrdMsg)
    (hblue : candidate.gm_color = Color.blue) :
    (acceptCandidate dag candidate).globalTip = some candidate.gm_id := by
  simp [acceptCandidate, hblue, enqueueMsg, publishTips]

theorem acceptCandidate_blue_sets_barrierTip_of_barrier (dag : MsgOrd) (candidate : OrdMsg)
    (hblue : candidate.gm_color = Color.blue)
    (hbar : candidate.gm_resource_keys = []) :
    (acceptCandidate dag candidate).barrierTip = some candidate.gm_id := by
  simp [acceptCandidate, hblue, enqueueMsg, publishTips, hbar]

theorem acceptCandidate_blue_preserves_barrierTip_of_nonbarrier (dag : MsgOrd) (candidate : OrdMsg)
    (hblue : candidate.gm_color = Color.blue)
    (hbar : candidate.gm_resource_keys ≠ []) :
    (acceptCandidate dag candidate).barrierTip = dag.barrierTip := by
  simp [acceptCandidate, hblue, enqueueMsg]
  exact publishTips_preserves_barrierTip_of_nonbarrier dag candidate hbar

theorem acceptCandidate_blue_sets_resource_tip
    (dag : MsgOrd)
    (candidate : OrdMsg)
    (key : ResourceKey)
    (hblue : candidate.gm_color = Color.blue)
    (hkey : key ∈ candidate.gm_resource_keys)
    (hnodup : candidate.gm_resource_keys.Nodup) :
    tipLookup (acceptCandidate dag candidate).tipTable key = some candidate.gm_id := by
  simp [acceptCandidate, hblue, enqueueMsg]
  exact publishTips_sets_resource_tip dag candidate key hkey hnodup

theorem acceptCandidate_blue_preserves_other_tip
    (dag : MsgOrd)
    (candidate : OrdMsg)
    (key : ResourceKey)
    (hblue : candidate.gm_color = Color.blue)
    (hnot : key ∉ candidate.gm_resource_keys) :
    tipLookup (acceptCandidate dag candidate).tipTable key = tipLookup dag.tipTable key := by
  simp [acceptCandidate, hblue, enqueueMsg]
  exact publishTips_preserves_resource_tip_of_not_mem dag candidate key hnot

theorem acceptCandidate_red_preserves_msgs (dag : MsgOrd) (candidate : OrdMsg)
    (hred : candidate.gm_color = Color.red) :
    (acceptCandidate dag candidate).msgs = dag.msgs := by
  simp [acceptCandidate, hred]

theorem acceptCandidate_red_preserves_globalTip (dag : MsgOrd) (candidate : OrdMsg)
    (hred : candidate.gm_color = Color.red) :
    (acceptCandidate dag candidate).globalTip = dag.globalTip := by
  simp [acceptCandidate, hred]

theorem acceptCandidate_red_preserves_barrierTip (dag : MsgOrd) (candidate : OrdMsg)
    (hred : candidate.gm_color = Color.red) :
    (acceptCandidate dag candidate).barrierTip = dag.barrierTip := by
  simp [acceptCandidate, hred]

theorem acceptCandidate_red_preserves_tipTable (dag : MsgOrd) (candidate : OrdMsg)
    (hred : candidate.gm_color = Color.red) :
    (acceptCandidate dag candidate).tipTable = dag.tipTable := by
  simp [acceptCandidate, hred]

theorem acceptCandidate_blue_preserves_blueOnly (dag : MsgOrd) (candidate : OrdMsg)
    (hinv : Inv_BlueOnly dag)
    (hblue : candidate.gm_color = Color.blue) :
    Inv_BlueOnly (acceptCandidate dag candidate) := by
  rw [acceptCandidate_blue_eq dag candidate hblue]
  have hpub : Inv_BlueOnly (publishTips dag candidate) :=
    publishTips_preserves_blueOnly dag candidate hinv
  exact enqueueMsg_preserves_blueOnly (publishTips dag candidate) candidate hpub hblue

theorem acceptCandidate_red_preserves_blueOnly (dag : MsgOrd) (candidate : OrdMsg)
    (hinv : Inv_BlueOnly dag)
    (hred : candidate.gm_color = Color.red) :
    Inv_BlueOnly (acceptCandidate dag candidate) := by
  simpa [acceptCandidate, hred] using hinv

theorem submitCandidate_snd_eq_prepare (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec) :
    (submitCandidate dag msg spec).2 = prepareCandidate dag msg spec := by
  simp [submitCandidate]

theorem submitCandidate_blue_fst_eq (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hblue : (prepareCandidate dag msg spec).gm_color = Color.blue) :
    (submitCandidate dag msg spec).1 =
      enqueueMsg (publishTips dag (prepareCandidate dag msg spec)) (prepareCandidate dag msg spec) := by
  simp [submitCandidate, acceptCandidate, hblue]

theorem submitCandidate_red_fst_eq (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hred : (prepareCandidate dag msg spec).gm_color = Color.red) :
    (submitCandidate dag msg spec).1 = dag := by
  simp [submitCandidate, acceptCandidate, hred]

theorem submitCandidate_blue_contains (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hblue : (prepareCandidate dag msg spec).gm_color = Color.blue) :
    (prepareCandidate dag msg spec) ∈ (submitCandidate dag msg spec).1.msgs := by
  simp [submitCandidate, acceptCandidate, hblue, enqueueMsg]

theorem submitCandidate_blue_mem_of_mem (dag : MsgOrd) (msg existing : OrdMsg) (spec : MsgOrdSpec)
    (hblue : (prepareCandidate dag msg spec).gm_color = Color.blue)
    (hmem : existing ∈ dag.msgs) :
    existing ∈ (submitCandidate dag msg spec).1.msgs := by
  simp [submitCandidate, acceptCandidate, hblue, enqueueMsg, publishTips, hmem]

theorem submitCandidate_blue_sets_globalTip (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hblue : (prepareCandidate dag msg spec).gm_color = Color.blue) :
    (submitCandidate dag msg spec).1.globalTip = some (prepareCandidate dag msg spec).gm_id := by
  simp [submitCandidate, acceptCandidate, hblue, enqueueMsg, publishTips]

theorem submitCandidate_blue_sets_barrierTip_of_barrier (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hblue : (prepareCandidate dag msg spec).gm_color = Color.blue)
    (hbar : (prepareCandidate dag msg spec).gm_resource_keys = []) :
    (submitCandidate dag msg spec).1.barrierTip = some (prepareCandidate dag msg spec).gm_id := by
  simp [submitCandidate, acceptCandidate, hblue, enqueueMsg, publishTips, hbar]

theorem submitCandidate_blue_preserves_barrierTip_of_nonbarrier (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hblue : (prepareCandidate dag msg spec).gm_color = Color.blue)
    (hbar : (prepareCandidate dag msg spec).gm_resource_keys ≠ []) :
    (submitCandidate dag msg spec).1.barrierTip = dag.barrierTip := by
  simp [submitCandidate, acceptCandidate, hblue, enqueueMsg]
  exact publishTips_preserves_barrierTip_of_nonbarrier dag (prepareCandidate dag msg spec) hbar

theorem submitCandidate_blue_sets_resource_tip
    (dag : MsgOrd)
    (msg : OrdMsg)
    (spec : MsgOrdSpec)
    (key : ResourceKey)
    (hblue : (prepareCandidate dag msg spec).gm_color = Color.blue)
    (hkey : key ∈ normalizeKeys spec.resourceKeys) :
    tipLookup (submitCandidate dag msg spec).1.tipTable key =
      some (prepareCandidate dag msg spec).gm_id := by
  have hnodup : (prepareCandidate dag msg spec).gm_resource_keys.Nodup :=
    prepareCandidate_nodup_keys dag msg spec
  have hkey' : key ∈ (prepareCandidate dag msg spec).gm_resource_keys := by
    simpa [prepareCandidate_uses_normalizedKeys dag msg spec] using hkey
  simp [submitCandidate, acceptCandidate, hblue, enqueueMsg]
  exact publishTips_sets_resource_tip dag (prepareCandidate dag msg spec) key hkey' hnodup

theorem submitCandidate_blue_preserves_other_tip
    (dag : MsgOrd)
    (msg : OrdMsg)
    (spec : MsgOrdSpec)
    (key : ResourceKey)
    (hblue : (prepareCandidate dag msg spec).gm_color = Color.blue)
    (hnot : key ∉ (prepareCandidate dag msg spec).gm_resource_keys) :
    tipLookup (submitCandidate dag msg spec).1.tipTable key = tipLookup dag.tipTable key := by
  simp [submitCandidate, acceptCandidate, hblue, enqueueMsg]
  exact publishTips_preserves_resource_tip_of_not_mem dag (prepareCandidate dag msg spec) key hnot

theorem submitCandidate_red_preserves_dag (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hred : (prepareCandidate dag msg spec).gm_color = Color.red) :
    (submitCandidate dag msg spec).1 = dag := by
  exact submitCandidate_red_fst_eq dag msg spec hred

theorem submitCandidate_red_preserves_msgs (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hred : (prepareCandidate dag msg spec).gm_color = Color.red) :
    (submitCandidate dag msg spec).1.msgs = dag.msgs := by
  simp [submitCandidate, acceptCandidate, hred]

theorem submitCandidate_red_preserves_globalTip (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hred : (prepareCandidate dag msg spec).gm_color = Color.red) :
    (submitCandidate dag msg spec).1.globalTip = dag.globalTip := by
  simp [submitCandidate, acceptCandidate, hred]

theorem submitCandidate_red_preserves_barrierTip (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hred : (prepareCandidate dag msg spec).gm_color = Color.red) :
    (submitCandidate dag msg spec).1.barrierTip = dag.barrierTip := by
  simp [submitCandidate, acceptCandidate, hred]

theorem submitCandidate_red_preserves_tipTable (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hred : (prepareCandidate dag msg spec).gm_color = Color.red) :
    (submitCandidate dag msg spec).1.tipTable = dag.tipTable := by
  simp [submitCandidate, acceptCandidate, hred]

theorem submitCandidate_blue_preserves_blueOnly (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hinv : Inv_BlueOnly dag)
    (hblue : (prepareCandidate dag msg spec).gm_color = Color.blue) :
    Inv_BlueOnly (submitCandidate dag msg spec).1 := by
  simp [submitCandidate, acceptCandidate, hblue]
  have hpub : Inv_BlueOnly (publishTips dag (prepareCandidate dag msg spec)) :=
    publishTips_preserves_blueOnly dag (prepareCandidate dag msg spec) hinv
  exact enqueueMsg_preserves_blueOnly
    (publishTips dag (prepareCandidate dag msg spec))
    (prepareCandidate dag msg spec)
    hpub hblue

theorem submitCandidate_red_preserves_blueOnly (dag : MsgOrd) (msg : OrdMsg) (spec : MsgOrdSpec)
    (hinv : Inv_BlueOnly dag)
    (hred : (prepareCandidate dag msg spec).gm_color = Color.red) :
    Inv_BlueOnly (submitCandidate dag msg spec).1 := by
  simp [submitCandidate, acceptCandidate, hred, hinv]

theorem canDeliver_requiresBlue (dag : MsgOrd) (msg : OrdMsg)
    (h : canDeliver dag msg) : msg.gm_color = Color.blue := by
  exact h.1

theorem canDeliver_parent_closed (dag : MsgOrd) (msg : OrdMsg)
    (h : canDeliver dag msg) :
    ∀ parentId, parentId ∈ msg.gm_parents → parentReady dag parentId := by
  exact h.2

theorem invBlueOnly_empty : Inv_BlueOnly {} := by
  intro msg hmem
  cases hmem

end MsgOrd
end LeanProofs
