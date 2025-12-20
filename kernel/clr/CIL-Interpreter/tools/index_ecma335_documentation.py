#!/usr/bin/env python3

import json
import subprocess
import sys
import os
import re

def extract_pdf_text_pypdf2(pdf_path):
    """Extract text from PDF using PyPDF2"""
    try:
        # Try to import PyPDF2, install if not available
        try:
            import PyPDF2
        except ImportError:
            print("PyPDF2 not found, installing...")
            subprocess.check_call([sys.executable, "-m", "pip", "install", "PyPDF2"])
            import PyPDF2
        
        with open(pdf_path, 'rb') as file:
            pdf_reader = PyPDF2.PdfReader(file)
            text = ""
            # Extract text from first 50 pages to avoid memory issues
            for page_num, page in enumerate(pdf_reader.pages[:50]):
                try:
                    page_text = page.extract_text()
                    if page_text.strip():
                        text += f"\n[PAGE {page_num + 1}]\n{page_text}\n"
                except Exception as e:
                    print(f"Warning: Could not extract text from page {page_num + 1}: {e}")
                    continue
            return text
    except Exception as e:
        print(f"Error reading PDF: {e}")
        return None

def parse_chapters_and_sections(text):
    """Parse ECMA-335 documentation into chapters and sections"""
    sections = []
    
    if not text:
        return sections
    
    # Look for section headers in ECMA-335 format
    # Pattern matches section numbers like "III.1", "1.1", "23.4.5", etc.
    section_pattern = r'\b([IVXLC]+\.[0-9]+(?:\.[0-9]+)*)\s+([^\n.]+(?:\.[^\n.]+)*)\.\s*([^\n]+)'
    section_matches = list(re.finditer(section_pattern, text, re.MULTILINE))
    
    print(f"Found {len(section_matches)} potential sections")
    
    # Process a reasonable number of matches to avoid overwhelming Solr
    for i, match in enumerate(section_matches[:50]):  # Limit to first 50 sections
        section_id = match.group(1).strip()
        title = match.group(2).strip()
        # Get content around the section
        start = max(0, match.start() - 100)
        end = min(len(text), match.end() + 500)  # Extract 500 chars after title
        
        content = text[start:end].strip()
        
        # Skip very short sections
        if len(content) < 50:
            continue
            
        # Clean up content
        content = re.sub(r'\n\s*\n\s*\n', '\n\n', content)
        content = content[:1000]  # Limit content size for Solr
        
        # Determine partition/chapter from section ID
        partition = section_id.split('.')[0] if '.' in section_id else "Unknown"
        chapter = f"Partition {partition}"
        
        sections.append({
            "id": f"ecma335_section_{section_id.replace('.', '_')}",
            "section_id": section_id,
            "title": title[:200],
            "content": content,
            "page_start": 0,
            "page_end": 0,
            "chapter": chapter,
            "type": "documentation"
        })
    
    # If no structured sections found, create chunks
    if not sections and text:
        print("Creating document chunks...")
        chunk_size = 1000
        for i in range(0, min(len(text), 30000), chunk_size):
            chunk = text[i:i+chunk_size]
            if len(chunk.strip()) > 50:
                sections.append({
                    "id": f"ecma335_chunk_{i//chunk_size}",
                    "section_id": f"chunk_{i//chunk_size}",
                    "title": f"Document Chunk {i//chunk_size}",
                    "content": chunk.strip()[:1000],
                    "page_start": 0,
                    "page_end": 0,
                    "chapter": "ECMA-335 Specification",
                    "type": "documentation"
                })
    
    return sections

def index_documentation(sections):
    """Index documentation sections into Solr"""
    if not sections:
        print("No sections to index")
        return False
    
    # Create JSON file with documentation sections
    temp_file = '/tmp/ecma335_documentation.json'
    with open(temp_file, 'w') as f:
        json.dump(sections, f, indent=2)
    
    try:
        # Index new documents
        result = subprocess.run([
            "curl", "-s", "-X", "POST",
            "http://localhost:8983/solr/ecma335_standards/update?commit=true",
            "-H", "Content-Type: application/json",
            "--data-binary", "@" + temp_file
        ], check=True, capture_output=True)
        
        print(f"Successfully indexed {len(sections)} documentation sections into Solr")
        return True
        
    except subprocess.CalledProcessError as e:
        print(f"Error indexing documentation into Solr: {e}")
        return False
    except Exception as e:
        print(f"Unexpected error: {e}")
        return False
    finally:
        # Clean up temp file
        if os.path.exists(temp_file):
            os.remove(temp_file)

def get_total_document_count():
    """Get total number of documents in Solr"""
    try:
        result = subprocess.run([
            "curl", "-s", "http://localhost:8983/solr/ecma335_standards/select?q=*:*&wt=json"
        ], capture_output=True, text=True, check=True)
        
        import json
        data = json.loads(result.stdout)
        return data.get('response', {}).get('numFound', 0)
    except Exception as e:
        print(f"Error getting document count: {e}")
        return 0

def main():
    pdf_path = '/home/scott/Repo/lux9-kernel/ECMA-335_6th_edition_june_2012.pdf'
    
    print("Checking current Solr document count...")
    initial_count = get_total_document_count()
    print(f"Current documents in Solr: {initial_count}")
    
    print("Extracting text from ECMA-335 PDF using PyPDF2...")
    text = extract_pdf_text_pypdf2(pdf_path)
    
    if not text:
        print("Failed to extract text from PDF")
        return 1
    
    print(f"Extracted {len(text)} characters from PDF")
    
    # Limit text size for processing if too large
    if len(text) > 100000:
        text = text[:100000]
        print("Limited text to first 100,000 characters for processing")
    
    print("Parsing documentation into sections...")
    sections = parse_chapters_and_sections(text)
    
    print(f"Parsed {len(sections)} sections from documentation")
    
    if not sections:
        print("No sections found in documentation")
        return 1
    
    # Display first few sections as examples
    print("\nFirst 3 sections found:")
    for i, section in enumerate(sections[:3]):
        print(f"  {i+1}. {section['section_id']}: {section['title'][:50]}...")
    
    print("\nIndexing documentation sections into Solr...")
    if index_documentation(sections):
        final_count = get_total_document_count()
        added_count = final_count - initial_count
        print(f"Documentation indexing completed successfully!")
        print(f"Added {added_count} new documentation documents to Solr")
        print(f"Total documents in Solr: {final_count}")
        return 0
    else:
        print("Failed to index documentation")
        return 1

if __name__ == "__main__":
    sys.exit(main())