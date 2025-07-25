from fastapi import FastAPI
from pydantic import BaseModel
from sentence_transformers import SentenceTransformer
from typing import List
import os
import sys

# Model name and cache directory
MODEL_NAME = "nomic-ai/nomic-embed-text-v2-moe"

app = FastAPI()
model = SentenceTransformer(MODEL_NAME, trust_remote_code=True)

class EmbeddingRequest(BaseModel):
    texts: List[str]
    ids: List[str]

class EmbeddingResponse(BaseModel):
    id: str
    embedding: List[float]

@app.post("/embed", response_model=List[EmbeddingResponse])
def embed(req: EmbeddingRequest):
    embeddings = model.encode(req.texts, show_progress_bar=False)
    return [{"id": id_, "embedding": emb.tolist()} for id_, emb in zip(req.ids, embeddings)] 