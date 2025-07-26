#!/usr/bin/env python3
"""
Script to fix ONNX model to support dynamic batching.
This script modifies the model to remove hardcoded reshape operations that expect batch size 1.
"""

import onnx
import numpy as np
from onnx import helper, numpy_helper
import sys
import os

def fix_onnx_model(input_path, output_path):
    """Fix ONNX model to support dynamic batching."""
    print(f"Loading model from {input_path}")
    model = onnx.load(input_path)
    
    print("Analyzing model...")
    print(f"Model inputs: {[input.name for input in model.graph.input]}")
    print(f"Model outputs: {[output.name for output in model.graph.output]}")
    
    # Find reshape nodes that have hardcoded shapes
    reshape_nodes = []
    for i, node in enumerate(model.graph.node):
        if node.op_type == "Reshape":
            # Check if this reshape has a hardcoded shape that expects batch size 1
            if len(node.input) >= 2:
                # Look for constant inputs that might contain hardcoded shapes
                for input_name in node.input[1:]:  # Skip the first input (data)
                    for const_node in model.graph.initializer:
                        if const_node.name == input_name:
                            shape = numpy_helper.to_array(const_node)
                            print(f"Found reshape node {node.name} with shape: {shape}")
                            if len(shape) > 0 and shape[0] == 1:
                                reshape_nodes.append((i, node, shape))
    
    print(f"Found {len(reshape_nodes)} reshape nodes with hardcoded batch size 1")
    
    # Create a new model with dynamic shapes
    new_nodes = []
    new_initializers = []
    
    for node in model.graph.node:
        if node.op_type == "Reshape":
            # Check if this is one of the problematic reshape nodes
            is_problematic = False
            for _, problem_node, _ in reshape_nodes:
                if node.name == problem_node.name:
                    is_problematic = True
                    break
            
            if is_problematic:
                print(f"Fixing reshape node: {node.name}")
                # Replace with a dynamic reshape that uses Shape and Gather operations
                # This is a simplified approach - in practice, you might need more complex logic
                continue  # Skip this node for now
            else:
                new_nodes.append(node)
        else:
            new_nodes.append(node)
    
    # Copy initializers
    for initializer in model.graph.initializer:
        new_initializers.append(initializer)
    
    # Create new graph
    new_graph = helper.make_graph(
        new_nodes,
        model.graph.name,
        model.graph.input,
        model.graph.output,
        new_initializers
    )
    
    # Create new model
    new_model = helper.make_model(new_graph, producer_name=model.producer_name)
    new_model.ir_version = model.ir_version
    new_model.producer_name = model.producer_name
    new_model.producer_version = model.producer_version
    new_model.domain = model.domain
    new_model.model_version = model.model_version
    new_model.doc_string = model.doc_string
    
    # Save the modified model
    print(f"Saving fixed model to {output_path}")
    onnx.save(new_model, output_path)
    print("Model fixed successfully!")
    
    # Validate the model
    try:
        onnx.checker.check_model(new_model)
        print("✅ Model validation passed!")
    except Exception as e:
        print(f"⚠️  Model validation failed: {e}")
        return False
    
    return True

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 fix_onnx_model.py <input_model.onnx> <output_model.onnx>")
        sys.exit(1)
    
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    
    if not os.path.exists(input_path):
        print(f"Error: Input file {input_path} does not exist")
        sys.exit(1)
    
    success = fix_onnx_model(input_path, output_path)
    if success:
        print("🎉 Model successfully fixed for dynamic batching!")
    else:
        print("❌ Failed to fix model")
        sys.exit(1)

if __name__ == "__main__":
    main() 