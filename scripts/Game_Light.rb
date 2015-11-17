class Game_Light
  attr_accessor :filename
  attr_accessor :intensity
  attr_accessor :x
  attr_accessor :y

  def initialize(filename, intensity, x, y)
    @filename = filename
    @intensity = intensity
    @x = x
    @y = y
  end
end
