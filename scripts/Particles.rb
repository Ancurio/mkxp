
class Particle
  def initialize(viewport, bitmap)
    @sprite = Sprite.new(viewport)
    @sprite.bitmap = bitmap
    @sprite.ox = bitmap.width / 2
    @sprite.oy = bitmap.height / 2
    self.x = rand(640)
    self.y = rand(480)
  end

  # Link various things to the sprite
  def x
    @x
  end
  def x=(val)
    @x = val
    width = @sprite.bitmap.width * @sprite.zoom_x
    if @x < -width
      @x = 640 + width
    elsif @x > 640 + width
      @x = -width
    end
    @sprite.x = @x
  end
  def y
    @y
  end
  def y=(val)
    @y = val
    height = @sprite.bitmap.height * @sprite.zoom_y
    if @y < -height
      @y = 480 + height
    elsif @y > 480 + height
      @y = -height
    end
    @sprite.y = @y
  end
  def scale
    @sprite.zoom_x
  end
  def scale=(val)
    @sprite.zoom_x = val
    @sprite.zoom_y = val
  end
  def opacity
    @sprite.opacity
  end
  def opacity=(val)
    @sprite.opacity = val
  end
  def dispose
    @sprite.dispose
    @sprite = nil
  end
end

class Particle_Firefly < Particle
  @@bitmap = RPG::Cache.picture('firefly')

  def initialize(viewport)
    super(viewport, @@bitmap)
    @phase = rand(120)
    @wavelength = rand(120..240)
    @vx = rand(0.2..1.5) * (rand(2) * 2 - 1)
    @vy = rand(0.2..1.5) * (rand(2) * 2 - 1)
    self.scale = rand(0.02..0.08)
  end

  def update
    self.x += @vx
    self.y += @vy
    self.opacity = Math.sin((@phase / @wavelength.to_f) * Math::PI) * 255
    @phase = (@phase + 1) % @wavelength
  end
end

# A layer of moving particle objects, useful for fireflies and shrimp
class ParticleLayer
  def initialize(viewport, klass, count)
    @particles = Array.new(count)
    count.times do |i|
      @particles[i] = klass.new(viewport)
    end
    @last_map_x = $game_map.display_x / 4
    @last_map_y = $game_map.display_y / 4
  end

  def update
    return unless @particles
    map_x = $game_map.display_x / 4
    map_y = $game_map.display_y / 4
    @particles.each do |p|
      p.x += @last_map_x - map_x
      p.y += @last_map_y - map_y
      p.update
    end
    @last_map_x = map_x
    @last_map_y = map_y
  end

  def dispose
    return unless @particles
    @particles.each do |p|
      p.dispose
    end
    @particles = nil
  end
end
