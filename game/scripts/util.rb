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
