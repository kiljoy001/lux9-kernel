/* Fruity IR CBOR Serialization
 *
 * Simplified implementation for kernel - handles core structures
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "fruity_ir.h"

/* Forward declarations */
extern char* strdup(char*);

/* CBOR library setup */
#define CBOR_KERNEL_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;
typedef uintptr uintptr_t;
typedef intptr intptr_t;
typedef usize size_t;
typedef int bool;
#define true 1
#define false 0
#ifndef NULL
#define NULL nil
#endif

#include "cbor/parser.h"
#include "cbor/decoder.h"
#include "cbor/encoder.h"

/* Maximum items for CBOR parsing */
#define MAX_CBOR_ITEMS 1024

/*
 * Serialize Fruity module to CBOR
 *
 * Format:
 *   {
 *     "name": string,
 *     "version": uint,
 *     "functions": [
 *       {
 *         "name": string,
 *         "blocks": [
 *           {
 *             "id": uint,
 *             "instructions": [
 *               { "opcode": uint, ... }
 *             ]
 *           }
 *         ]
 *       }
 *     ]
 *   }
 */
ulong
fruity_module_to_cbor(fruity_module_t *module, u8int *buf, ulong bufsize, char *errbuf, ulong errbuf_size)
{
	cbor_writer_t writer;
	fruity_function_t *func;
	fruity_basic_block_t *bb;
	fruity_instruction_t *instr;
	ulong func_count, bb_count, instr_count;

	if(module == nil || buf == nil){
		if(errbuf && errbuf_size > 0)
			snprint(errbuf, errbuf_size, "null parameters");
		return 0;
	}

	cbor_writer_init(&writer, buf, bufsize);

	/* Start root map */
	cbor_encode_map(&writer, 3);  /* 3 fields: name, version, functions */

	/* "name": string */
	cbor_encode_text_string(&writer, "name", 4);
	if(module->name)
		cbor_encode_text_string(&writer, module->name, strlen(module->name));
	else
		cbor_encode_text_string(&writer, "module", 6);

	/* "version": uint */
	cbor_encode_text_string(&writer, "version", 7);
	cbor_encode_unsigned_integer(&writer, module->version);

	/* "functions": array */
	cbor_encode_text_string(&writer, "functions", 9);

	/* Count functions */
	func_count = 0;
	for(func = module->functions_head; func != nil; func = func->next)
		func_count++;

	cbor_encode_array(&writer, func_count);

	/* Encode each function */
	for(func = module->functions_head; func != nil; func = func->next){
		cbor_encode_map(&writer, 2);  /* name, blocks */

		/* Function name */
		cbor_encode_text_string(&writer, "name", 4);
		if(func->name)
			cbor_encode_text_string(&writer, func->name, strlen(func->name));
		else
			cbor_encode_text_string(&writer, "func", 4);

		/* Blocks array */
		cbor_encode_text_string(&writer, "blocks", 6);

		/* Count blocks */
		bb_count = 0;
		for(bb = func->blocks_head; bb != nil; bb = bb->next)
			bb_count++;

		cbor_encode_array(&writer, bb_count);

		/* Encode each block */
		for(bb = func->blocks_head; bb != nil; bb = bb->next){
			cbor_encode_map(&writer, 2);  /* id, instructions */

			/* Block ID */
			cbor_encode_text_string(&writer, "id", 2);
			cbor_encode_unsigned_integer(&writer, bb->block_id);

			/* Instructions array */
			cbor_encode_text_string(&writer, "instructions", 12);

			/* Count instructions */
			instr_count = 0;
			for(instr = bb->instructions_head; instr != nil; instr = instr->next)
				instr_count++;

			cbor_encode_array(&writer, instr_count);

			/* Encode each instruction */
			for(instr = bb->instructions_head; instr != nil; instr = instr->next){
				cbor_encode_map(&writer, 1);  /* Just opcode for now */
				cbor_encode_text_string(&writer, "opcode", 6);
				cbor_encode_unsigned_integer(&writer, instr->opcode);
			}
		}
	}

	return cbor_writer_len(&writer);
}

/*
 * Deserialize CBOR to Fruity module
 *
 * For now, just create a minimal stub module
 */
