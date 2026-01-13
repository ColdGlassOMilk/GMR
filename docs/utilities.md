# Utilities

GMR provides several utility systems for common game development patterns.

## Timer

Execute code after a delay or at regular intervals without manual bookkeeping.

### One-Shot Timers

```ruby
# Fire once after 2 seconds
Timer.after(2.0) { spawn_enemy }

# Invincibility frames
@player.invincible = true
Timer.after(1.5) { @player.invincible = false }
```

### Repeating Timers

```ruby
# Spawn a wave every 3 seconds
@spawn_timer = Timer.every(3.0) { spawn_wave }

# Stop the timer when done
@spawn_timer.cancel
```

### Named Timers

Use named timers for cancellation from anywhere:

```ruby
# Start a respawn timer
Timer.after(5.0, name: :respawn) { respawn_player }

# Cancel by name (e.g., if player gets revived)
Timer.cancel(:respawn)
```

### Timer Options

```ruby
# Ignore Time.scale (useful for pause menus)
Timer.after(0.5, scaled: false) { show_pause_menu }

# Set initial delay for repeating timer (default: same as interval)
Timer.every(1.0, delay: 0.0) { tick }  # Fire immediately, then every 1s
```

### Timer Instance Methods

```ruby
@timer = Timer.every(1.0) { update_score }

@timer.pause          # Pause the timer
@timer.resume         # Resume after pause
@timer.cancel         # Stop the timer permanently

@timer.active?        # true if running
@timer.cancelled?     # true if cancelled
@timer.elapsed        # Time since last fire
@timer.remaining      # Time until next fire
```

## Random

Deterministic, seedable random number generation.

### Basic Usage

```ruby
# Integer in range (inclusive)
roll = Random.int(1, 6)        # Dice roll: 1-6
index = Random.int(0, 9)       # Random index: 0-9

# Float in range
x = Random.float(-1.0, 1.0)    # Random direction
chance = Random.float          # 0.0 to 1.0 (no args = default range)

# Boolean
if Random.bool
  attack_left
else
  attack_right
end

# Probability check (0.0 to 1.0)
if Random.chance(0.25)         # 25% chance
  critical_hit
end
```

### Array Operations

```ruby
# Pick random element
enemy_type = Random.choose([:goblin, :orc, :troll])
spawn_point = Random.choose(@spawn_points)

# Shuffle in place
deck = Random.shuffle([1, 2, 3, 4, 5])
```

### Weighted Selection

```ruby
# Weights are relative (don't need to sum to 100)
tier = Random.weighted({
  common: 70,
  rare: 25,
  legendary: 5
})

# Weights can be any positive number
direction = Random.weighted({ north: 1, south: 1, east: 2 })  # East twice as likely
```

### Seeding for Determinism

```ruby
# Seed for reproducible sequences (replays, testing)
Random.seed(12345)
puts Random.int(1, 100)  # Always same result with same seed

# Re-randomize from system entropy
Random.seed
```

### Named Streams

Isolate RNG for different systems:

```ruby
# Separate streams for gameplay vs visual effects
Random.seed_stream(:loot, 42)
Random.seed_stream(:particles, 99)

# Use streams independently
loot_roll = Random.stream(:loot).int(1, 100)
particle_dir = Random.stream(:particles).float(-1, 1)

# Stream methods mirror module functions
rng = Random.stream(:combat)
rng.seed(12345)
rng.int(1, 20)
rng.float
rng.bool
rng.chance(0.5)
rng.choose(enemies)
```

## Sequence

Express multi-step behaviors declaratively. Useful for boss patterns, cutscenes, and tutorials.

### Basic Sequence

```ruby
Sequence.run do |s|
  s.call { @boss.telegraph(:attack) }
  s.wait(0.8)
  s.call { @boss.attack }
  s.wait_until { @boss.attack_complete? }
  s.call { @boss.recover }
end
```

### Sequence Steps

