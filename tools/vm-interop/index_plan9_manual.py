#!/usr/bin/env python3
"""
Index Plan 9 Programmer's Manual into Solr for semantic search
"""

import json
import hashlib
import subprocess
import requests
from pathlib import Path

SOLR_URL = "http://localhost:8983/solr"
CORE_NAME = "plan9"  # Using existing plan9 core
PDF_PATH = "/home/scott/Repo/VM-Interop/plan9_programmers_value.pdf"
CHUNK_SIZE = 2000  # Characters per chunk for better search

def extract_pdf_text(pdf_path):
    """Extract text from PDF using pdftotext"""
    try:
        result = subprocess.run(
            ["pdftotext", "-layout", pdf_path, "-"],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Error extracting PDF: {e}")
        return None

def chunk_text(text, chunk_size=CHUNK_SIZE):
    """Split text into overlapping chunks for better search"""
    chunks = []
    lines = text.split('\n')
    current_chunk = []
    current_size = 0
    
    for line in lines:
        line_size = len(line)
        if current_size + line_size > chunk_size and current_chunk:
            # Save current chunk
            chunk_text = '\n'.join(current_chunk)
            chunks.append(chunk_text)
            # Keep last 20% for overlap
            overlap_lines = current_chunk[-(len(current_chunk)//5):]
            current_chunk = overlap_lines + [line]
            current_size = sum(len(l) for l in current_chunk)
        else:
            current_chunk.append(line)
            current_size += line_size
    
    # Don't forget the last chunk
    if current_chunk:
        chunks.append('\n'.join(current_chunk))
    
    return chunks

def create_document(chunk_text, chunk_id, page_estimate):
    """Create a Solr document from a text chunk"""
    # Generate unique ID with timestamp to avoid collisions
    import time
    timestamp = int(time.time() * 1000000)  # Microsecond precision
    doc_id = f"plan9_manual_{timestamp}_{chunk_id}"
    
    # Extract section/chapter info if present
    section = ""
    chapter = ""
    lines = chunk_text.split('\n')
    for line in lines[:10]:  # Check first 10 lines for headers
        if 'SECTION' in line.upper() or 'CHAPTER' in line.upper():
            section = line.strip()
            break
        if line.strip() and len(line.strip()) < 50 and line.isupper():
            chapter = line.strip()
            break
    
    doc = {
        "id": doc_id,
        "content": [chunk_text],  # Content as list per schema
        "file_path": PDF_PATH,
        "file_type": "pdf",
        "type": "manual",
        "page": page_estimate,
    }
    
    # Add optional fields only if they have values
    if section:
        doc["section"] = section
    if chapter:
        doc["chapter"] = chapter
    
    return doc

def index_to_solr(documents):
    """Send documents to Solr for indexing"""
    url = f"{SOLR_URL}/{CORE_NAME}/update/json"
    headers = {"Content-Type": "application/json"}
    
    # Index in batches
    batch_size = 50
    for i in range(0, len(documents), batch_size):
        batch = documents[i:i+batch_size]
        data = json.dumps(batch)
        
        try:
            response = requests.post(url, headers=headers, data=data)
            response.raise_for_status()
            print(f"Indexed batch {i//batch_size + 1} ({len(batch)} documents)")
        except requests.exceptions.RequestException as e:
            print(f"Error indexing batch: {e}")
            if response.text:
                print(f"Solr error response: {response.text[:1000]}")
            # Try indexing documents one by one to find the problematic one
            if len(batch) > 1:
                print("Trying to index documents individually to find the error...")
                for j, doc in enumerate(batch):
                    try:
                        single_response = requests.post(url, headers=headers, data=json.dumps([doc]))
                        single_response.raise_for_status()
                    except Exception as single_e:
                        print(f"Document {i+j} failed: {single_e}")
                        print(f"Document content preview: {str(doc)[:500]}")
                        break
            return False
    
    # Commit changes
    commit_url = f"{SOLR_URL}/{CORE_NAME}/update?commit=true"
    try:
        response = requests.get(commit_url)
        response.raise_for_status()
        print("Successfully committed changes to Solr")
        return True
    except requests.exceptions.RequestException as e:
        print(f"Error committing: {e}")
        return False

def main():
    print(f"Indexing Plan 9 Programmer's Manual from {PDF_PATH}")
    
    # Extract text
    print("Extracting text from PDF...")
    text = extract_pdf_text(PDF_PATH)
    if not text:
        print("Failed to extract text from PDF")
        return
    
    print(f"Extracted {len(text)} characters")
    
    # Chunk the text
    print("Chunking text for indexing...")
    chunks = chunk_text(text)
    print(f"Created {len(chunks)} chunks")
    
    # Create Solr documents
    print("Creating Solr documents...")
    documents = []
    chars_per_page = 3000  # Rough estimate
    
    for i, chunk in enumerate(chunks):
        page_estimate = (i * CHUNK_SIZE) // chars_per_page + 1
        doc = create_document(chunk, i, page_estimate)
        documents.append(doc)
    
    print(f"Created {len(documents)} documents")
    
    # Index to Solr
    print("Indexing to Solr...")
    if index_to_solr(documents):
        print(f"Successfully indexed {len(documents)} chunks from Plan 9 Programmer's Manual")
        
        # Test search
        print("\nTesting search for '9p synthetic'...")
        test_url = f"{SOLR_URL}/{CORE_NAME}/select?q=content:9p+synthetic&rows=2&wt=json"
        try:
            response = requests.get(test_url)
            data = response.json()
            print(f"Found {data['response']['numFound']} matching documents")
        except:
            pass
    else:
        print("Indexing failed")

if __name__ == "__main__":
    main()