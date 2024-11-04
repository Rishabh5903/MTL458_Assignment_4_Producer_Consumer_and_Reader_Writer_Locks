#!/bin/zsh

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to clear and create initial files
setup_files() {
    # Clear previous output files
    echo -n > output-reader-pref.txt
    echo -n > output-writer-pref.txt
    echo -n > shared-file.txt
}

# Function to compile programs
compile_programs() {
    echo "${YELLOW}Compiling programs...${NC}"
    
    if gcc rwlock-reader-pref.c -o reader-pref -lpthread; then
        echo "${GREEN}✓ reader-pref compiled successfully${NC}"
    else
        echo "${RED}✗ reader-pref compilation failed${NC}"
        return 1
    fi
    
    if gcc rwlock-writer-pref.c -o writer-pref -lpthread; then
        echo "${GREEN}✓ writer-pref compiled successfully${NC}"
    else
        echo "${RED}✗ writer-pref compilation failed${NC}"
        return 1
    fi
}

# Function to run test case
run_test() {
    local readers=$1
    local writers=$2
    
    echo "\n${YELLOW}Running test with $readers readers and $writers writers${NC}"
    
    # Test reader preference
    echo "${YELLOW}Testing reader preference implementation:${NC}"
    setup_files
    ./reader-pref $readers $writers
    echo "${GREEN}✓ Reader preference test complete${NC}"
    echo "Output from output-reader-pref.txt:"
    cat output-reader-pref.txt
    echo "\nContent of shared-file.txt:"
    cat shared-file.txt
    
    # Test writer preference
    echo "\n${YELLOW}Testing writer preference implementation:${NC}"
    setup_files
    ./writer-pref $readers $writers
    echo "${GREEN}✓ Writer preference test complete${NC}"
    echo "Output from output-writer-pref.txt:"
    cat output-writer-pref.txt
    echo "\nContent of shared-file.txt:"
    cat shared-file.txt
    
    echo "\n${YELLOW}----------------------------------------${NC}"
}

# Main execution
echo "${YELLOW}Starting Reader-Writer Lock Testing${NC}"

# Compile programs
compile_programs || exit 1

# Run various test cases
test_cases=(
    "2 1"  # 2 readers, 1 writer
    "5 2"  # 5 readers, 2 writers
    "3 3"  # 3 readers, 3 writers
    "1 5"  # 1 reader, 5 writers
)

for test_case in "${test_cases[@]}"; do
    read readers writers <<< "$test_case"
    run_test $readers $writers
done

echo "${GREEN}All tests completed!${NC}"