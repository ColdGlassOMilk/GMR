module MapLoader
  CHAR_TO_TILE = {
    '#' => Tiles::GROUND_TOP_CENTER,
    '*' => Tiles::GROUND_MID_CENTER,
    '-' => Tiles::THIN_CENTER
  }

  # Decorations: character => type symbol
  CHAR_TO_DECORATION = {
    'S' => :shop
  }

  def self.load(filename)
    content = File.read_text("maps/#{filename}", root: :assets)
    lines = content.split("\n").reject { |l| l.strip.empty? }

    height = lines.size
    width = lines.map(&:size).max
    spawn = nil
    tiles = []
    decorations = []

    lines.each_with_index do |line, y|
      row = []
      width.times do |x|
        char = line[x] || ' '
        if char == 'x'
          spawn = { x: x, y: y }
          row << nil
        elsif CHAR_TO_DECORATION[char]
          decorations << { type: CHAR_TO_DECORATION[char], x: x, y: y }
          row << nil
        else
          row << CHAR_TO_TILE[char]
        end
      end
      tiles << row
    end

    { tiles: tiles, spawn: spawn, decorations: decorations, width: width, height: height }
  end
end
