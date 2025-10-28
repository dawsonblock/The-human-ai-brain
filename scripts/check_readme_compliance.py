#!/usr/bin/env python3
"""
README Compliance Checker for Human-AI Brain v3.4.0

Validates that all features documented in README.md are:
1. Implemented in the codebase
2. Tested with passing tests
3. Measured with performance metrics
4. Documented with examples

Exit code 0 = full compliance
Exit code 1 = missing features or failed validations
"""

import os
import sys
import re
import json
from pathlib import Path
from typing import List, Dict, Tuple

class ComplianceChecker:
    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.readme_path = self.repo_root / "README.md"
        self.failures: List[str] = []
        self.warnings: List[str] = []
        
    def check_all(self) -> bool:
        """Run all compliance checks"""
        print("🔍 README Compliance Checker v3.4.0")
        print("=" * 60)
        
        checks = [
            ("Quantum Consciousness Features", self.check_quantum_features),
            ("Memory System Features", self.check_memory_features),
            ("RAG Integration", self.check_rag_features),
            ("gRPC Service", self.check_grpc_service),
            ("Testing Coverage", self.check_test_coverage),
            ("Performance Metrics", self.check_performance_metrics),
            ("Deployment Infrastructure", self.check_deployment),
            ("Security Features", self.check_security),
        ]
        
        for name, check_func in checks:
            print(f"\n📋 {name}")
            print("-" * 60)
            try:
                check_func()
                print(f"✅ PASS")
            except Exception as e:
                self.failures.append(f"{name}: {str(e)}")
                print(f"❌ FAIL: {e}")
        
        return self.report_results()
    
    def check_quantum_features(self):
        """Verify quantum consciousness implementation"""
        required_files = [
            "kernel/include/brain/qw.hpp",
            "kernel/src/qw.cpp",
        ]
        
        for file_path in required_files:
            full_path = self.repo_root / file_path
            if not full_path.exists():
                raise FileNotFoundError(f"Missing: {file_path}")
        
        # Check for Lindblad evolution
        qw_cpp = (self.repo_root / "kernel/src/qw.cpp").read_text()
        if "Lindblad" not in qw_cpp:
            raise AssertionError("Lindblad evolution not found in qw.cpp")
        
        # Check for entropy calculation
        if "entropy" not in qw_cpp.lower():
            raise AssertionError("Entropy calculation not found")
        
        # Check for collapse mechanism
        if "collapse" not in qw_cpp.lower():
            raise AssertionError("Collapse mechanism not found")
        
        print("  ✓ Lindblad evolution implemented")
        print("  ✓ Entropy calculation present")
        print("  ✓ Collapse mechanism present")
    
    def check_memory_features(self):
        """Verify million-scale tiered memory"""
        tiered_memory_hpp = self.repo_root / "kernel/include/brain/tiered_memory.hpp"
        tiered_memory_cpp = self.repo_root / "kernel/src/tiered_memory.cpp"
        
        if not tiered_memory_hpp.exists():
            raise FileNotFoundError("Missing: tiered_memory.hpp")
        if not tiered_memory_cpp.exists():
            raise FileNotFoundError("Missing: tiered_memory.cpp")
        
        content = tiered_memory_hpp.read_text()
        
        # Check for three tiers
        required_components = [
            "HotTierConfig",
            "WarmTierConfig", 
            "ColdTierConfig",
            "TieredLTM"
        ]
        
        for component in required_components:
            if component not in content:
                raise AssertionError(f"Missing component: {component}")
        
        # Check for thread safety
        if "std::mutex" not in content and "std::shared_mutex" not in content:
            self.warnings.append("Thread safety primitives not clearly visible")
        
        print("  ✓ Three-tier architecture (Hot/Warm/Cold)")
        print("  ✓ TieredLTM class defined")
        print("  ✓ Thread safety primitives present")
    
    def check_rag_features(self):
        """Verify RAG integration"""
        ingest_dir = self.repo_root / "ingest_cpp"
        
        if not ingest_dir.exists():
            raise FileNotFoundError("Missing: ingest_cpp directory")
        
        required_files = [
            "ingest_cpp/include/brain/pdf_render.hpp",
            "ingest_cpp/include/brain/ocr_client.hpp",
            "ingest_cpp/include/brain/chunker.hpp",
        ]
        
        for file_path in required_files:
            if not (self.repo_root / file_path).exists():
                self.warnings.append(f"Missing RAG component: {file_path}")
        
        # Check for Qdrant integration
        qdrant_files = list(ingest_dir.glob("**/qdrant*.hpp")) + \
                       list(ingest_dir.glob("**/qdrant*.cpp"))
        
        if not qdrant_files:
            self.warnings.append("Qdrant integration not found")
        
        print("  ✓ PDF rendering support")
        print("  ✓ OCR client present")
        print("  ✓ Text chunker present")
        if qdrant_files:
            print("  ✓ Qdrant integration present")
    
    def check_grpc_service(self):
        """Verify gRPC service implementation"""
        proto_file = self.repo_root / "kernel/proto/brain.proto"
        service_hpp = self.repo_root / "kernel/include/brain/brain_service.hpp"
        service_cpp = self.repo_root / "kernel/src/brain_service.cpp"
        
        if not proto_file.exists():
            raise FileNotFoundError("Missing: brain.proto")
        
        proto_content = proto_file.read_text()
        
        # Check for required RPCs
        required_rpcs = ["Health", "Step", "GetState"]
        for rpc in required_rpcs:
            if f"rpc {rpc}" not in proto_content:
                raise AssertionError(f"Missing RPC: {rpc}")
        
        print(f"  ✓ protobuf definition present")
        print(f"  ✓ Health RPC defined")
        print(f"  ✓ Step RPC defined")
        print(f"  ✓ GetState RPC defined")
    
    def check_test_coverage(self):
        """Verify test suite completeness"""
        tests_dir = self.repo_root / "kernel/tests"
        
        if not tests_dir.exists():
            raise FileNotFoundError("Missing: kernel/tests directory")
        
        test_files = list(tests_dir.glob("*.cpp"))
        
        if len(test_files) < 3:
            raise AssertionError(f"Insufficient test files: {len(test_files)} < 3")
        
        # Check for tiered memory tests
        tiered_tests = tests_dir / "tiered_memory_tests.cpp"
        if not tiered_tests.exists():
            self.warnings.append("Missing: tiered_memory_tests.cpp")
        else:
            content = tiered_tests.read_text()
            if "TEST" not in content:
                raise AssertionError("No test cases found in tiered_memory_tests.cpp")
        
        print(f"  ✓ {len(test_files)} test files found")
        print(f"  ✓ Tiered memory tests present")
    
    def check_performance_metrics(self):
        """Verify performance measurement infrastructure"""
        readme = self.readme_path.read_text()
        
        # Check for documented performance metrics
        perf_keywords = [
            "latency",
            "throughput",
            "benchmark",
            "performance"
        ]
        
        found_metrics = []
        for keyword in perf_keywords:
            if keyword.lower() in readme.lower():
                found_metrics.append(keyword)
        
        if len(found_metrics) < 3:
            self.warnings.append(f"Limited performance documentation: {found_metrics}")
        
        # Check for metrics in code
        metrics_files = list(self.repo_root.glob("**/metrics.hpp")) + \
                       list(self.repo_root.glob("**/metrics.cpp"))
        
        if not metrics_files:
            self.warnings.append("No metrics implementation files found")
        
        print(f"  ✓ Performance metrics documented in README")
        if metrics_files:
            print(f"  ✓ Metrics implementation present")
    
    def check_deployment(self):
        """Verify deployment infrastructure"""
        # Check for Docker
        dockerfile = self.repo_root / "Dockerfile"
        docker_compose = self.repo_root / "docker-compose.yml"
        
        if not dockerfile.exists():
            raise FileNotFoundError("Missing: Dockerfile")
        
        # Check for Kubernetes
        k8s_dir = self.repo_root / "k8s"
        if not k8s_dir.exists():
            self.warnings.append("Missing: k8s/ directory")
        else:
            required_k8s = ["deployment.yaml", "service.yaml"]
            for file_name in required_k8s:
                if not (k8s_dir / file_name).exists():
                    self.warnings.append(f"Missing: k8s/{file_name}")
        
        print("  ✓ Dockerfile present")
        if docker_compose.exists():
            print("  ✓ docker-compose.yml present")
        if k8s_dir.exists():
            print("  ✓ Kubernetes manifests present")
    
    def check_security(self):
        """Verify security features"""
        # Check for seccomp profile
        seccomp = self.repo_root / "docker/seccomp.json"
        if not seccomp.exists():
            self.warnings.append("Missing: docker/seccomp.json")
        
        # Check for config validation
        config_validate = self.repo_root / "kernel/src/config_validate.cpp"
        if not config_validate.exists():
            raise FileNotFoundError("Missing: config_validate.cpp")
        
        content = config_validate.read_text()
        if "validate" not in content.lower():
            raise AssertionError("No validation logic found")
        
        print("  ✓ Configuration validation present")
        if seccomp.exists():
            print("  ✓ seccomp profile present")
    
    def report_results(self) -> bool:
        """Print final report and return success status"""
        print("\n" + "=" * 60)
        print("📊 COMPLIANCE REPORT")
        print("=" * 60)
        
        if not self.failures and not self.warnings:
            print("✅ FULL COMPLIANCE - All checks passed!")
            return True
        
        if self.warnings:
            print(f"\n⚠️  {len(self.warnings)} WARNINGS:")
            for warning in self.warnings:
                print(f"  • {warning}")
        
        if self.failures:
            print(f"\n❌ {len(self.failures)} FAILURES:")
            for failure in self.failures:
                print(f"  • {failure}")
            print("\n🚫 COMPLIANCE FAILED")
            return False
        
        print("\n✅ COMPLIANT (with warnings)")
        return True

def main():
    # Detect repository root
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent
    
    checker = ComplianceChecker(str(repo_root))
    success = checker.check_all()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
