#!/usr/bin/env python3
"""Simple Python client to test the Brain gRPC server."""

import grpc
import sys
import json

# We need to generate the Python gRPC stubs from proto first
# For now, let's use grpc_cli or manual testing

def test_health():
    """Test the Health endpoint."""
    channel = grpc.insecure_channel('localhost:50051')
    
    # Import the generated proto files
    try:
        import brain_pb2
        import brain_pb2_grpc
        
        stub = brain_pb2_grpc.BrainStub(channel)
        request = brain_pb2.HealthReq()
        response = stub.Health(request)
        
        print("✓ Health Check:")
        print(f"  Status: {response.status}")
        print(f"  Version: {response.version}")
        print(f"  Uptime: {response.uptime_ms} ms")
        return True
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_step():
    """Test the Step endpoint with random input."""
    import numpy as np
    
    channel = grpc.insecure_channel('localhost:50051')
    
    try:
        import brain_pb2
        import brain_pb2_grpc
        
        stub = brain_pb2_grpc.BrainStub(channel)
        
        # Create MNIST-like input (28x28 = 784 dimensions)
        input_vec = np.random.rand(784).tolist()
        
        request = brain_pb2.StepReq(input=input_vec)
        response = stub.Step(request)
        
        print("\n✓ Cognitive Step:")
        print(f"  WM Dimension: {len(response.h_wm)}")
        print(f"  Global Dimension: {len(response.h_global)}")
        print(f"  Collapsed Index: {response.collapsed_index}")
        print(f"  Entropy: {response.entropy:.4f}")
        print(f"\n  Phenomenal Report:")
        print(f"    Intensity: {response.phenomenal.intensity:.3f}")
        print(f"    Clarity: {response.phenomenal.clarity:.3f}")
        print(f"    Valence: {response.phenomenal.valence:.3f}")
        print(f"    Arousal: {response.phenomenal.arousal:.3f}")
        print(f"\n  Metrics:")
        print(f"    Collapse Rate: {response.metrics.collapse_rate_hz:.2f} Hz")
        return True
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_get_state():
    """Test the GetState endpoint."""
    channel = grpc.insecure_channel('localhost:50051')
    
    try:
        import brain_pb2
        import brain_pb2_grpc
        
        stub = brain_pb2_grpc.BrainStub(channel)
        request = brain_pb2.StateReq()
        response = stub.GetState(request)
        
        print("\n✓ Quantum State:")
        print(f"  Dimension: {response.dimension}")
        print(f"  Entropy: {response.entropy:.4f}")
        print(f"  Eigenvalues: {list(response.eigenvalues)[:5]}...")  # First 5
        print(f"\n  Memory:")
        print(f"    STM Size: {response.memory.stm_size}")
        print(f"    LTM Size: {response.memory.ltm_size}")
        return True
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("=" * 60)
    print(" Brain FDQC gRPC Server - Test Client")
    print("=" * 60)
    
    success = True
    
    # Test all endpoints
    success &= test_health()
    success &= test_step()
    success &= test_get_state()
    
    print("\n" + "=" * 60)
    if success:
        print("✓ All tests passed!")
    else:
        print("✗ Some tests failed")
    print("=" * 60)
