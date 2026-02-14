#!/usr/bin/env python3
"""
measure_compile_time.py - Measure compile-time impact of IoC container registrations

This script measures the compile-time overhead of template-based IoC containers
to assess the impact of heavy template instantiation.

Usage:
    python measure_compile_time.py --container-header "Skirnir.hpp" \
                                   --compiler clang++ \
                                   --registrations "0,10,50,100,500" \
                                   --output results.csv

Requirements:
    - Python 3.6+
    - A C++ compiler (g++, clang++, or cl.exe)
"""

import argparse
import csv
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import List, Tuple


class CompileTimeMeasurer:
    """Measures compile-time impact of IoC container registrations."""
    
    def __init__(self, compiler: str, container_header: str, cpp_standard: str = "20"):
        self.compiler = compiler
        self.container_header = container_header
        self.cpp_standard = cpp_standard
        
    def generate_test_file(self, num_registrations: int) -> str:
        """Generate a C++ file with N service registrations."""
        
        # Template for the test file
        template = f'''#include "{self.container_header}"
#include <memory>

// Generate {num_registrations} dummy services for registration
'''
        
        # Generate service classes
        for i in range(num_registrations):
            template += f'''
class Service{i} {{
public:
    Service{i}() = default;
    void work() {{}}
}};
'''
        
        # Generate registration code
        template += f'''
int main() {{
    // Container setup
    auto container = skr::ServiceCollection();
    
    // Register {num_registrations} services
'''
        
        for i in range(num_registrations):
            template += f'    container.AddSingleton<Service{i}>();\n'
        
        template += '''    
    auto provider = container.CreateServiceProvider();
    return 0;
}
'''
        return template
    
    def compile_file(self, source_code: str, optimization: str = "-O2") -> Tuple[float, int]:
        """
        Compile the given source code and measure time.
        
        Returns:
            Tuple of (compilation_time_seconds, binary_size_bytes)
        """
        with tempfile.NamedTemporaryFile(mode='w', suffix='.cpp', delete=False) as f:
            f.write(source_code)
            source_file = f.name
        
        output_file = source_file.replace('.cpp', '.out')
        
        try:
            # Determine compiler flags based on compiler
            if 'cl.exe' in self.compiler or self.compiler == 'cl':
                # MSVC
                cmd = [
                    self.compiler,
                    '/std:c++' + self.cpp_standard,
                    '/EHsc',
                    source_file,
                    '/Fe:' + output_file,
                    '/nologo'
                ]
                if optimization == "-O2":
                    cmd.append('/O2')
                elif optimization == "-O0":
                    cmd.append('/Od')
            else:
                # GCC or Clang
                cmd = [
                    self.compiler,
                    f'-std=c++{self.cpp_standard}',
                    optimization,
                    '-c',  # Compile only, don't link for speed
                    '-o', output_file,
                    source_file
                ]
            
            # Measure compilation time
            start_time = time.perf_counter()
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300  # 5 minute timeout
            )
            end_time = time.perf_counter()
            
            if result.returncode != 0:
                print(f"Compilation failed:\n{result.stderr}")
                return -1, 0
            
            compilation_time = end_time - start_time
            
            # Get binary size
            binary_size = 0
            if os.path.exists(output_file):
                binary_size = os.path.getsize(output_file)
            
            return compilation_time, binary_size
            
        except subprocess.TimeoutExpired:
            print("Compilation timed out!")
            return -1, 0
        except Exception as e:
            print(f"Error during compilation: {e}")
            return -1, 0
        finally:
            # Cleanup
            if os.path.exists(source_file):
                os.unlink(source_file)
            if os.path.exists(output_file):
                os.unlink(output_file)
    
    def measure(self, registrations: List[int], iterations: int = 3) -> List[dict]:
        """
        Measure compile times for different registration counts.
        
        Args:
            registrations: List of registration counts to test
            iterations: Number of times to compile each configuration (for averaging)
        
        Returns:
            List of measurement results
        """
        results = []
        
        print(f"Measuring compile times with {self.compiler} (C++{self.cpp_standard})")
        print(f"Container header: {self.container_header}")
        print(f"Iterations per configuration: {iterations}")
        print("-" * 80)
        
        for num_regs in registrations:
            print(f"\nTesting with {num_regs} registrations...")
            
            # Generate test file
            source_code = self.generate_test_file(num_regs)
            
            # Run multiple iterations for averaging
            times = []
            sizes = []
            
            for i in range(iterations):
                print(f"  Iteration {i+1}/{iterations}...", end=' ', flush=True)
                compile_time, binary_size = self.compile_file(source_code)
                
                if compile_time < 0:
                    print("FAILED")
                    continue
                
                times.append(compile_time)
                sizes.append(binary_size)
                print(f"{compile_time:.3f}s")
            
            if times:
                avg_time = sum(times) / len(times)
                avg_size = sum(sizes) / len(sizes) if sizes else 0
                
                result = {
                    'registrations': num_regs,
                    'avg_compile_time': avg_time,
                    'min_compile_time': min(times),
                    'max_compile_time': max(times),
                    'avg_binary_size': avg_size,
                    'iterations': len(times)
                }
                results.append(result)
                
                print(f"  Average: {avg_time:.3f}s")
                print(f"  Binary size: {avg_size / 1024:.1f} KB")
        
        return results


