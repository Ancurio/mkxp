# A scene lightmap object
class Light < RPG::Sprite
  def initialize(viewport, filename, intensity, x, y)
    super(viewport)
    self.lightmap = RPG::Cache.light(filename)
    self.intensity = intensity
    @map_x = x
    @map_y = y
    self.z = 9999
    self.visible = true
    update
  end

  def update
    self.x = @map_x - $game_map.display_x / 4
    self.y = @map_y - $game_map.display_y / 4
  end
end
