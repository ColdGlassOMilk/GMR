include GMR

def init
  @test_results = []
  @tests_passed = 0
  @tests_failed = 0

  # Run all tests
  test_write_and_read_text
  test_write_and_read_json
  test_write_and_read_json_pretty
  test_read_bytes
  test_exists
  test_path_validation
  test_write_to_assets_rejected
  test_default_root
  test_subdirectory_creation

  # Storage tests
  test_storage_get_set
  test_storage_increment_decrement
  test_storage_has_key
  test_storage_delete
  test_storage_shorthand
end

def update(dt)
end

def draw
  Graphics.clear("#1a1a2e")

  y = 10

  # Header
  Graphics.draw_text("GMR File I/O & Storage Test Suite", 10, y, 24, :white)
  y += 35

  # Summary
  total = @tests_passed + @tests_failed
  status_color = @tests_failed == 0 ? :green : :red
  Graphics.draw_text("Results: #{@tests_passed}/#{total} passed", 10, y, 18, status_color)
  y += 30

  # Test results (scrollable list)
  @test_results.each do |result|
    if result.start_with?("[PASS]")
      Graphics.draw_text(result, 10, y, 14, :green)
    else
      Graphics.draw_text(result, 10, y, 14, :red)
    end
    y += 18

    # Stop if we run out of screen space
    break if y > 520
  end

  # Final status at bottom if all tests fit
  if @tests_failed == 0 && y < 500
    Graphics.draw_text("All tests passed!", 10, 520, 16, :green)
  end
end

# === Test Helpers ===

def test(name)
  begin
    yield
    @tests_passed += 1
    @test_results << "[PASS] #{name}"
  rescue => e
    @tests_failed += 1
    @test_results << "[FAIL] #{name}: #{e.message}"
  end
end

def assert(condition, message = "Assertion failed")
  raise message unless condition
end

def assert_eq(expected, actual, message = nil)
  msg = message || "Expected #{expected.inspect}, got #{actual.inspect}"
  raise msg unless expected == actual
end

def assert_raises(error_class = StandardError)
  yield
  raise "Expected #{error_class} to be raised, but nothing was raised"
rescue error_class
  # Expected exception was raised
rescue => e
  raise "Expected #{error_class}, but got #{e.class}: #{e.message}"
end

# === Tests ===

def test_write_and_read_text
  test("write and read text file") do
    content = "Hello, GMR!\nThis is a test file.\nLine 3."
    File.write_text("test_text.txt", content, root: :data)

    loaded = File.read_text("test_text.txt", root: :data)
    assert_eq content, loaded
  end
end

def test_write_and_read_json
  test("write and read JSON (minified)") do
    data = { "level" => 5, "score" => 1000, "name" => "Player", "inventory" => ["sword", "shield"] }
    File.write_json("test_save.json", data, root: :data)

    loaded = File.read_json("test_save.json", root: :data)
    assert_eq data, loaded
  end
end

def test_write_and_read_json_pretty
  test("write and read JSON (pretty)") do
    data = { "config" => { "volume" => 0.8, "fullscreen" => false } }
    File.write_json("test_config.json", data, root: :data, pretty: true)

    loaded = File.read_json("test_config.json", root: :data)
    assert_eq data, loaded

    # Verify it's actually pretty-printed
    text = File.read_text("test_config.json", root: :data)
    assert text.include?("\n"), "JSON should be pretty-printed (contain newlines)"
  end
end

def test_read_bytes
  test("read and write bytes") do
    # Write binary data with null bytes
    binary_data = "Binary\x00Data\xFF"
    File.write_bytes("test_bytes.dat", binary_data, root: :data)
    bytes = File.read_bytes("test_bytes.dat", root: :data)

    assert bytes.is_a?(String), "read_bytes should return a String"
    assert_eq binary_data.length, bytes.length, "Byte length should match"
  end
end

def test_exists
  test("exists? method") do
    # Create a file
    File.write_text("test_exists.txt", "exists", root: :data)

    # Should exist
    assert File.exists?("test_exists.txt", root: :data), "File should exist"

    # Should not exist
    assert !File.exists?("nonexistent_file.txt", root: :data), "File should not exist"
  end
end

def test_path_validation
  test("path validation rejects directory traversal") do
    assert_raises(ArgumentError) do
      File.read_text("../../../etc/passwd", root: :data)
    end
  end

  test("path validation rejects absolute paths") do
    assert_raises(ArgumentError) do
      File.read_text("/etc/passwd", root: :data)
    end
  end
end

def test_write_to_assets_rejected
  test("write to :assets root is rejected") do
    assert_raises(ArgumentError) do
      File.write_text("test.txt", "content", root: :assets)
    end
  end
end

def test_default_root
  test("default root is :data") do
    File.write_text("test_default_root.txt", "default")
    loaded = File.read_text("test_default_root.txt")
    assert_eq "default", loaded
  end
end

def test_subdirectory_creation
  test("automatic subdirectory creation") do
    File.write_text("saves/slot1.json", '{"level":1}', root: :data)
    loaded = File.read_text("saves/slot1.json", root: :data)
    assert_eq '{"level":1}', loaded
  end
end

# === Storage Tests ===

def test_storage_get_set
  test("Storage get and set") do
    Storage.set(:test_value, 42)
    value = Storage.get(:test_value)
    assert_eq 42, value
  end

  test("Storage default value") do
    # Delete first to ensure it doesn't exist
    Storage.delete(:nonexistent)
    value = Storage.get(:nonexistent, 999)
    assert_eq 999, value
  end
end

def test_storage_increment_decrement
  test("Storage increment") do
    Storage.set(:counter, 10)
    new_value = Storage.increment(:counter)
    assert_eq 11, new_value
    assert_eq 11, Storage.get(:counter)
  end

  test("Storage increment by amount") do
    Storage.set(:score, 100)
    Storage.increment(:score, 50)
    assert_eq 150, Storage.get(:score)
  end

  test("Storage decrement") do
    Storage.set(:lives, 5)
    new_value = Storage.decrement(:lives)
    assert_eq 4, new_value
    assert_eq 4, Storage.get(:lives)
  end

  test("Storage decrement by amount") do
    Storage.set(:gold, 100)
    Storage.decrement(:gold, 30)
    assert_eq 70, Storage.get(:gold)
  end
end

def test_storage_has_key
  test("Storage has_key? for existing key") do
    Storage.set(:exists, 123)
    assert Storage.has_key?(:exists), "Key should exist"
  end

  test("Storage has_key? for non-existing key") do
    Storage.delete(:not_exists)
    assert !Storage.has_key?(:not_exists), "Key should not exist"
  end
end

def test_storage_delete
  test("Storage delete") do
    Storage.set(:to_delete, 100)
    assert Storage.has_key?(:to_delete), "Key should exist before delete"

    Storage.delete(:to_delete)
    assert !Storage.has_key?(:to_delete), "Key should not exist after delete"
  end
end

def test_storage_shorthand
  test("Storage shorthand [] syntax") do
    Storage[:shorthand] = 777
    value = Storage[:shorthand]
    assert_eq 777, value
  end
end