| Step | Description |
|------|-------------|
| `s.call { ... }` | Execute a block immediately |
| `s.wait(seconds)` | Wait for a duration |
| `s.wait_until { condition }` | Wait until condition returns true |

### Named Sequences

```ruby
# Name for later cancellation
Sequence.run(:boss_intro) do |s|
  s.call { show_title("BOSS BATTLE") }
  s.wait(2.0)
  s.call { hide_title }
end

# Cancel by name
Sequence.cancel(:boss_intro)
```

### Completion Callbacks

```ruby
Sequence.run do |s|
  s.call { @enemy.die }
  s.wait(0.5)
  s.call { spawn_loot }
end.then { @score += 100 }
```

### Sequence Instance Methods

```ruby
@seq = Sequence.run { |s| ... }

@seq.cancel       # Stop the sequence
@seq.active?      # true if still running
@seq.completed?   # true if finished all steps
```

### Boss Pattern Example

```ruby
def boss_attack_pattern
  Sequence.run(:attack_pattern) do |s|
    # Telegraph
    s.call { @boss.sprite.flash(:red) }
    s.wait(0.5)

    # Attack
    s.call { @boss.fire_laser }
    s.wait_until { @boss.laser_complete? }

    # Cooldown
    s.call { @boss.vulnerable = true }
    s.wait(2.0)
    s.call { @boss.vulnerable = false }

    # Loop by starting new sequence
    s.call { boss_attack_pattern }
  end
end
```

## Signal

Decouple object communication with the observer pattern.

### Emitting Signals

```ruby
class Player
  include Signal

  def take_damage(amount)
    @health -= amount
    emit(:health_changed, @health)
    emit(:died) if @health <= 0
  end
end
```

### Connecting Handlers

```ruby
# Connect to signals
@player.on(:health_changed) { |hp| @hud.update_health(hp) }
@player.on(:died) { @music.play(:game_over) }

# One-shot handler (auto-disconnects after firing once)
@player.once(:died) { show_death_cutscene }
```

### Disconnecting

```ruby
# Store connection ID
@health_connection = @player.on(:health_changed) { |hp| update_bar(hp) }

# Disconnect by ID
@player.off(:health_changed, @health_connection)

# Disconnect all handlers for a signal
@player.off(:health_changed)

# Clear all signals from an object
@player.clear_signals
```

### Query Signals

```ruby
if @player.has_signal?(:died)
  puts "Death handlers registered"
end
```

## Destroyable

Standard lifecycle pattern for game objects.

### Basic Usage

```ruby
class Enemy
  include Destroyable

  def on_destroy
    # Called automatically when destroy is invoked
    @death_sound.play
    spawn_particles
  end

  def take_damage(amount)
    @health -= amount
    destroy if @health <= 0
  end
end

# Mark for destruction
enemy.destroy

# Query state
enemy.destroyed?  # true after destroy called
enemy.alive?      # false after destroy called
```

### GameArray

An Array subclass that automatically removes destroyed objects:

```ruby
@enemies = GameArray.new
@enemies << Enemy.new
@enemies << Enemy.new

# Iterate only alive objects (destroyed ones are auto-removed)
@enemies.each_alive do |enemy|
  enemy.update(dt)
end

# Manual cleanup
@enemies.compact_destroyed!

# Query
@enemies.alive_count        # Number of alive objects
@enemies.all_destroyed?     # true if all objects are destroyed
```

### Combining with Timer

```ruby
class Bullet
  include Destroyable

  def initialize
    # Auto-destroy after 5 seconds
    Timer.after(5.0) { destroy }
  end

  def on_destroy
    Timer.cancel_all(self)  # Clean up any remaining timers
  end
end
```

## See Also

- [Spatial Queries](spatial.md) - Efficient entity lookups by position
- [State Machine](state-machine.md) - State-based behavior with transitions
- [Animation](animation.md) - Tweens for property animation
