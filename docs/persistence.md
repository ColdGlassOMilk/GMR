# Persistence

GMR provides systems for saving and loading game data:
- **File** - Read/write files (JSON, text, binary)
- **Storage** - Simple integer key-value storage
- **JSON** - Parse and stringify JSON data
- **Serializable** - Declarative object serialization DSL

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

# Write to data (save files) - root: :data is implied for writes
File.write_json("save.json", save_data)
```

### Text Files

```ruby
# Write text (writes to :data by default)
File.write_text("notes.txt", "Some text content")

# Read text
content = File.read_text("notes.txt", root: :data)
```

### JSON Files

```ruby
# Write JSON (writes to :data by default)
save_data = {
  level: 5,
  score: 1000,
  name: "Alice",
  inventory: ["sword", "shield"]
}
File.write_json("save.json", save_data)

# Read JSON (returns Hash)
loaded = File.read_json("save.json", root: :data)
puts loaded["level"]  # 5

# Pretty-print for human-readability
File.write_json("debug.json", data, pretty: true)
```

### Binary Files

```ruby
# Write binary (root: :data implied)
bytes = [0x89, 0x50, 0x4E, 0x47].pack("C*")
File.write_bytes("data.bin", bytes)

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
File.write_json("saves/slot1.json", data)
File.write_json("profiles/player1.json", profile)
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
File.write_text("/absolute/path", "")         # Rejected
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

## JSON

`GMR::JSON` provides direct JSON parsing and stringifying utilities.

### Parsing JSON

```ruby
# Parse JSON string to Ruby object
data = JSON.parse('{"name": "Player", "health": 100}')
puts data["name"]    # "Player"
puts data["health"]  # 100

# Parse arrays
items = JSON.parse('["sword", "shield", "potion"]')
puts items[0]        # "sword"

# Invalid JSON raises an error
begin
  JSON.parse('not valid json')
rescue => e
  puts "Parse error: #{e.message}"
end
```

### Stringifying Objects

```ruby
# Convert Ruby object to JSON string
player = { name: "Hero", level: 5, items: ["sword", "shield"] }
json = JSON.stringify(player)
# '{"name":"Hero","level":5,"items":["sword","shield"]}'

# Pretty-print with indentation
json = JSON.stringify(player, true)
# {
#   "name": "Hero",
#   "level": 5,
#   "items": ["sword", "shield"]
# }
```

### Use Cases

JSON is useful when you need fine-grained control over serialization:

```ruby
# Manual serialization
def export_level
  data = {
    version: 1,
    tiles: @tilemap.tiles,
    entities: @entities.map { |e| e.to_h }
  }
  JSON.stringify(data, true)
end

# Custom parsing with validation
def import_level(json_str)
  data = JSON.parse(json_str)
  raise "Invalid version" unless data["version"] == 1
  load_tiles(data["tiles"])
  load_entities(data["entities"])
end
```

## Serializable

`GMR::Serializable` provides a declarative DSL for object serialization. Include the module and define which fields to serialize.

### Basic Usage

```ruby
include GMR

class Player
  include Serializable

  serializable do
    field :name
    field :health, default: 100
    field :level, default: 1
  end

  attr_accessor :name, :health, :level
end

# Create and serialize
player = Player.new
player.name = "Hero"
player.health = 85
player.level = 5

data = player.serialize           # Returns Hash
json = player.to_json             # Returns JSON string
json = player.to_json(true)       # Pretty-printed JSON

# Deserialize
loaded = Player.deserialize(data)       # From Hash
loaded = Player.from_json(json)         # From JSON string
```

### Engine Object Serialization

Engine objects like `Vec2`, `Rect`, and `Transform2D` automatically serialize with a `_type` field for reconstruction:

```ruby
vec = Vec2.new(100, 200)
vec.to_h  # { "_type" => "Vec2", "x" => 100.0, "y" => 200.0 }

rect = Rect.new(10, 20, 100, 50)
rect.to_h  # { "_type" => "Rect", "x" => 10.0, "y" => 20.0, "w" => 100.0, "h" => 50.0 }

transform = Transform2D.new
transform.x = 100
transform.to_h  # { "_type" => "Transform2D", "x" => 100.0, "y" => 0.0, ... }
```

These are automatically handled by Serializable:

```ruby
class Entity
  include Serializable

  serializable do
    field :position   # Vec2 serializes automatically
    field :bounds     # Rect serializes automatically
  end

  attr_accessor :position, :bounds
end

entity = Entity.new
entity.position = Vec2.new(100, 200)
entity.bounds = Rect.new(0, 0, 32, 32)

data = entity.serialize
# {
#   "position" => { "_type" => "Vec2", "x" => 100.0, "y" => 200.0 },
#   "bounds" => { "_type" => "Rect", "x" => 0.0, "y" => 0.0, "w" => 32.0, "h" => 32.0 }
# }

# Deserialize reconstructs engine objects
loaded = Entity.deserialize(data)
loaded.position.x  # 100.0
```

