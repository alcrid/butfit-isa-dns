#!/bin/bash
set +e

# Configuration
DNS_BINARY="../dns"
RESOLVER="dns.google"
PORT=12345
BLOCKLIST_FILE="filter.txt"

# Colors
RED='\e[31m'
GREEN='\e[32m'
RESET='\e[0m'

# Test counter
TEST_NUM=1
PASSED=0
FAILED=0

pass() {
    echo -e "${GREEN}Test $TEST_NUM: $1${RESET}"
    ((TEST_NUM++))
    ((PASSED++))
}

fail() {
    echo -e "${RED}Test $TEST_NUM: $1${RESET}"
    if [ -n "$2" ]; then
        echo -e "${RED}  Details: $2${RESET}"
    fi
    ((TEST_NUM++))
    ((FAILED++))
}

cleanup() {
    if [ -n "$DNS_PID" ] && kill -0 "$DNS_PID" 2>/dev/null; then
        kill "$DNS_PID" >/dev/null 2>&1 || true
        wait "$DNS_PID" 2>/dev/null || true
    fi
    rm -f "$BLOCKLIST_FILE"
}

trap cleanup EXIT INT TERM

# Prepare blocklist
cat > $BLOCKLIST_FILE << 'EOF'
facebook.com
youtube.com
www.alza.sk
# .sk
EOF

# Test 1: No parameters
if $DNS_BINARY > /dev/null 2>&1; then
    fail "No parameters should fail"
else
    pass "No parameters"
fi

# Test 2: Unknown flag (-x)
if $DNS_BINARY -x > /dev/null 2>&1; then
    fail "Unknown flag (-x) should fail"
else
    pass "Unknown flag (-x)"
fi

# Test 3: Multiple unknown flags
if $DNS_BINARY -x -y -z > /dev/null 2>&1; then
    fail "Multiple unknown flags should fail"
else
    pass "Multiple unknown flags"
fi

# Test 4: Help option (-h)
HELP_OUTPUT=$($DNS_BINARY -h 2>&1)
if [ $? -eq 0 ] && [ -n "$HELP_OUTPUT" ]; then
    pass "Help option (-h)"
else
    fail "Help option (-h) should succeed and print usage"
fi

# Test 5: Missing -s
if $DNS_BINARY -p $PORT -f $BLOCKLIST_FILE > /dev/null 2>&1; then
    fail "Missing -s should fail"
else
    pass "Missing -s"
fi

# Test 6: Missing -f
if $DNS_BINARY -s $RESOLVER -p $PORT > /dev/null 2>&1; then
    fail "Missing -f should fail"
else
    pass "Missing -f"
fi

# Test 7: Non-existing filter file
if $DNS_BINARY -s $RESOLVER -p $PORT -f nonexisting_test.txt > /dev/null 2>&1; then
    fail "Non-existing filter file should fail"
else
    pass "Non-existing filter file (-f)"
fi

# Test 8: Non-existing server
if $DNS_BINARY -s test_asdfghjlk.asdf -p $PORT -f $BLOCKLIST_FILE > /dev/null 2>&1; then
    fail "Non-existing server should fail"
else
    pass "Non-existing server (-s)"
fi

# Test 9: Empty server parameter
if $DNS_BINARY -s "" -p $PORT -f $BLOCKLIST_FILE > /dev/null 2>&1; then
    fail "Empty server parameter should fail"
else
    pass "Empty server parameter"
fi

# Test 10: Non-numeric port
if $DNS_BINARY -s $RESOLVER -p abc -f $BLOCKLIST_FILE > /dev/null 2>&1; then
    fail "Non-numeric port should fail"
else
    pass "Non-numeric port (-p abc)"
fi

# Test 11: Negative port
if $DNS_BINARY -s $RESOLVER -p -1 -f $BLOCKLIST_FILE > /dev/null 2>&1; then
    fail "Negative port should fail"
else
    pass "Negative port (-p -1)"
fi

# Test 12: Port out of range (65536)
if $DNS_BINARY -s $RESOLVER -p 65536 -f $BLOCKLIST_FILE > /dev/null 2>&1; then
    fail "Port 65536 should fail (out of range)"
else
    pass "Port 65536 (out of range)"
fi

# Test 13: Decimal port
if $DNS_BINARY -s $RESOLVER -p 12.34 -f $BLOCKLIST_FILE > /dev/null 2>&1; then
    fail "Decimal port should fail"
else
    pass "Decimal port (-p 12.34)"
fi

# Test 14: Duplicate -s parameter
if $DNS_BINARY -s $RESOLVER -s 8.8.8.8 -p $PORT -f $BLOCKLIST_FILE > /dev/null 2>&1; then
    fail "Duplicate -s parameter should fail"
else
    pass "Duplicate -s parameter"
fi

# Test 15: Duplicate -p parameter
if $DNS_BINARY -s $RESOLVER -p $PORT -p 5353 -f $BLOCKLIST_FILE > /dev/null 2>&1; then
    fail "Duplicate -p parameter should fail"
else
    pass "Duplicate -p parameter"
fi

# Test 16: Duplicate -f parameter
if $DNS_BINARY -s $RESOLVER -p $PORT -f $BLOCKLIST_FILE -f other.txt > /dev/null 2>&1; then
    fail "Duplicate -f parameter should fail"
else
    pass "Duplicate -f parameter"
fi

# Test 17: Standard parameter order
$DNS_BINARY -s $RESOLVER -p $PORT -f $BLOCKLIST_FILE > /dev/null 2>&1 &
TEST_PID=$!
sleep 0.5
if kill -0 "$TEST_PID" 2>/dev/null; then
    pass "Standard parameter order (-s -p -f)"
    kill "$TEST_PID" > /dev/null 2>&1 || true
    wait "$TEST_PID" 2>/dev/null || true
else
    fail "Standard parameter order (-s -p -f) failed to start"
fi

echo "================================"
echo -e "Total:   $((TEST_NUM - 1)) tests"
echo -e "${GREEN}Passed:  $PASSED${RESET}"
echo -e "${RED}Failed:  $FAILED${RESET}"
echo "================================"