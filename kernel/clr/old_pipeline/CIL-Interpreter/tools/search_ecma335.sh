#!/bin/bash

# Search ECMA-335 CIL Standards Documentation
# Usage: ./search_ecma335.sh <search_term> [fields]

SOLR_URL="http://localhost:8983/solr/ecma335_standards"

usage() {
    echo "Usage: $0 <search_term> [fields]"
    echo "  search_term: Term to search for in ECMA-335 documentation"
    echo "  fields: Comma-separated list of fields to return (optional)"
    echo ""
    echo "Examples:"
    echo "  $0 \"add instruction\""
    echo "  $0 \"stack behavior\" \"opcode_name,specification,stack_behavior\""
    echo "  $0 \"overflow\" \"opcode_name,exception_behavior\""
    echo ""
    echo "Available fields:"
    echo "  opcode_name, opcode_value, opcode_family, specification, description,"
    echo "  syntax, stack_behavior, operand_types, return_type, exception_behavior,"
    echo "  validation_rules, ecma335_section, compliance_notes, related_opcodes,"
    echo "  see_also, chapter, page_number"
}

if [ $# -eq 0 ]; then
    usage
    exit 1
fi

SEARCH_TERM="$1"
FIELDS="${2:-opcode_name,opcode_value,specification,description}"

# URL encode the search term
ENCODED_TERM=$(echo "$SEARCH_TERM" | sed 's/ /%20/g')

# Perform the search
curl -s "${SOLR_URL}/select?q=${ENCODED_TERM}&fl=${FIELDS}&wt=json" | \
jq -r '.response.docs[] | "=== \(.opcode_name) (0x\(.opcode_value | tostring | ltrimstr("0")) \(.opcode_value)) ===\nSection: \(.ecma335_section)\nDescription: \(.description)\nSpecification: \(.specification)\nStack: \(.stack_behavior)\nRelated: \(.related_opcodes[]?)\n\n"'