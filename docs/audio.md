# Audio

GMR provides two audio systems: Sound for short sound effects, and Music for streamed background music.

## Sound Effects

Use `Audio::Sound` for short audio like jumps, gunshots, and UI clicks.

### Loading Sounds

```ruby
include GMR

def init
  # Basic load
  @jump_sfx = Audio::Sound.load("sfx/jump.wav")

  # Load with configuration
  @coin_sfx = Audio::Sound.load("sfx/coin.ogg", volume: 0.5, pitch: 1.2)
end
```

### Playing Sounds

```ruby
def update(dt)
  @jump_sfx.play if Input.action_pressed?(:jump)
  @coin_sfx.play if player_collected_coin?
end
```

Each `play` call starts a new instance. The same sound can play multiple times simultaneously.

### Sound Properties

```ruby
@sound.volume = 0.8   # 0.0 (silent) to 1.0 (full)
@sound.pitch = 1.5    # 0.5 (half speed) to 2.0 (double speed)
@sound.pan = 0.0      # 0.0 (left) to 1.0 (right), 0.5 is center

# Read properties
vol = @sound.volume
```

### Playback Control

```ruby
@sound.play           # Play from beginning
@sound.stop           # Stop playback
@sound.pause          # Pause at current position
@sound.resume         # Resume from pause
@sound.playing?       # Check if currently playing
```

### Positional Audio

Use pan for spatial feedback:

```ruby
def play_sound_at(sound, world_x)
  # Convert world position to screen position
  screen_x = @camera.world_to_screen(Mathf::Vec2.new(world_x, 0)).x

  # Map to pan (0.0 = left, 1.0 = right)
  sound.pan = Mathf.clamp(screen_x / Window.width, 0.0, 1.0)
  sound.play
end
```

### Pitch Variation

Vary pitch for repeated sounds to avoid repetition fatigue:

```ruby
def play_footstep
  @footstep.pitch = Mathf.random_float(0.9, 1.1)
  @footstep.play
end
```

## Music Streaming

Use `Audio::Music` for background music and longer audio.

### Loading Music

```ruby
def init
  @music = Audio::Music.load("music/level1.ogg",
    volume: 0.6,
    loop: true
  )
  @music.play
end
```

### Music Properties

```ruby
@music.volume = 0.5   # Volume (0.0-1.0)
@music.pitch = 0.8    # Pitch/speed (0.5-2.0)
@music.pan = 0.5      # Stereo pan (0.0-1.0)
@music.loop = true    # Enable/disable looping

# Read properties
is_looping = @music.loop
```

### Playback Control

```ruby
@music.play           # Start from current position
@music.stop           # Stop and reset to beginning
@music.pause          # Pause at current position
@music.resume         # Resume from pause
```

### State Queries

```ruby
@music.playing?       # Is currently playing?
@music.loaded?        # Is music loaded and valid?
```

### Time Control

Music supports seeking and position queries:

```ruby
@music.seek(30.0)            # Jump to 30 seconds
length = @music.length       # Total length in seconds
pos = @music.position        # Current position in seconds

# Progress percentage
progress = @music.position / @music.length
```

### Progress Display

```ruby
def draw_music_progress
  progress = @music.position / @music.length
  bar_width = 200

  Graphics.draw_rect(10, 10, bar_width, 10, :dark_gray)
  Graphics.draw_rect(10, 10, bar_width * progress, 10, :green)

  time_text = "#{@music.position.to_i} / #{@music.length.to_i}"
  Graphics.draw_text(time_text, 10, 25, 16, :white)
end
```

## Dynamic Music

### Switching Tracks

```ruby
class MusicController
  def initialize
    @tracks = {
      menu: Audio::Music.load("music/menu.ogg", volume: 0.6, loop: true),
      explore: Audio::Music.load("music/explore.ogg", volume: 0.5, loop: true),
      combat: Audio::Music.load("music/combat.ogg", volume: 0.7, loop: true),
      boss: Audio::Music.load("music/boss.ogg", volume: 0.8, loop: true)
    }
    @current = nil
  end

  def switch_to(track_name)
    return if @current == @tracks[track_name]

    @current&.stop
    @current = @tracks[track_name]
    @current.play
  end
end

# Usage
@music_controller = MusicController.new
@music_controller.switch_to(:menu)
```

### Crossfading

```ruby
def fade_to(track_name, duration: 1.0)
  new_track = @tracks[track_name]
  return if @current == new_track

  # Fade out current
  if @current
    Tween.to(@current, :volume, 0.0, duration: duration * 0.5)
      .on_complete do
        @current.stop
        @current.volume = 0.6  # Reset for next time
      end
  end

  # Fade in new
  new_track.volume = 0.0
  new_track.play
  Tween.to(new_track, :volume, 0.6, duration: duration * 0.5)

  @current = new_track
end
```

