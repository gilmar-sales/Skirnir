#!/bin/bash
# measure_compile_time.sh - Shell script alternative to measure_compile_time.py
# Usage: ./measure_compile_time.sh [compiler] [header] [registrations...]
# Example: ./measure_compile_time.sh g++ "Skirnir.hpp" 0 10 50 100 500

set -e

COMPILER=${1:-g++}
HEADER=${2:-Skirnir.hpp}
shift 2
REGISTRATIONS=($@)

if [ ${#REGISTRATIONS[@]} -eq 0 ]; then
    REGISTRATIONS=(0 10 50 100 500)
fi

echo "=================================="
echo "IoC Container Compile Time Analysis"
echo "=================================="
echo "Compiler: $COMPILER"
echo "Header: $HEADER"
echo "Registrations: ${REGISTRATIONS[@]}"
echo ""

# Create temporary directory
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

# Results file
RESULTS_FILE="compile_time_results.csv"
echo "registrations,compile_time_seconds,binary_size_bytes" > $RESULTS_FILE

generate_test_file() {
    local num_regs=$1
    local output=$2
    
    cat > $output << EOF
#include "$HEADER"
#include <memory>

// Generate $num_regs dummy services
EOF

    for ((i=0; i<num_regs; i++)); do
        cat >> $output << EOF
class Service$i {
public:
    Service$i() = default;
    void work() {}
};
EOF
    done

    cat >> $output << EOF

int main() {
    auto container = skr::ServiceCollection();
    
    // Register $num_regs services
EOF

    for ((i=0; i<num_regs; i++)); do
        echo "    container.AddSingleton<Service$i>();" >> $output
    done

    cat >> $output << EOF
    
    auto provider = container.CreateServiceProvider();
    return 0;
}
EOF
}

measure_compile_time() {
    local num_regs=$1
    local source_file=$2
    
    echo "Testing with $num_regs registrations..."
    
    # Generate source
    generate_test_file $num_regs $source_file
    
    # Measure compilation time
    start_time=$(date +%s.%N)
    
    if [[ "$COMPILER" == *"cl"* ]] || [[ "$COMPILER" == *"cl.exe"* ]]; then
        # MSVC
        $COMPILER /std:c++20 /O2 /c /nologo "$source_file" /Fo:"${source_file}.obj" 2>&1
        compile_time=$(echo "$(date +%s.%N) - $start_time" | bc)
        binary_size=$(stat -f%z "${source_file}.obj" 2>/dev/null || stat -c%s "${source_file}.obj" 2>/dev/null || echo 0)
    else
        # GCC/Clang
        $COMPILER -std=c++20 -O2 -c -o "${source_file}.o" "$source_file" 2>&1
        compile_time=$(echo "$(date +%s.%N) - $start_time" | bc)
        binary_size=$(stat -f%z "${source_file}.o" 2>/dev/null || stat -c%s "${source_file}.o" 2>/dev/null || echo 0)
    fi
    
    echo "$num_regs,$compile_time,$binary_size" >> $RESULTS_FILE
    printf "  Compile time: %.3f seconds\n" $compile_time
    printf "  Binary size: %.1f KB\n" $(echo "scale=1; $binary_size / 1024" | bc)
    echo ""
}

# Main loop
for num_regs in "${REGISTRATIONS[@]}"; do
    source_file="$TMPDIR/test_${num_regs}_regs.cpp"
    measure_compile_time $num_regs $source_file
done

echo "=================================="
echo "Results saved to: $RESULTS_FILE"
echo "=================================="
echo ""
echo "Summary:"
printf "%-15s %-20s %-20s\n" "Registrations" "Compile Time (s)" "Binary Size (KB)"
echo "--------------------------------------------------------"

tail -n +2 $RESULTS_FILE | while IFS=, read -r regs time size; do
    size_kb=$(echo "scale=1; $size / 1024" | bc)
    printf "%-15s %-20.3f %-20.1f\n" "$regs" "$time" "$size_kb"
done

# Calculate overhead per registration
echo ""
echo "Analysis:"
first_line=$(tail -n +2 $RESULTS_FILE | head -1)
last_line=$(tail -1 $RESULTS_FILE)
first_regs=$(echo "$first_line" | cut -d',' -f1)
first_time=$(echo "$first_line" | cut -d',' -f2)
last_regs=$(echo "$last_line" | cut -d',' -f1)
last_time=$(echo "$last_line" | cut -d',' -f2)

if [ "$first_regs" -eq 0 ] && [ "$last_regs" -gt 0 ]; then
    time_diff=$(echo "$last_time - $first_time" | bc)
    regs_diff=$((last_regs - first_regs))
    time_per_reg=$(echo "scale=6; $time_diff / $regs_diff" | bc)
    time_per_reg_ms=$(echo "scale=3; $time_per_reg * 1000" | bc)
    echo "Estimated overhead per registration: ${time_per_reg_ms}ms"
fi
