#!/usr/bin/env python3
"""
Index 9front source code into Solr plan9 core
Focuses on rc scripts, C code, and system utilities
"""

import json
import requests
import os
import sys
import re
from pathlib import Path

SOLR_URL = 'http://localhost:8983/solr/plan9'
BATCH_SIZE = 50

def is_text_file(filepath):
    """Check if file is likely text based on extension and name"""
    text_extensions = {
        '.c', '.h', '.rc', '.s', '.S', '.asm', '.py', '.pl', '.awk',
        '.sh', '.mk', '.txt', '.md', '.man', '.1', '.2', '.3', '.4',
        '.5', '.6', '.7', '.8', '.9', '.ms', '.ps', '.tex', '.go',
        '.y', '.l', '.cc', '.cpp', '.hpp', '.fs', '.rs', '.ml'
    }
    
    # Check extension
    ext = Path(filepath).suffix
    if ext in text_extensions:
        return True
    
    # Check for specific files without extensions
    name = Path(filepath).name
    if name in ['README', 'INSTALL', 'LICENSE', 'Makefile', 'mkfile', 'CHANGES', 'TODO']:
        return True
    
    # Check for rc scripts (often no extension)
    if '/rc/' in str(filepath) or name.startswith('rc'):
        return True
        
    return False

def extract_code_metadata(content, filepath):
    """Extract metadata from source code"""
    metadata = {
        'file_type': 'unknown',
        'functions': [],
        'includes': [],
        'defines': []
    }
    
    # Determine file type
    if filepath.endswith('.c'):
        metadata['file_type'] = 'c_source'
        # Extract function names
        func_pattern = r'^[a-zA-Z_][a-zA-Z0-9_]*\s*\([^)]*\)\s*\{'
        metadata['functions'] = re.findall(func_pattern, content, re.MULTILINE)[:10]
        # Extract includes
        metadata['includes'] = re.findall(r'#include\s*[<"]([^>"]+)[>"]', content)[:10]
        # Extract defines
        metadata['defines'] = re.findall(r'#define\s+([A-Z_][A-Z0-9_]*)', content)[:10]
    elif filepath.endswith('.h'):
        metadata['file_type'] = 'c_header'
        metadata['defines'] = re.findall(r'#define\s+([A-Z_][A-Z0-9_]*)', content)[:10]
    elif filepath.endswith('.rc') or '/rc/' in filepath:
        metadata['file_type'] = 'rc_script'
        # Extract function definitions in rc
        metadata['functions'] = re.findall(r'^fn\s+([a-zA-Z_][a-zA-Z0-9_]*)', content, re.MULTILINE)[:10]
    elif filepath.endswith('.s') or filepath.endswith('.S'):
        metadata['file_type'] = 'assembly'
    
    return metadata

def index_directory(base_path, path_pattern=""):
    """Recursively index source files from directory"""
    documents = []
    base_path = Path(base_path)
    
    print(f"Scanning {base_path}...")
    
    for root, dirs, files in os.walk(base_path):
        # Skip certain directories
        dirs[:] = [d for d in dirs if d not in ['.git', '.hg', 'obj', 'bin', '386', 'amd64']]
        
        for file in files:
            filepath = os.path.join(root, file)
            
            # Apply path pattern filter if specified
            if path_pattern and path_pattern not in filepath:
                continue
                
            if not is_text_file(filepath):
                continue
            
            try:
                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                
                # Skip binary files that slipped through
                if '\x00' in content:
                    continue
                
                # Extract relative path for better organization
                rel_path = os.path.relpath(filepath, base_path)
                
                # Extract metadata
                metadata = extract_code_metadata(content, filepath)
                
                # Determine chapter/section based on path
                path_parts = rel_path.split('/')
                if len(path_parts) >= 2:
                    chapter = f"9front/{path_parts[0]}"
                    section = '/'.join(path_parts[1:-1]) if len(path_parts) > 2 else path_parts[0]
                else:
                    chapter = "9front"
                    section = path_parts[0] if path_parts else ""
                
                doc = {
                    'id': f'9front_code_{len(documents)}',
                    'chapter': chapter,
                    'section': section,
                    'page': 0,  # Not applicable for code
                    'content': content[:50000],  # Limit content size
                    'type': 'source_code',
                    'file_path': rel_path,
                    'file_type': metadata['file_type']
                }
                
                # Add metadata as searchable text
                if metadata['functions']:
                    doc['content'] += f"\n[FUNCTIONS] {' '.join(metadata['functions'])}"
                if metadata['includes']:
                    doc['content'] += f"\n[INCLUDES] {' '.join(metadata['includes'])}"
                if metadata['defines']:
                    doc['content'] += f"\n[DEFINES] {' '.join(metadata['defines'])}"
                
                documents.append(doc)
                
                if len(documents) % 100 == 0:
                    print(f"  Processed {len(documents)} files...")
                    
            except Exception as e:
                print(f"  Error reading {filepath}: {e}")
                continue
    
    return documents