### Layered Music

Create dynamic soundscapes by layering tracks:

```ruby
class LayeredMusic
  def initialize
    @base = Audio::Music.load("music/base.ogg", volume: 0.6, loop: true)
    @drums = Audio::Music.load("music/drums.ogg", volume: 0.0, loop: true)
    @melody = Audio::Music.load("music/melody.ogg", volume: 0.0, loop: true)

    # Start all tracks synchronized
    @base.play
    @drums.play
    @melody.play
  end

  def set_intensity(level)
    # level: 0.0 (calm) to 1.0 (intense)
    @drums.volume = Mathf.lerp(0.0, 0.7, level)
    @melody.volume = Mathf.lerp(0.0, 0.8, Mathf.clamp(level - 0.3, 0.0, 1.0))
  end
end
```

## Audio Settings

Provide user-configurable volume:

```ruby
class AudioSettings
  def initialize
    @master_volume = Storage.get(:master_volume, 100) / 100.0
    @music_volume = Storage.get(:music_volume, 80) / 100.0
    @sfx_volume = Storage.get(:sfx_volume, 100) / 100.0
  end

  def set_master_volume(volume)
    @master_volume = Mathf.clamp(volume, 0.0, 1.0)
    Storage.set(:master_volume, (@master_volume * 100).to_i)
  end

  def set_music_volume(volume)
    @music_volume = Mathf.clamp(volume, 0.0, 1.0)
    Storage.set(:music_volume, (@music_volume * 100).to_i)
  end

  def set_sfx_volume(volume)
    @sfx_volume = Mathf.clamp(volume, 0.0, 1.0)
    Storage.set(:sfx_volume, (@sfx_volume * 100).to_i)
  end

  def effective_music_volume
    @music_volume * @master_volume
  end

  def effective_sfx_volume
    @sfx_volume * @master_volume
  end
end

# Usage
@audio_settings = AudioSettings.new
@music.volume = @audio_settings.effective_music_volume
@jump_sfx.volume = @audio_settings.effective_sfx_volume
```

## Best Practices

### Sound Effects

- Use WAV for short, frequently-played sounds (lowest latency)
- Use OGG for longer sound effects to save memory
- Keep individual files under 1-2 seconds when possible
- Vary pitch slightly for repeated sounds
- Use pan for spatial audio cues

### Music

- Use OGG format for best compression/quality ratio
- Keep files under 5MB for fast loading
- Always enable looping for background music
- Preload all tracks during level load, not during gameplay
- Use `seek()` for seamless transitions between song sections

### Performance

- Limit simultaneous sound effects to ~10-15 for best performance
- Music streams from disk; only one track plays at a time
- Unload unused sounds in large games to free memory

## Supported Formats

| Format | Best For | Notes |
|--------|----------|-------|
| WAV | Short SFX | Uncompressed, lowest latency, larger files |
| OGG | Music, longer SFX | Vorbis compression, good quality/size ratio |
| MP3 | Music | Widely compatible, slight quality loss |

## Complete Example

```ruby
include GMR

def init
  # Sound effects
  @jump_sfx = Audio::Sound.load("sfx/jump.wav", volume: 0.7)
  @coin_sfx = Audio::Sound.load("sfx/coin.ogg", volume: 0.5)
  @hurt_sfx = Audio::Sound.load("sfx/hurt.wav", volume: 0.8)

  # Music
  @music = Audio::Music.load("music/gameplay.ogg", volume: 0.6, loop: true)
  @music.play

  @player = Player.new(400, 300)
  @coins = create_coins()
end

def update(dt)
  # Player jump with sound
  if Input.action_pressed?(:jump) && @player.on_ground?
    @player.jump
    @jump_sfx.pitch = Mathf.random_float(0.95, 1.05)
    @jump_sfx.play
  end

  @player.update(dt)

  # Coin collection
  @coins.each do |coin|
    if colliding?(@player, coin)
      @coins.delete(coin)
      @coin_sfx.play
    end
  end

  # Player damage
  if @player.took_damage_this_frame?
    @hurt_sfx.play
    # Brief music duck
    Tween.to(@music, :volume, 0.3, duration: 0.1)
      .on_complete do
        Tween.to(@music, :volume, 0.6, duration: 0.3)
      end
  end
end

def draw
  Graphics.clear("#1e1e32")
  @player.draw
  @coins.each(&:draw)
end
```

## See Also

- [Persistence](persistence.md) - Saving audio settings
- [Animation](animation.md) - Tweening volume for fades
- [API Reference](api/engine/audio/README.md) - Complete Audio API