fruity_module_t*
fruity_module_from_cbor(u8int *data, ulong datalen, char *errbuf, ulong errbuf_size)
{
	cbor_reader_t reader;
	cbor_item_t items[MAX_CBOR_ITEMS];
	fruity_module_t *module;
	cbor_error_t err;
	usize nitems;

	if(data == nil){
		if(errbuf && errbuf_size > 0)
			snprint(errbuf, errbuf_size, "null data");
		return nil;
	}

	/* Initialize reader */
	cbor_reader_init(&reader, items, MAX_CBOR_ITEMS);

	/* Parse CBOR */
	err = cbor_parse(&reader, data, datalen, &nitems);
	if(err != CBOR_SUCCESS){
		if(errbuf && errbuf_size > 0)
			snprint(errbuf, errbuf_size, "CBOR parse error: %d", err);
		return nil;
	}

	/* Allocate module */
	module = mallocz(sizeof(fruity_module_t), 1);
	if(module == nil){
		if(errbuf && errbuf_size > 0)
			snprint(errbuf, errbuf_size, "allocation failed");
		return nil;
	}

	/* Parse CBOR structure and reconstruct Fruity IR */
	if(nitems < 1 || items[0].type != CBOR_ITEM_MAP){
		if(errbuf && errbuf_size > 0)
			snprint(errbuf, errbuf_size, "root must be map");
		free(module);
		return nil;
	}

	/* Navigate root map to find name, version, functions */
	ulong idx, name_idx, version_idx, functions_idx;
	char name_buf[256], key_buf[64];
	int found_name, found_version, found_functions;

	name_idx = version_idx = functions_idx = 0;
	found_name = found_version = found_functions = 0;

	/* Root map has size pairs (key, value) */
	ulong map_pairs = items[0].size;
	idx = 1; /* Start after root map item */

	for(ulong pair = 0; pair < map_pairs && idx + 1 < nitems; pair++){
		cbor_item_t *key = &items[idx];
		cbor_item_t *val = &items[idx + 1];

		/* Decode key string */
		if(key->type != CBOR_ITEM_STRING){
			idx += 2;
			continue;
		}

		memset(key_buf, 0, sizeof(key_buf));
		cbor_decode(&reader, key, key_buf, sizeof(key_buf) - 1);

		if(strcmp(key_buf, "name") == 0){
			name_idx = idx + 1;
			found_name = 1;
		}else if(strcmp(key_buf, "version") == 0){
			version_idx = idx + 1;
			found_version = 1;
		}else if(strcmp(key_buf, "functions") == 0){
			functions_idx = idx + 1;
			found_functions = 1;
		}

		idx += 2; /* Skip key+value pair */
	}

	if(!found_name || !found_version || !found_functions){
		if(errbuf && errbuf_size > 0)
			snprint(errbuf, errbuf_size, "missing required fields");
		free(module);
		return nil;
	}

	/* Extract name */
	memset(name_buf, 0, sizeof(name_buf));
	if(cbor_decode(&reader, &items[name_idx], name_buf, sizeof(name_buf) - 1) != CBOR_SUCCESS){
		if(errbuf && errbuf_size > 0)
			snprint(errbuf, errbuf_size, "failed to decode name");
		free(module);
		return nil;
	}
	module->name = strdup(name_buf);

	/* Extract version */
	u64int version_val;
	if(cbor_decode(&reader, &items[version_idx], &version_val, sizeof(version_val)) != CBOR_SUCCESS){
		if(errbuf && errbuf_size > 0)
			snprint(errbuf, errbuf_size, "failed to decode version");
		free(module->name);
		free(module);
		return nil;
	}
	module->version = (u32int)version_val;

	/* Extract functions array */
	if(items[functions_idx].type != CBOR_ITEM_ARRAY){
		if(errbuf && errbuf_size > 0)
			snprint(errbuf, errbuf_size, "functions must be array");
		free(module->name);
		free(module);
		return nil;
	}

	module->functions_head = nil;
	module->functions_tail = nil;
	module->function_count = 0;

	ulong func_count = items[functions_idx].size;
	idx = functions_idx + 1; /* First function map */

	for(ulong fi = 0; fi < func_count && idx < nitems; fi++){
		cbor_item_t *func_map = &items[idx];
		if(func_map->type != CBOR_ITEM_MAP){
			if(errbuf && errbuf_size > 0)
				snprint(errbuf, errbuf_size, "function must be map");
			/* Best effort: skip to next */
			idx++;
			continue;
		}

		/* Create function */
		fruity_function_t *func = fruity_function_create(nil, 0);
		if(func == nil){
			if(errbuf && errbuf_size > 0)
				snprint(errbuf, errbuf_size, "function allocation failed");
			fruity_module_destroy(module);
			return nil;
		}

		/* Parse function map: name, blocks */
		ulong func_pairs = func_map->size;
		ulong func_idx = idx + 1;
		ulong func_name_idx = 0, func_blocks_idx = 0;
		int found_func_name = 0, found_func_blocks = 0;

		for(ulong fp = 0; fp < func_pairs && func_idx + 1 < nitems; fp++){
			cbor_item_t *fkey = &items[func_idx];
			cbor_item_t *fval = &items[func_idx + 1];

			if(fkey->type != CBOR_ITEM_STRING){
				func_idx += 2;
				continue;
			}

			memset(key_buf, 0, sizeof(key_buf));
			cbor_decode(&reader, fkey, key_buf, sizeof(key_buf) - 1);

			if(strcmp(key_buf, "name") == 0){
				func_name_idx = func_idx + 1;
				found_func_name = 1;
			}else if(strcmp(key_buf, "blocks") == 0){
				func_blocks_idx = func_idx + 1;
				found_func_blocks = 1;
			}

			func_idx += 2;
		}

		if(found_func_name){
			char func_name_buf[256];
			memset(func_name_buf, 0, sizeof(func_name_buf));
			cbor_decode(&reader, &items[func_name_idx], func_name_buf, sizeof(func_name_buf) - 1);
			if(func->name)
				free(func->name);
			func->name = strdup(func_name_buf);
		}

		/* Parse blocks array */
		if(found_func_blocks && items[func_blocks_idx].type == CBOR_ITEM_ARRAY){
			ulong block_count = items[func_blocks_idx].size;
			ulong block_idx = func_blocks_idx + 1;

			for(ulong bi = 0; bi < block_count && block_idx < nitems; bi++){
				cbor_item_t *block_map = &items[block_idx];
				if(block_map->type != CBOR_ITEM_MAP){
					block_idx++;
					continue;
				}

				/* Create basic block */
				fruity_basic_block_t *bb = fruity_basic_block_create(0);
				if(bb == nil){
					fruity_function_destroy(func);
					fruity_module_destroy(module);
					return nil;
				}

				/* Parse block map: id, instructions */
				ulong bb_pairs = block_map->size;
				ulong bb_idx = block_idx + 1;
				ulong bb_id_idx = 0, bb_instrs_idx = 0;
				int found_bb_id = 0, found_bb_instrs = 0;

				for(ulong bp = 0; bp < bb_pairs && bb_idx + 1 < nitems; bp++){
					cbor_item_t *bkey = &items[bb_idx];
					cbor_item_t *bval = &items[bb_idx + 1];

					if(bkey->type != CBOR_ITEM_STRING){
						bb_idx += 2;
						continue;
					}

					memset(key_buf, 0, sizeof(key_buf));
					cbor_decode(&reader, bkey, key_buf, sizeof(key_buf) - 1);

					if(strcmp(key_buf, "id") == 0){
						bb_id_idx = bb_idx + 1;
						found_bb_id = 1;
					}else if(strcmp(key_buf, "instructions") == 0){
						bb_instrs_idx = bb_idx + 1;
						found_bb_instrs = 1;
					}

					bb_idx += 2;
				}

				if(found_bb_id){
					u64int id_val;
					cbor_decode(&reader, &items[bb_id_idx], &id_val, sizeof(id_val));
					bb->block_id = (u32int)id_val;
				}

				/* Parse instructions array */
				if(found_bb_instrs && items[bb_instrs_idx].type == CBOR_ITEM_ARRAY){
					ulong instr_count = items[bb_instrs_idx].size;
					ulong instr_idx = bb_instrs_idx + 1;

					for(ulong ii = 0; ii < instr_count && instr_idx < nitems; ii++){
						cbor_item_t *instr_map = &items[instr_idx];
						if(instr_map->type != CBOR_ITEM_MAP){
							instr_idx++;
							continue;
						}

						/* Parse instruction: opcode */
						ulong instr_pairs = instr_map->size;
						ulong instr_map_idx = instr_idx + 1;
						ulong opcode_idx = 0;
						int found_opcode = 0;

						for(ulong ip = 0; ip < instr_pairs && instr_map_idx + 1 < nitems; ip++){
							cbor_item_t *ikey = &items[instr_map_idx];
							cbor_item_t *ival = &items[instr_map_idx + 1];

							if(ikey->type != CBOR_ITEM_STRING){
								instr_map_idx += 2;
								continue;
							}

							memset(key_buf, 0, sizeof(key_buf));
							cbor_decode(&reader, ikey, key_buf, sizeof(key_buf) - 1);

							if(strcmp(key_buf, "opcode") == 0){
								opcode_idx = instr_map_idx + 1;
								found_opcode = 1;
							}

							instr_map_idx += 2;
						}

						if(found_opcode){
							u64int opcode_val;
							cbor_decode(&reader, &items[opcode_idx], &opcode_val, sizeof(opcode_val));

							fruity_instruction_t *instr = fruity_instruction_create((fruity_opcode_t)opcode_val);
							if(instr != nil){
								fruity_basic_block_add_instruction(bb, instr);
							}
						}

						instr_idx++; /* Next instruction */
					}
				}

				/* Add block to function manually (intrusive list) */
				bb->next = nil;
				bb->prev = func->blocks_tail;

				if(func->blocks_tail != nil)
					func->blocks_tail->next = bb;
				else
					func->blocks_head = bb;

				func->blocks_tail = bb;
				func->block_count++;

				/* First block is entry */
				if(func->entry_block == nil)
					func->entry_block = bb;

				block_idx++; /* Next block */
			}
		}

		/* Add function to module manually (intrusive list) */
		func->next = nil;
		func->prev = module->functions_tail;

		if(module->functions_tail != nil)
			module->functions_tail->next = func;
		else
			module->functions_head = func;

		module->functions_tail = func;
		module->function_count++;

		idx = func_idx; /* Move to next function */
	}

	return module;
}
