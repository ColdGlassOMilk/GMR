# Persistence

GMR provides two systems for saving data: File for complex data (save states, level data, profiles) and Storage for simple integers (settings, scores, flags).

## File System

`GMR::File` reads and writes files with automatic cross-platform path handling.

### Logical Roots

Files use logical roots that map to different locations:

| Root | Native Path | Web Path | Access |
|------|-------------|----------|--------|
| `:assets` | `game/assets/` | `/assets/` | Read-only |
| `:data` | `game/data/` | `/data/` (IndexedDB) | Read/write |

```ruby
# Read from assets (game content)
config = File.read_json("config.json", root: :assets)

# Write to data (save files)
File.write_json("save.json", save_data, root: :data)
```

### Text Files

```ruby
# Write text
File.write_text("notes.txt", "Some text content", root: :data)

# Read text
content = File.read_text("notes.txt", root: :data)
```

### JSON Files

```ruby
# Write JSON
save_data = {
  level: 5,
  score: 1000,
  name: "Alice",
  inventory: ["sword", "shield"]
}
File.write_json("save.json", save_data, root: :data)

# Read JSON (returns Hash)
loaded = File.read_json("save.json", root: :data)
puts loaded["level"]  # 5

# Pretty-print for debugging
File.write_json("debug.json", data, root: :data, pretty: true)
```

### Binary Files

```ruby
# Write binary
bytes = [0x89, 0x50, 0x4E, 0x47].pack("C*")
File.write_bytes("data.bin", bytes, root: :data)

# Read binary
loaded = File.read_bytes("data.bin", root: :data)
```

### File Queries

```ruby
# Check existence
if File.exists?("save.json", root: :data)
  load_game
else
  start_new_game
end

# List files in directory
saves = File.list_files("saves", root: :data)
# => ["slot1.json", "slot2.json", "slot3.json"]
```

### Subdirectories

Subdirectories are created automatically:

```ruby
# Creates saves/ directory if needed
File.write_json("saves/slot1.json", data, root: :data)
File.write_json("profiles/player1.json", profile, root: :data)
```

### Platform Behavior

**Native builds:**
- Writes persist immediately to disk
- Hot-reload friendly (can edit asset files while game runs)

**Web builds:**
- `:assets` files are preloaded from WASM package (read-only)
- `:data` writes go to IndexedDB via IDBFS
- Data survives page refreshes and browser restarts

### Security

The following are enforced:

- Directory traversal (`..`) is rejected
- Absolute paths are rejected
- Writing to `:assets` raises an error
- Invalid characters in paths are rejected on Windows

```ruby
# These will fail:
File.read_text("../etc/passwd", root: :data)  # Rejected
File.write_text("/absolute/path", "", root: :data)  # Rejected
File.write_text("file.txt", "", root: :assets)  # Assets are read-only
```

## Storage

`GMR::Storage` provides simple integer key-value storage for settings, scores, and flags.

### Basic Usage

```ruby
# Set and get
Storage.set(:high_score, 1000)
score = Storage.get(:high_score)          # => 1000
volume = Storage.get(:volume, 80)          # => 80 (default if missing)

# Shorthand syntax
Storage[:volume] = 80
volume = Storage[:volume]
```

### Increment/Decrement

```ruby
Storage.increment(:play_count)              # Start at 1, then 2, 3...
Storage.increment(:total_score, 100)        # Add 100 to total
Storage.decrement(:lives, 1)                # Subtract 1
```

### Existence and Deletion

```ruby
if Storage.has_key?(:high_score)
  show_high_score
end

Storage.delete(:high_score)                 # Remove key
```

### When to Use Storage

Storage is best for:
- Settings (volume, difficulty, etc.)
- High scores and achievements
- Counters (play count, deaths, etc.)
- Boolean flags stored as 0/1
- Unlockables

```ruby
# Settings
Storage[:master_volume] = 80
Storage[:sfx_volume] = 100
Storage[:difficulty] = 2  # 0=easy, 1=normal, 2=hard

# Achievements as flags
Storage[:beat_tutorial] = 1
Storage[:found_secret] = 1

# Statistics
Storage.increment(:total_deaths)
Storage.increment(:enemies_killed)
```

### Limitations

- **Integer values only** - Use File for strings, arrays, complex data
- **Maximum ~1000 keys** - Hash collision limit
- **Zero means missing** - Can't store actual zero values reliably

## Choosing Between File and Storage

| Use Storage | Use File |
|-------------|----------|
| Single integers | Complex objects |
| Settings | Save game state |
| High scores | Player profiles |
| Flags (0/1) | Level data |
| Counters | Strings or arrays |

## Complete Save System Example

```ruby
include GMR

class SaveManager
  SAVE_SLOTS = 3

  def initialize
    @current_slot = Storage.get(:last_slot, 1)
  end

  def save(slot, game_state)
    data = {
      version: 1,
      timestamp: Time.elapsed,
      player: {
        x: game_state.player.transform.x,
        y: game_state.player.transform.y,
        health: game_state.player.health,
        inventory: game_state.player.inventory
      },
      level: game_state.current_level,
      score: game_state.score,
      flags: game_state.story_flags
    }

    File.write_json("saves/slot#{slot}.json", data, root: :data)
    Storage[:last_slot] = slot
  end

  def load(slot)
    path = "saves/slot#{slot}.json"
    return nil unless File.exists?(path, root: :data)

    data = File.read_json(path, root: :data)
    migrate_save(data) if data["version"] < 1
    data
  end

  def slot_exists?(slot)
    File.exists?("saves/slot#{slot}.json", root: :data)
  end

  def delete_slot(slot)
    # File deletion not directly supported, write empty/marker
    File.write_json("saves/slot#{slot}.json", { deleted: true }, root: :data)
  end

  def list_saves
    (1..SAVE_SLOTS).map do |slot|
      if slot_exists?(slot)
        data = load(slot)
        next nil if data["deleted"]
        {
          slot: slot,
          level: data["level"],
          score: data["score"]
        }
      end
    end.compact
  end

  private

  def migrate_save(data)
    # Handle old save format migrations
  end
end

# Usage
@save_manager = SaveManager.new

# Save current game
@save_manager.save(1, @game_state)

# Load game
if save_data = @save_manager.load(1)
  restore_game(save_data)
end
```

## Settings Example

```ruby
include GMR

class Settings
  DEFAULTS = {
    master_volume: 100,
    music_volume: 80,
    sfx_volume: 100,
    fullscreen: 0,
    difficulty: 1
  }

  def initialize
    @values = {}
    DEFAULTS.each do |key, default|
      @values[key] = Storage.get(key, default)
    end
  end

  def get(key)
    @values[key]
  end

  def set(key, value)
    @values[key] = value
    Storage[key] = value
    apply_setting(key, value)
  end

  private

  def apply_setting(key, value)
    case key
    when :master_volume
      # Update all audio volumes
    when :fullscreen
      Window.fullscreen = value == 1
    end
  end
end

# Usage
@settings = Settings.new
@settings.set(:master_volume, 70)
volume = @settings.get(:master_volume)
```

## See Also

- [Audio](audio.md) - Saving audio settings
- [Engine Model](engine-model.md) - Platform behavior differences