def index_to_solr(documents):
    """Index documents to Solr"""
    url = f'{SOLR_URL}/update/json/docs'
    headers = {'Content-Type': 'application/json'}
    
    print(f"\nIndexing {len(documents)} documents to Solr...")
    
    for i in range(0, len(documents), BATCH_SIZE):
        batch = documents[i:i+BATCH_SIZE]
        try:
            # Add file_path and file_type fields to schema if needed
            for field in ['file_path', 'file_type']:
                schema_url = f'{SOLR_URL}/schema'
                field_def = {
                    "add-field": {
                        "name": field,
                        "type": "string",
                        "stored": True,
                        "indexed": True
                    }
                }
                requests.post(schema_url, json=field_def)
            
            r = requests.post(url, json=batch, headers=headers)
            if r.status_code == 200:
                print(f"  Indexed batch {i//BATCH_SIZE + 1}: {len(batch)} docs")
            else:
                print(f"  Error indexing batch: {r.status_code} - {r.text[:200]}")
        except Exception as e:
            print(f"  Error indexing: {e}")
    
    # Commit changes
    requests.get(f'{SOLR_URL}/update?commit=true')
    print("Committed changes to Solr")

def search_code_examples():
    """Search for specific code patterns"""
    queries = [
        ('rc while loop', 'file_type:rc_script AND content:while'),
        ('vmx functions', 'file_path:*vmx* AND content:FUNCTIONS'),
        ('9p protocol', 'content:9p AND file_type:c_source'),
        ('kernel init', 'content:init AND file_path:*port*')
    ]
    
    for name, query in queries:
        url = f'{SOLR_URL}/select'
        params = {
            'q': query,
            'fl': 'file_path,chapter,section,content',
            'rows': 2,
            'wt': 'json'
        }
        try:
            r = requests.get(url, params=params)
            results = r.json()
            print(f"\n=== {name} ===")
            print(f"Found: {results['response']['numFound']} matches")
            for doc in results['response']['docs'][:2]:
                print(f"  {doc.get('file_path', 'unknown')}")
        except Exception as e:
            print(f"Search error: {e}")

if __name__ == "__main__":
    # Default to 9front source directory
    source_dir = "/home/scott/Repo/9front"
    
    if len(sys.argv) > 1:
        source_dir = sys.argv[1]
    
    if not os.path.exists(source_dir):
        print(f"Error: {source_dir} does not exist")
        sys.exit(1)
    
    # Index different parts of 9front
    all_docs = []
    
    # Focus on key directories
    key_dirs = [
        ("sys/src/cmd", ""),  # Commands
        ("sys/src/9", ""),    # Kernel
        ("sys/src/lib9p", ""), # 9P library
        ("rc", ""),           # RC scripts
        ("sys/include", ""),  # Headers
    ]
    
    for subdir, pattern in key_dirs:
        full_path = os.path.join(source_dir, subdir)
        if os.path.exists(full_path):
            print(f"\nIndexing {subdir}...")
            docs = index_directory(full_path, pattern)
            all_docs.extend(docs)
            print(f"  Found {len(docs)} files in {subdir}")
    
    if all_docs:
        print(f"\nTotal documents to index: {len(all_docs)}")
        index_to_solr(all_docs)
        
        print("\nTesting searches...")
        search_code_examples()
        
        print("\nDone! You can search with:")
        print("curl 'http://localhost:8983/solr/plan9/select?q=file_type:rc_script&fl=file_path,content&rows=5'")
    else:
        print("No documents found to index")