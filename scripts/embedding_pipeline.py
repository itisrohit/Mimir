#!/usr/bin/env python3
"""
Embedding pipeline for Mimir using Nomic Embed Text v2 MoE
- Accepts JSON list of chunks from stdin
- Outputs JSON list of {id, embedding, metadata}
- Ready for C++ integration
"""
import sys
import json
from sentence_transformers import SentenceTransformer
import numpy as np

MODEL_NAME = "nomic-ai/nomic-embed-text-v2-moe"
MODEL_CACHE = "models/nomic-embed-text-v2-moe"
EMBED_DIM = 256  # Use 256 for Matryoshka efficiency, or 768 for full

# Helper: Add prefix for correct embedding mode
def add_prefix(texts, mode):
    if mode == "document":
        return [f"search_document: {t}" for t in texts]
    elif mode == "query":
        return [f"search_query: {t}" for t in texts]
    else:
        raise ValueError("mode must be 'document' or 'query'")

def embed_chunks(chunks, mode="document", truncate_dim=EMBED_DIM):
    """
    chunks: list of dicts with keys: id, content, (optional) metadata
    mode: 'document' or 'query'
    Returns: list of dicts with id, embedding (np.ndarray), metadata
    """
    model = SentenceTransformer(MODEL_NAME, cache_folder=MODEL_CACHE, trust_remote_code=True, truncate_dim=truncate_dim)
    texts = [c["content"] for c in chunks]
    ids = [c["id"] for c in chunks]
    metadatas = [c.get("metadata", "") for c in chunks]
    texts_prefixed = add_prefix(texts, mode)
    embeddings = model.encode(texts_prefixed, show_progress_bar=False)
    return [
        {"id": cid, "embedding": emb.tolist(), "metadata": meta}
        for cid, emb, meta in zip(ids, embeddings, metadatas)
    ]

def main():
    # Read JSON list of chunks from stdin
    chunks = json.load(sys.stdin)
    results = embed_chunks(chunks, mode="document")
    print(json.dumps(results))

if __name__ == "__main__":
    main() 