def main():
    parser = argparse.ArgumentParser(
        description='Measure IoC container compile-time impact'
    )
    parser.add_argument(
        '--container-header',
        default='Skirnir.hpp',
        help='Path to your IoC container header file'
    )
    parser.add_argument(
        '--compiler',
        default='g++',
        help='C++ compiler to use (g++, clang++, cl.exe)'
    )
    parser.add_argument(
        '--cpp-standard',
        default='20',
        help='C++ standard version (17, 20, 23)'
    )
    parser.add_argument(
        '--registrations',
        default='0,10,50,100,500',
        help='Comma-separated list of registration counts to test'
    )
    parser.add_argument(
        '--iterations',
        type=int,
        default=3,
        help='Number of compilation iterations per configuration'
    )
    parser.add_argument(
        '--output',
        default='compile_time_results.csv',
        help='Output CSV file for results'
    )
    parser.add_argument(
        '--generate-100-regs-file',
        action='store_true',
        help='Generate a standalone C++ file with 100 registrations'
    )
    
    args = parser.parse_args()
    
    # Parse registration counts
    registration_counts = [int(x.strip()) for x in args.registrations.split(',')]
    
    if args.generate_100_regs_file:
        # Generate a standalone file with 100 registrations
        measurer = CompileTimeMeasurer(args.compiler, args.container_header, args.cpp_standard)
        source_code = measurer.generate_test_file(100)
        
        output_filename = 'test_100_registrations.cpp'
        with open(output_filename, 'w') as f:
            f.write(source_code)
        
        print(f"Generated {output_filename} with 100 service registrations")
        print(f"Compile with: {args.compiler} -std=c++{args.cpp_standard} -O2 -o test_100 {output_filename}")
        return
    
    # Run measurements
    measurer = CompileTimeMeasurer(args.compiler, args.container_header, args.cpp_standard)
    results = measurer.measure(registration_counts, args.iterations)
    
    # Export to CSV
    with open(args.output, 'w', newline='') as csvfile:
        fieldnames = ['registrations', 'avg_compile_time', 'min_compile_time', 
                     'max_compile_time', 'avg_binary_size', 'iterations']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        
        writer.writeheader()
        for result in results:
            writer.writerow(result)
    
    print(f"\n{'='*80}")
    print(f"Results saved to: {args.output}")
    print(f"{'='*80}")
    
    # Print summary table
    print("\nSummary:")
    print(f"{'Registrations':<15} {'Avg Time (s)':<15} {'Binary Size (KB)':<20}")
    print("-" * 50)
    for r in results:
        print(f"{r['registrations']:<15} {r['avg_compile_time']:<15.3f} {r['avg_binary_size']/1024:<20.1f}")
    
    # Calculate overhead per registration
    if len(results) >= 2:
        baseline = results[0]
        max_test = results[-1]
        
        if baseline['registrations'] == 0 and max_test['registrations'] > 0:
            time_diff = max_test['avg_compile_time'] - baseline['avg_compile_time']
            regs_diff = max_test['registrations'] - baseline['registrations']
            time_per_reg = time_diff / regs_diff
            
            print(f"\nEstimated overhead per registration: {time_per_reg*1000:.3f} ms")


if __name__ == '__main__':
    main()
