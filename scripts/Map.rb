def bg(name)
  $game_map.bg_name = name
end

def particles(type)
  $game_map.particles_type = type
end

def ambient(r, g = -1, b = -1)
  g = r if g < 0
  b = r if b < 0
  #$scene.ambient = Color.new(r, g, b)
end

def clear_ambient
  #$scene.ambient = Color.new(255, 255, 255)
end

def add_light(id, file, intensity, x, y)
  #$scene.add_light(id, file, intensity, x, y)
end

def del_light(id)
  #$scene.del_light(id)
end

def clear_lights
  #$scene.clear_lights
end

def fade_in_bulb
  #$scene.fade_in_bulb
end

def clamp_panorama
  $game_map.clamped_panorama = true
end
