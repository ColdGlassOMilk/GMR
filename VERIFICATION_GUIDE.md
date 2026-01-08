# Error Handling & Debugging Fixes - Verification Guide

## Changes Made

### Fix #1: Variable Inspector Direct Clear
**File:** [src/debug/variable_inspector.cpp:280](src/debug/variable_inspector.cpp#L280)
- Reverted `safe_clear_exception()` back to direct `mrb->exc = nullptr`
- Prevents cascading exceptions during debugger expression evaluation
- **Impact:** Debugger expression errors no longer corrupt state

### Fix #2: Protected Exception Capture
**File:** [src/scripting/helpers.cpp:75-138](src/scripting/helpers.cpp#L75-L138)
- Added `mrb_protect_error()` wrappers for `message()` and `backtrace()` calls
- Graceful fallback if exception methods fail
- Secondary exceptions are cleared to prevent corruption
- **Impact:** Stack traces are always captured, even when exception objects are malformed

### Fix #3: Enhanced Debug Logging
**Files:**
- [src/scripting/helpers.cpp:107-109, 122-124](src/scripting/helpers.cpp#L107-L109)
- Logs warnings when exception extraction fails (debug builds only)
- **Impact:** Helps diagnose future exception capture issues

### Fix #4: Output Capture Diagnostics
**File:** [src/scripting/loader.cpp:567-572](src/scripting/loader.cpp#L567-L572)
- Detects if REPL output capture is active during game errors
- Warns if errors might be swallowed by capture system
- **Impact:** Identifies if REPL is hiding error output

## Build Instructions

Please build the project with:
```bash
python build.py native
```

For debug builds with full diagnostics:
```bash
python build.py native --debug
```

## Verification Tests

### Test 1: Basic Script Error Stack Traces ✓

**Procedure:**
1. Edit `game/scripts/main.rb`
2. Add at the top: `raise "Test error from main.rb"`
3. Run the game
4. Check stderr output

**Expected Results:**
- ✅ Full stack trace printed to stderr
- ✅ Error message: "RuntimeError: Test error from main.rb"
- ✅ File and line number shown: `game/scripts/main.rb:1`
- ✅ NDJSON event emitted (check IDE or stdout)
- ✅ No "[DEBUG] Warning" messages about failed extraction

**If Fails:**
- Check if stderr is being redirected
- Verify GMR_DEBUG_ENABLED is defined in debug builds
- Look for "[DEBUG] Warning" messages indicating extraction failures

---

### Test 2: Nested Function Stack Traces ✓

**Procedure:**
1. Copy `test_error_handling.rb` to `game/scripts/`
2. Edit `game/scripts/main.rb` to load it:
   ```ruby
   require_relative 'test_error_handling'
   test_nested_error  # Uncomment in test file
   ```
3. Run the game

**Expected Results:**
- ✅ Stack trace shows all three levels: `level_3` → `level_2` → `level_1`
- ✅ Each frame shows file and line number
- ✅ Full backtrace array in NDJSON event

**If Fails:**
- Check if backtrace method failed (look for "[DEBUG] Warning" about backtrace)
- Verify mruby backtrace is enabled in build config

---

### Test 3: Debugger Expression Errors ✓

**Prerequisites:** IDE with debug client connected to port 5678

**Procedure:**
1. Set a breakpoint in `game/scripts/main.rb`
2. Run game in debug mode
3. When paused, evaluate invalid expression in IDE: `nonexistent_variable.foo`
4. Check IDE response and game stderr

**Expected Results:**
- ✅ IDE receives error JSON: `{"type":"Error","value":"..."}`
- ✅ Debugger remains responsive (not crashed)
- ✅ Can continue execution after error
- ✅ No "[DEBUG] WARNING: Output capture is active" message

**If Fails:**
- Game crashes → Variable inspector fix didn't work
- No error in IDE → Check debug server connection
- Corrupted error message → Check JSON escaping

---

### Test 4: Hot Reload with Syntax Errors ✓

**Procedure:**
1. Start game with valid `game/scripts/main.rb`
2. While running, edit file to add syntax error:
   ```ruby
   def broken_syntax
     # Missing end
   ```
3. Save file (triggers hot reload)
4. Check stderr output

**Expected Results:**
- ✅ Syntax error displayed in stderr immediately
- ✅ Error message shows file and line
- ✅ NDJSON error event emitted
- ✅ Game shows "Script error - check console" message
- ✅ Fix error and save → hot reload succeeds, game continues

**If Fails:**
- No error shown → Check file watcher is detecting changes
- Error shown but hot reload doesn't retry → Expected behavior (need to fix and save again)

---

### Test 5: Hot Reload with Runtime Errors ✓

**Procedure:**
1. Start game with valid script
2. Edit to add runtime error in `update()`:
   ```ruby
   def update(dt)
     raise "Runtime error in update"
   end
   ```
3. Save file

**Expected Results:**
- ✅ Hot reload succeeds (code is syntactically valid)
- ✅ Error occurs on next frame
- ✅ Full stack trace shown in stderr
- ✅ Error includes "Runtime error in update" message
- ✅ Game continues running (error state)

---

### Test 6: Output Capture Isolation ✓

**Procedure:**
1. Open in-game console (toggle key)
2. Type and evaluate: `raise "Console error"`
3. Check console output
4. While console open, trigger error in game loop (via hot reload)
5. Check stderr

**Expected Results:**
- ✅ Console error shows in console UI
- ✅ Game error shows in stderr (not captured)
- ✅ No "[DEBUG] WARNING: Output capture is active during error" (console errors are expected to be captured)

**If Fails:**
- Game errors captured → Output capture leaking into game loop
- Warning message appears for game errors → Indicates capture system needs fixing

---

### Test 7: Cascading Exception Protection ✓

**Advanced Test - Only if you have custom exception classes**

**Procedure:**
1. Create custom exception with broken `message` method:
   ```ruby
   class BrokenException < StandardError
     def message
       raise "Message method is broken!"
     end
   end

   raise BrokenException.new("Original message")
   ```
2. Run game

**Expected Results:**
- ✅ Error captured with class name as fallback: "BrokenException"
- ✅ "[DEBUG] Warning: Failed to extract exception message" in stderr (debug builds)
- ✅ No crash or corruption
- ✅ Secondary exception cleared automatically

---

## Success Criteria Summary

All fixes are successful if:

1. ✅ **Stack traces visible** - All script errors show full backtrace in stderr
2. ✅ **Debugger stable** - Expression evaluation errors don't crash or corrupt
3. ✅ **Hot reload errors visible** - Both syntax and runtime errors are reported
4. ✅ **NDJSON events complete** - IDE receives full error information
5. ✅ **No cascading failures** - Protected capture prevents secondary exceptions
6. ✅ **Diagnostic warnings work** - Debug builds show helpful warnings when extraction fails

## Common Issues & Solutions

### Issue: No stack traces at all
- **Cause:** mruby backtrace disabled in build
- **Solution:** Check mruby build config, ensure backtrace is enabled

### Issue: Partial stack traces (only class name)
- **Cause:** `message()` method failed, fallback working correctly
- **Solution:** Check "[DEBUG] Warning" output, may indicate exception object corruption

### Issue: "[DEBUG] WARNING: Output capture is active"
- **Cause:** REPL capture leaking into game loop
- **Solution:** Check `repl::begin_capture()` / `end_capture()` pairing in console code

### Issue: Debugger becomes unresponsive after expression error
- **Cause:** Variable inspector fix not applied
- **Solution:** Verify [src/debug/variable_inspector.cpp:280](src/debug/variable_inspector.cpp#L280) uses `mrb->exc = nullptr`

### Issue: Hot reload not detecting changes
- **Cause:** File watcher issue (not related to these fixes)
- **Solution:** Check loader timestamp polling, verify file system access

## Notes

- Protected exception capture adds minimal overhead (only on error path)
- Debug logging only active in `GMR_DEBUG_ENABLED` builds
- All fixes are cross-platform compatible (Windows, Linux, macOS, Emscripten)
- `mrb_protect_error` is available in both native and web builds
