# Test script for verifying error handling and debugging fixes
# Place this in game/scripts/ to test, or run portions manually

puts "=== GMR Error Handling Test Suite ==="

# Test 1: Basic exception with stack trace
def test_basic_error
  puts "\n[Test 1] Basic error with stack trace"
  raise "This is a test error"
end

# Test 2: Nested function calls for deeper stack trace
def level_3
  raise "Error from level 3"
end

def level_2
  level_3
end

def level_1
  level_2
end

def test_nested_error
  puts "\n[Test 2] Nested error (should show full stack)"
  level_1
end

# Test 3: Method missing (common error type)
def test_method_missing
  puts "\n[Test 3] Method missing error"
  nonexistent_variable.foo
end

# Test 4: Division by zero
def test_division_error
  puts "\n[Test 4] Division by zero"
  x = 5 / 0
end

# Test 5: Array index out of bounds
def test_array_error
  puts "\n[Test 5] Array access error"
  arr = [1, 2, 3]
  # This won't raise in mruby, but let's try to raise manually
  raise IndexError, "Array index out of bounds"
end

# Uncomment one test at a time to verify error reporting:

# test_basic_error
# test_nested_error
# test_method_missing
# test_division_error
# test_array_error

puts "Test script loaded. Uncomment test calls to run."
puts "Expected: Each error should show full stack trace in stderr"
puts "Expected: NDJSON events should be emitted for IDE"
puts "Expected: No crashes or corrupted error messages"