### Nested Serializable Objects

Use the `type` option to deserialize nested Serializable classes:

```ruby
class Inventory
  include Serializable

  serializable do
    field :slots, default: []
    field :gold, default: 0
  end

  attr_accessor :slots, :gold
end

class Player
  include Serializable

  serializable do
    field :name
    field :inventory, type: Inventory
  end

  attr_accessor :name, :inventory
end

player = Player.new
player.name = "Hero"
player.inventory = Inventory.new
player.inventory.slots = ["sword", "shield"]
player.inventory.gold = 500

json = player.to_json(true)
loaded = Player.from_json(json)
loaded.inventory.gold  # 500
```

### Arrays of Objects

Arrays are serialized element by element:

```ruby
class GameState
  include Serializable

  serializable do
    field :enemies, type: :array  # Mark as array for proper handling
    field :checkpoints
  end

  attr_accessor :enemies, :checkpoints
end

state = GameState.new
state.checkpoints = [Vec2.new(100, 0), Vec2.new(200, 0)]
state.serialize
# { "checkpoints" => [{ "_type" => "Vec2", ... }, { "_type" => "Vec2", ... }] }
```

### Post-Deserialization Hook

Define `_after_deserialize` for post-load initialization:

```ruby
class Player
  include Serializable

  serializable do
    field :name
    field :sprite_name
  end

  attr_accessor :name, :sprite_name, :sprite

  def _after_deserialize
    # Reload resources that can't be serialized
    @sprite = Sprite.new(@sprite_name)
  end
end
```

### Default Values

Fields with defaults use those values when missing from data:

```ruby
class Settings
  include Serializable

  serializable do
    field :volume, default: 100
    field :difficulty, default: 1
    field :show_hints, default: 1
  end

  attr_accessor :volume, :difficulty, :show_hints
end

# Empty hash uses all defaults
settings = Settings.deserialize({})
settings.volume      # 100
settings.difficulty  # 1
```

## Choosing Between File and Storage

| Use Storage | Use File |
|-------------|----------|
| Single integers | Complex objects |
| Settings | Save game state |
| High scores | Player profiles |
| Flags (0/1) | Level data |
| Counters | Strings or arrays |

## Complete Save System Example

Using `Serializable` for clean, declarative save data:

```ruby
include GMR

class PlayerData
  include Serializable

  serializable do
    field :name
    field :position       # Vec2 auto-serializes
    field :health, default: 100
    field :inventory, default: []
  end

  attr_accessor :name, :position, :health, :inventory
end

class GameSave
  include Serializable

  serializable do
    field :version, default: 1
    field :timestamp
    field :player, type: PlayerData
    field :level
    field :score, default: 0
    field :flags, default: {}
  end

  attr_accessor :version, :timestamp, :player, :level, :score, :flags
end

class SaveManager
  SAVE_SLOTS = 3

  def initialize
    @current_slot = Storage.get(:last_slot, 1)
  end

  def save(slot, game_state)
    save_data = GameSave.new
    save_data.timestamp = Time.elapsed
    save_data.level = game_state.current_level
    save_data.score = game_state.score
    save_data.flags = game_state.story_flags

    save_data.player = PlayerData.new
    save_data.player.name = game_state.player.name
    save_data.player.position = Vec2.new(
      game_state.player.transform.x,
      game_state.player.transform.y
    )
    save_data.player.health = game_state.player.health
    save_data.player.inventory = game_state.player.inventory

    File.write_json("saves/slot#{slot}.json", save_data.serialize)
    Storage[:last_slot] = slot
  end

  def load(slot)
    path = "saves/slot#{slot}.json"
    return nil unless File.exists?(path, root: :data)

    data = File.read_json(path, root: :data)
    return nil if data["deleted"]

    GameSave.deserialize(data)
  end

  def slot_exists?(slot)
    File.exists?("saves/slot#{slot}.json", root: :data)
  end

  def delete_slot(slot)
    File.write_json("saves/slot#{slot}.json", { deleted: true })
  end

  def list_saves
    (1..SAVE_SLOTS).map do |slot|
      next nil unless slot_exists?(slot)
      save = load(slot)
      next nil unless save
      { slot: slot, level: save.level, score: save.score }
    end.compact
  end
end

# Usage
@save_manager = SaveManager.new

# Save current game
@save_manager.save(1, @game_state)

# Load game
if save = @save_manager.load(1)
  @player.transform.x = save.player.position.x
  @player.transform.y = save.player.position.y
  @player.health = save.player.health
  @current_level = save.level
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
- [Math](math.md) - Vec2, Vec3, Rect classes with `to_h` serialization
- [Graphics](graphics.md) - Transform2D with `to_h` serialization
