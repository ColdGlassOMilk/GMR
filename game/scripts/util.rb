# Utility functions for GMR demos

# Convert HSV to RGB color array
# h: hue (0-360), s: saturation (0-1), v: value (0-1)
# Returns [r, g, b] with values 0-255
def hue_to_rgb(h, s = 1.0, v = 1.0)
  h = h % 360
  c = v * s
  x = c * (1 - ((h / 60.0) % 2 - 1).abs)
  m = v - c

  r, g, b = case (h / 60).to_i
    when 0 then [c, x, 0]
    when 1 then [x, c, 0]
    when 2 then [0, c, x]
    when 3 then [0, x, c]
    when 4 then [x, 0, c]
    else        [c, 0, x]
  end

  [((r + m) * 255).to_i, ((g + m) * 255).to_i, ((b + m) * 255).to_i]
end

# Lerp (linear interpolation) between two values
def lerp(a, b, t)
  a + (b - a) * t
end

# Clamp a value between min and max
def clamp(value, min_val, max_val)
  [[value, min_val].max, max_val].min
end

# Lerp between two colors
# c1, c2: color arrays [r, g, b] or [r, g, b, a]
# t: interpolation factor (0.0 to 1.0)
def lerp_color(c1, c2, t)
  t = clamp(t, 0.0, 1.0)
  r = lerp(c1[0], c2[0], t).to_i
  g = lerp(c1[1], c2[1], t).to_i
  b = lerp(c1[2], c2[2], t).to_i
  a1 = c1[3] || 255
  a2 = c2[3] || 255
  a = lerp(a1, a2, t).to_i
  [r, g, b, a]
end

# Normalize a 2D vector, returns [nx, ny]
def normalize(x, y)
  len = Math.sqrt(x * x + y * y)
  return [0, 0] if len == 0
  [x / len, y / len]
end

# Random float between min and max
def rand_range(min_val, max_val)
  min_val + GMR::System.random_float * (max_val - min_val)
end

# Ease out quad for smooth deceleration
def ease_out_quad(t)
  1 - (1 - t) * (1 - t)
end

# Ease in out for smooth transitions
def ease_in_out(t)
  t < 0.5 ? 2 * t * t : 1 - (-2 * t + 2) ** 2 / 2
end
