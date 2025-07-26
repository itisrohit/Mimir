import os
import numpy as np
import onnxruntime as ort
from transformers import AutoTokenizer

# Paths
MODEL_DIR = os.path.join(os.path.dirname(__file__), "..", "models")
ONNX_PATH = os.path.join(MODEL_DIR, "onnx", "model.onnx")
TOKENIZER_PATH = MODEL_DIR  # Directory containing tokenizer files

# Test sentence
TEST_TEXT = "The sky is blue because of Rayleigh scattering."

# Load tokenizer using transformers
try:
    tokenizer = AutoTokenizer.from_pretrained(TOKENIZER_PATH)
except Exception as e:
    print(f"Failed to load tokenizer: {e}")
    exit(1)

# Tokenize input
inputs = tokenizer(TEST_TEXT, return_tensors="np", padding=True, truncation=True)

# Load ONNX model
try:
    session = ort.InferenceSession(ONNX_PATH)
except Exception as e:
    print(f"Failed to load ONNX model: {e}")
    exit(1)

# Prepare model inputs (may need to adjust keys based on model signature)
model_inputs = {k: v for k, v in inputs.items() if k in [i.name for i in session.get_inputs()]}

# Add task_id for Jina Embeddings V3 (4 = 'text-matching')
model_inputs["task_id"] = np.array([4], dtype=np.int64)

# Run model
try:
    outputs = session.run(None, model_inputs)[0]
except Exception as e:
    print(f"Failed to run ONNX model: {e}")
    exit(1)

# Mean pooling (assuming output is [batch, seq, hidden])
def mean_pooling(token_embeddings, attention_mask):
    input_mask_expanded = np.expand_dims(attention_mask, -1)
    sum_embeddings = np.sum(token_embeddings * input_mask_expanded, axis=1)
    sum_mask = np.clip(np.sum(input_mask_expanded, axis=1), a_min=1e-9, a_max=None)
    return sum_embeddings / sum_mask

# Extract attention_mask
attention_mask = inputs.get("attention_mask")
embedding = mean_pooling(outputs, attention_mask)
embedding = embedding / np.linalg.norm(embedding, ord=2, axis=1, keepdims=True)

print("Test embedding shape:", embedding.shape)
print("Test embedding (first 8 values):", embedding[0][:8]) 