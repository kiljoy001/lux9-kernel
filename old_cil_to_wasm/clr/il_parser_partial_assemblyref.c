assemblyref_row_t *il_get_assemblyref(il_assembly_t *assembly, uint32_t rid) {
  if (rid == 0 || rid > assembly->tables_header.row_counts[TABLE_ASSEMBLYREF]) {
    return NULL;
  }

  /* Cache check (TODO: add cache if needed, for now re-read) */
  /* We don't have a cache for AssemblyRef yet in the struct, so just allocating
   * one off */
  /* Actually, let's look at how other getters work. They usually return a
     pointer to a struct that is cached in the assembly object. We need to add
     assemblyrefs array to il_assembly_t in the header first if we want caching.
     Checking il_assembly_t definition in il_parser.h...
     It has methods, typerefs, typedefs... but NO assemblyrefs.
     I should add it to the struct or just return a static/malloced one.
     The other getters return pointers to cached arrays.
     I will implement it to return a static thread-local or just malloc for now,
     OR I can update the struct.
     Let's check memberref_row_t *il_get_memberref. It likely uses the cached
     array.

     Wait, I missed adding `assemblyref_row_t *assemblyrefs` to `il_assembly_t`
     in `il_parser.h`. I should probably do that for consistency, but to avoid
     replacing the whole struct again, I will implement a direct reader that
     reads from `tables_data`.
  */

  uint8_t *ptr = il_get_table_start(assembly, TABLE_ASSEMBLYREF);
  if (!ptr)
    return NULL;

  uint32_t row_size = get_table_row_size(assembly, TABLE_ASSEMBLYREF);
  ptr += (rid - 1) * row_size;

  /* Allocate a temp row to return (caller shouldn't free if it expects a cache
     pointer... but here we are stuck. Let's make it static for now or just
     malloc and leak/exepct free? Better: implementation detail - we can cast
     the raw data if it matched, but it doesn't match struct layout (compressed
     ints vs native). So we must parse.
  */
  assemblyref_row_t *row = IL_MALLOC(sizeof(assemblyref_row_t));
  if (!row)
    return NULL;

  row->major_version = READ_UINT16(ptr);
  ptr += 2;
  row->minor_version = READ_UINT16(ptr);
  ptr += 2;
  row->build_number = READ_UINT16(ptr);
  ptr += 2;
  row->revision_number = READ_UINT16(ptr);
  ptr += 2;
  row->flags = READ_UINT32(ptr);
  ptr += 4;

  int blob_wide = (assembly->tables_header.heap_sizes & 0x04) != 0;
  int string_wide = (assembly->tables_header.heap_sizes & 0x01) != 0;

  if (blob_wide) {
    row->public_key_or_token = READ_UINT32(ptr);
    ptr += 4;
  } else {
    row->public_key_or_token = READ_UINT16(ptr);
    ptr += 2;
  }

  if (string_wide) {
    row->name_index = READ_UINT32(ptr);
    ptr += 4;
  } else {
    row->name_index = READ_UINT16(ptr);
    ptr += 2;
  }

  if (string_wide) {
    row->culture_index = READ_UINT32(ptr);
    ptr += 4;
  } else {
    row->culture_index = READ_UINT16(ptr);
    ptr += 2;
  }

  if (blob_wide) {
    row->hash_value = READ_UINT32(ptr);
    ptr += 4;
  } else {
    row->hash_value = READ_UINT16(ptr);
    ptr += 2;
  }

  return row;
}
