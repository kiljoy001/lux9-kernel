#!/usr/bin/env python3
"""
Semantic Search API for Plan 9 Solr Index
Leverages embeddings and similarity features for intelligent code search
"""

import json
import requests
from typing import List, Dict, Any
import numpy as np
import base64

class Plan9SemanticSearch:
    def __init__(self, solr_url="http://localhost:8983/solr/plan9"):
        self.solr_url = solr_url

    def decode_word2vec(self, encoded_vec: str) -> np.ndarray:
        """Decode base64 word2vec embedding to numpy array"""
        if not encoded_vec:
            return np.array([])
        decoded = base64.b64decode(encoded_vec)
        return np.frombuffer(decoded, dtype=np.float32)

    def cosine_similarity(self, vec1: np.ndarray, vec2: np.ndarray) -> float:
        """Calculate cosine similarity between two vectors"""
        if len(vec1) == 0 or len(vec2) == 0:
            return 0.0
        dot_product = np.dot(vec1, vec2)
        norm1 = np.linalg.norm(vec1)
        norm2 = np.linalg.norm(vec2)
        if norm1 == 0 or norm2 == 0:
            return 0.0
        return dot_product / (norm1 * norm2)

    def semantic_search(self, query: str, top_k: int = 10) -> List[Dict[str, Any]]:
        """
        Perform semantic search using multiple signals:
        - Content matching with fuzzy search
        - Boost by similarity scores
        - Use edismax for intelligent query parsing
        """
        params = {
            'q': f'content:"{query}"~2 OR content:{query}*',
            'defType': 'edismax',
            'qf': 'content^3 file_path^2',
            'mm': '50%',
            'fl': 'id,file_path,type,coq_similarity_score,content',
            'rows': top_k,
            'wt': 'json',
            'boost': 'coq_similarity_score'
        }

        response = requests.get(f"{self.solr_url}/select", params=params)
        data = response.json()
        return data['response']['docs']

    def find_similar_by_vector(self, doc_id: str, top_k: int = 5) -> List[Dict[str, Any]]:
        """Find similar documents using word2vec embeddings"""
        # First get the source document's vector
        params = {
            'q': f'id:{doc_id}',
            'fl': 'coq_word2vec',
            'rows': 1,
            'wt': 'json'
        }
        response = requests.get(f"{self.solr_url}/select", params=params)
        data = response.json()

        if not data['response']['docs']:
            return []

        source_vec_encoded = data['response']['docs'][0].get('coq_word2vec')
        if not source_vec_encoded:
            return []

        source_vec = self.decode_word2vec(source_vec_encoded)

        # Get all documents (in production, use pagination)
        params = {
            'q': '*:*',
            'fl': 'id,file_path,coq_word2vec,coq_similarity_score',
            'rows': 100,
            'wt': 'json'
        }
        response = requests.get(f"{self.solr_url}/select", params=params)
        data = response.json()

        # Calculate similarities
        similarities = []
        for doc in data['response']['docs']:
            if doc['id'] == doc_id:
                continue
            doc_vec_encoded = doc.get('coq_word2vec')
            if doc_vec_encoded:
                doc_vec = self.decode_word2vec(doc_vec_encoded)
                sim = self.cosine_similarity(source_vec, doc_vec)
                similarities.append({
                    'id': doc['id'],
                    'file_path': doc['file_path'],
                    'vector_similarity': sim,
                    'indexed_similarity': doc.get('coq_similarity_score', 0)
                })

        # Sort by similarity and return top k
        similarities.sort(key=lambda x: x['vector_similarity'], reverse=True)
        return similarities[:top_k]

    def minhash_near_duplicates(self, doc_id: str, threshold: int = 2) -> List[Dict[str, Any]]:
        """Find near-duplicate documents using MinHash"""
        # Get the source document's simhash
        params = {
            'q': f'id:{doc_id}',
            'fl': 'simhash_fingerprint',
            'rows': 1,
            'wt': 'json'
        }
        response = requests.get(f"{self.solr_url}/select", params=params)
        data = response.json()

        if not data['response']['docs']:
            return []

        simhash = data['response']['docs'][0].get('simhash_fingerprint')
        if not simhash:
            return []

        # Find documents with similar simhash (fuzzy match)
        params = {
            'q': f'simhash_fingerprint:{simhash}~{threshold}',
            'fl': 'id,file_path,simhash_fingerprint',
            'rows': 20,
            'wt': 'json'
        }
        response = requests.get(f"{self.solr_url}/select", params=params)
        data = response.json()

        return [doc for doc in data['response']['docs'] if doc['id'] != doc_id]

    def conceptual_search(self, concepts: List[str], top_k: int = 10) -> List[Dict[str, Any]]:
        """Search for documents containing multiple concepts"""
        # Build a query that looks for all concepts with varying importance
        concept_queries = []
        for i, concept in enumerate(concepts):
            boost = len(concepts) - i  # Higher boost for earlier concepts
            concept_queries.append(f'content:"{concept}"^{boost}')

        query = ' OR '.join(concept_queries)

        params = {
            'q': query,
            'defType': 'edismax',
            'mm': '25%',  # At least 25% of concepts should match
            'fl': 'id,file_path,type,coq_similarity_score',
            'rows': top_k,
            'wt': 'json'
        }

        response = requests.get(f"{self.solr_url}/select", params=params)
        data = response.json()
        return data['response']['docs']

    def more_like_this(self, doc_id: str, top_k: int = 5) -> List[Dict[str, Any]]:
        """Use Solr's MLT handler for finding similar documents"""
        params = {
            'q': f'id:{doc_id}',
            'mlt.fl': 'content,coq_word2vec',
            'mlt.mindf': 1,
            'mlt.mintf': 1,
            'mlt.maxqt': 25,
            'fl': 'id,file_path,type,coq_similarity_score',
            'rows': top_k,
            'wt': 'json'
        }

        response = requests.get(f"{self.solr_url}/mlt", params=params)
        data = response.json()
        return data['response']['docs']


def main():
    """Example usage of the semantic search API"""
    search = Plan9SemanticSearch()

    print("=== Semantic Search Examples ===\n")

    # Example 1: Semantic search
    print("1. Searching for 'message parsing':")
    results = search.semantic_search("message parsing", top_k=3)
    for doc in results:
        print(f"  - {doc['file_path']} (similarity: {doc.get('coq_similarity_score', 0):.3f})")

    # Example 2: Conceptual search
    print("\n2. Conceptual search for network protocols:")
    results = search.conceptual_search(["tcp", "udp", "network", "socket"], top_k=3)
    for doc in results:
        print(f"  - {doc['file_path']}")

    # Example 3: Find similar documents
    print("\n3. Finding documents similar to 9front_code_4510:")
    results = search.more_like_this("9front_code_4510", top_k=3)
    for doc in results:
        print(f"  - {doc['file_path']} (type: {doc.get('type', 'unknown')})")


if __name__ == "__main__":
    main()