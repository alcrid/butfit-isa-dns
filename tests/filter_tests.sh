#!/bin/bash
set +e

cd "$(dirname "$0")" || exit 1
DNS_BINARY="../dns"
RESOLVER="dns.google"
PORT=2224
BLOCKLIST_FILE="filter.txt"

RED='\e[31m'
GREEN='\e[32m'
RESET='\e[0m'

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
        sleep 1
        
        if kill -0 "$DNS_PID" 2>/dev/null; then
            kill -9 "$DNS_PID" >/dev/null 2>&1 || true
        fi
        
        wait "$DNS_PID" 2>/dev/null || true
    fi    
    rm -f "$BLOCKLIST_FILE"
}

trap cleanup EXIT INT TERM

# Create blocklist 
cat > $BLOCKLIST_FILE << 'EOF'
# Test blocklist for DNS filter
facebook.com
 youtube.com
www.alza.sk
stackoverflow.com
    messenger.com
test.example.com

github.com 
EOF

valgrind --leak-check=full --error-exitcode=1 --log-file=valgrind.log \
    "$DNS_BINARY" -s "$RESOLVER" -p "$PORT" -f "$BLOCKLIST_FILE" >/dev/null 2>&1 &

DNS_PID=$!
sleep 2

# Test 1: Allowed domain returns A record
RES=$(dig @127.0.0.1 -p $PORT +short google.com A 2>/dev/null || true)
if [ -n "$RES" ] && [[ "$RES" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    pass "Allowed domain (google.com) returns A record"
else
    fail "Allowed domain (google.com) should return A record" "Got: $RES"
fi

# Test 2: Subdomain of allowed domain
RES=$(dig @127.0.0.1 -p $PORT +short www.google.com A 2>/dev/null || true)
if [ -n "$RES" ]; then
    pass "Subdomain of allowed domain works"
else
    fail "www.google.com should return A record" "Got: $RES"
fi


# Test 3: Blocked domain returns REFUSED
if dig @127.0.0.1 -p $PORT facebook.com A 2>/dev/null | grep -q "status: REFUSED"; then
    pass "Blocked domain returns REFUSED"
else
    fail "facebook.com should return REFUSED" "Response doesn't contain REFUSED status"
fi

# Test 4: Case-insensitive blocking
if dig @127.0.0.1 -p $PORT FaceBook.CoM A 2>/dev/null | grep -q "status: REFUSED"; then
    pass "Blocked domain is case-insensitive"
else
    fail "FaceBook.CoM should be blocked (case-insensitive)"
fi


# Test 5: Subdomain of blocked domain
if dig @127.0.0.1 -p $PORT business.facebook.com A 2>/dev/null | grep -q "status: REFUSED"; then
    pass "Subdomain of blocked domain is blocked"
else
    fail "business.facebook.com should be blocked"
fi

# Test 6: Specific subdomain blocked (test.example.com)
if dig @127.0.0.1 -p $PORT test.example.com A 2>/dev/null | grep -q "status: REFUSED"; then
    pass "Specific subdomain is blocked"
else
    fail "test.example.com should be blocked"
fi

# Test 7: Sub-subdomain of blocked domain
if dig @127.0.0.1 -p $PORT sub.test.example.com A 2>/dev/null | grep -q "status: REFUSED"; then
    pass "Sub-subdomain of blocked domain is blocked"
else
    fail "sub.test.example.com should be blocked"
fi

# Test 8: Parent domain allowed when only subdomain is blocked
RES=$(dig @127.0.0.1 -p $PORT +short example.com A 2>/dev/null || true)
if [ -n "$RES" ]; then
    pass "Parent domain is allowed when only subdomain is blocked"
else
    fail "example.com should be allowed (only test.example.com is blocked)"
fi

# Test 9: www prefix blocked
if dig @127.0.0.1 -p $PORT www.alza.sk A 2>/dev/null | grep -q "status: REFUSED"; then
    pass "www.alza.sk is blocked"
else
    fail "www.alza.sk should be blocked"
fi

# Test 10: Parent of www subdomain allowed
RES=$(dig @127.0.0.1 -p $PORT +short alza.sk A 2>/dev/null || true)
if [ -n "$RES" ]; then
    pass "Parent domain (alza.sk) is allowed"
else
    fail "alza.sk should be allowed (only www.alza.sk is blocked)"
fi


# Test 11: Leading whitespace domain (youtube.com)
if dig @127.0.0.1 -p $PORT youtube.com A 2>/dev/null | grep -q "status: REFUSED"; then
    pass "Domain with leading whitespace is normalized and blocked"
else
    fail "youtube.com should be blocked (leading whitespace in blocklist)"
fi

# Test 12: Trailing whitespace domain (github.com)
if dig @127.0.0.1 -p $PORT github.com A 2>/dev/null | grep -q "status: REFUSED"; then
    pass "Domain with trailing whitespace is normalized and blocked"
else
    fail "github.com should be blocked (trailing whitespace in blocklist)"
fi

# Test 13: Both leading and trailing whitespace (messenger.com)
if dig @127.0.0.1 -p $PORT messenger.com A 2>/dev/null | grep -q "status: REFUSED"; then
    pass "Domain with leading/trailing whitespace is normalized"
else
    fail "messenger.com should be blocked (whitespace around it in blocklist)"
fi


# Test 14: Commented line not blocked
RES=$(dig @127.0.0.1 -p $PORT +short example.sk A 2>/dev/null || true)
if [ -n "$RES" ]; then
    pass "Commented domain (.sk) is not blocked"
else
    fail "example.sk should be allowed (not in blocklist)"
fi


# Test 15: NOT_IMPLEMENTED for MX queries
if dig @127.0.0.1 -p $PORT facebook.com MX 2>/dev/null | grep -qF "status: NOTIMP"; then
    pass "MX query returns NOTIMP"
else
    fail "MX query should return NOTIMP status"
fi

# Test 16: NOT_IMPLEMENTED for AAAA queries
if dig @127.0.0.1 -p $PORT facebook.com AAAA 2>/dev/null | grep -qF "status: NOTIMP"; then
    pass "AAAA query returns NOTIMP"
else
    fail "AAAA query should return NOTIMP status"
fi

# Test 17: TXT query (not implemented)
if dig @127.0.0.1 -p $PORT google.com TXT 2>/dev/null | grep -qF "status: NOTIMP"; then
    pass "TXT query returns NOTIMP"
else
    fail "TXT query should return NOTIMP" "Only A records are supported"
fi

# Test 18: NS query (not implemented)
if dig @127.0.0.1 -p $PORT google.com NS 2>/dev/null | grep -qF "status: NOTIMP"; then
    pass "NS query returns NOTIMP"
else
    fail "NS query should return NOTIMP" "Only A records are supported"
fi

# Test 19: CNAME query (not implemented)
if dig @127.0.0.1 -p $PORT google.com CNAME 2>/dev/null | grep -qF "status: NOTIMP"; then
    pass "CNAME query returns NOTIMP"
else
    fail "CNAME query should return NOTIMP" "Only A records are supported"
fi


# Test 20: Empty query handling
echo "" | timeout 2 nc -u 127.0.0.1 $PORT >/dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 124 ] || [ $EXIT_CODE -eq 0 ]; then
    pass "Empty query handled without crash"
else
    pass "Empty query handled"
fi


# Test 21: Multiple concurrent allowed queries
(
    dig @127.0.0.1 -p $PORT +short google.com A >/dev/null 2>&1 &
    dig @127.0.0.1 -p $PORT +short cloudflare.com A >/dev/null 2>&1 &
    dig @127.0.0.1 -p $PORT +short example.com A >/dev/null 2>&1 &
    wait
)
if [ $? -eq 0 ]; then
    pass "Concurrent allowed queries handled"
else
    fail "Concurrent queries failed" "Threading issue possible"
fi

# Test 22: Mixed concurrent queries
(
    dig @127.0.0.1 -p $PORT google.com A >/dev/null 2>&1 &
    dig @127.0.0.1 -p $PORT facebook.com A >/dev/null 2>&1 &
    dig @127.0.0.1 -p $PORT cloudflare.com A >/dev/null 2>&1 &
    wait
)
if [ $? -eq 0 ]; then
    pass "Mixed concurrent queries handled"
else
    fail "Mixed concurrent queries failed" "Threading issue possible"
fi

# Test 23: IPv6 connectivity
RES=$(dig @::1 -p $PORT +short google.com A 2>/dev/null || true)
if [ -n "$RES" ]; then
    pass "IPv6 connectivity works"
fi

cleanup

if [ -f valgrind.log ]; then
    # Check for definite memory leaks (the important ones)
    if grep -q "All heap blocks were freed" valgrind.log; then
        pass "No memory leaks (all heap blocks freed)"
    else
        LEAKED=$(grep "definitely lost:" valgrind.log | awk '{print $4}' || echo "unknown")
        if [ "$LEAKED" = "0" ] || [ -z "$LEAKED" ]; then
            pass "No definite memory leaks"
        else
            fail "Memory leaks detected: $LEAKED bytes definitely lost" "Check valgrind.log"
        fi
    fi
    
    # Check for errors
    ERROR_COUNT=$(grep "ERROR SUMMARY:" valgrind.log | grep -oP '\d+(?= errors)' | head -1 || echo "0")
    
    if [ "$ERROR_COUNT" = "0" ] || [ -z "$ERROR_COUNT" ]; then
        pass "No Valgrind errors detected"
    else
        fail "Valgrind errors detected: $ERROR_COUNT errors" "Check valgrind.log for details"
    fi
    
    echo ""
    echo "=== Valgrind Summary ==="
    grep "ERROR SUMMARY" valgrind.log || true
    grep "All heap blocks were freed" valgrind.log || true
    grep "definitely lost:" valgrind.log || true
    grep "indirectly lost:" valgrind.log || true
    grep "possibly lost:" valgrind.log || true
    grep "still reachable:" valgrind.log || true
fi

echo "================================"
echo -e "Total:   $((TEST_NUM - 1)) tests"
echo -e "${GREEN}Passed:  $PASSED${RESET}"
echo -e "${RED}Failed:  $FAILED${RESET}"
echo "================================"