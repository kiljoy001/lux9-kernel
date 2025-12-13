#!/bin/bash

# Run the ECMA-335 documentation indexing script using pipenv
cd /home/scott/Repo/lux9-kernel/kernel/clr/CIL-Interpreter/tools
pipenv run python3 -c "
import json
import subprocess
import sys
import os
import re

def extract_pdf_text_pypdf2(pdf_path):
    try:
        import PyPDF2
        with open(pdf_path, 'rb') as file:
            pdf_reader = PyPDF2.PdfReader(file)
            text = ''
            # Extract text from first 30 pages to avoid memory issues
            for page_num, page in enumerate(pdf_reader.pages[:30]):
                try:
                    page_text = page.extract_text()
                    if page_text.strip():
                        text += f'\\n[PAGE {page_num + 1}]\\n{page_text}\\n'
                except Exception as e:
                    print(f'Warning: Could not extract text from page {page_num + 1}: {e}')
                    continue
            return text
    except Exception as e:
        print(f'Error reading PDF: {e}')
        return None

def parse_sections(text):
    sections = []
    if not text:
        return sections
    
    # Simple approach: split into chunks
    chunk_size = 1500
    for i in range(0, min(len(text), 30000), chunk_size):
        chunk = text[i:i+chunk_size]
        if len(chunk.strip()) > 50:
            sections.append({
                'id': f'ecma335_doc_chunk_{i//chunk_size}',
                'opcode_name': f'doc_chunk_{i//chunk_size}',  # Using available field
                'opcode_value': 1000 + (i//chunk_size),       # Using available field
                'opcode_family': 'documentation',             # Using available field
                'description': f'ECMA-335 Documentation Chunk {i//chunk_size}',
                'specification': chunk.strip()[:1500],
                'content': chunk.strip()[:1500],              # This will be ignored but included for completeness
                'page_number': 0,
                'chapter': 'ECMA-335 Specification',
                'type': 'documentation'
            })
    return sections

def index_documentation(sections):
    if not sections:
        print('No sections to index')
        return False
    
    temp_file = '/tmp/ecma335_doc_chunks.json'
    with open(temp_file, 'w') as f:
        json.dump(sections, f, indent=2)
    
    try:
        subprocess.run([
            'curl', '-s', '-X', 'POST',
            'http://localhost:8983/solr/ecma335_standards/update?commit=true',
            '-H', 'Content-Type: application/json',
            '--data-binary', '@' + temp_file
        ], check=True)
        
        print(f'Successfully indexed {len(sections)} documentation chunks into Solr')
        return True
    except subprocess.CalledProcessError as e:
        print(f'Error indexing documentation: {e}')
        return False
    finally:
        if os.path.exists(temp_file):
            os.remove(temp_file)

def main():
    pdf_path = '/home/scott/Repo/lux9-kernel/ECMA-335_6th_edition_june_2012.pdf'
    
    print('Extracting text from ECMA-335 PDF...')
    text = extract_pdf_text_pypdf2(pdf_path)
    
    if not text:
        print('Failed to extract text from PDF')
        return 1
    
    print(f'Extracted {len(text)} characters from PDF')
    
    if len(text) > 50000:
        text = text[:50000]
        print('Limited text to first 50,000 characters')
    
    print('Creating document chunks...')
    sections = parse_sections(text)
    
    print(f'Created {len(sections)} document chunks')
    
    if not sections:
        print('No sections created')
        return 1
    
    print('Indexing documentation chunks into Solr...')
    if index_documentation(sections):
        print('Documentation indexing completed!')
        return 0
    else:
        print('Failed to index documentation')
        return 1

if __name__ == '__main__':
    sys.exit(main())
"