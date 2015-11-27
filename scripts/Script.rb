module Script
  def self.px
    logpos($game_player.x, $game_player.real_x, $game_player.direction == 6)
  end

  def self.py
    logpos($game_player.y, $game_player.real_y, $game_player.direction == 2)
  end

  # Temporary switch assignment
  def self.tmp_s1=(val)
    $game_switches[TMP_INDEX+0] = val
  end

  def self.tmp_s2=(val)
    $game_switches[TMP_INDEX+1] = val
  end

  def self.tmp_s3=(val)
    $game_switches[TMP_INDEX+2] = val
  end

  def self.tmp_v1=(val)
    $game_variables[TMP_INDEX+0] = val
  end

  def self.tmp_v2=(val)
    $game_variables[TMP_INDEX+1] = val
  end

  def self.tmp_v3=(val)
    $game_variables[TMP_INDEX+2] = val
  end

  private
  TMP_INDEX = 22

  def self.logpos(pos, realpos, positive)
    bigpos = pos * 128
    if realpos < bigpos
      return pos - 1 if positive
    elsif realpos > bigpos
      return pos + 1 if positive
    end
    return pos
  end
end

def has_lightbulb?
  $game_party.item_number(1) > 0
end

def enter_name
  $game_temp.name_calling = true
end

def check_exit(min, max, x: -1, y: -1)
  result = false
  if x >= 0
    if $game_player.x == x
      if $game_player.y >= min && $game_player.y <= max
        result = true
      end
    end
  elsif y >= 0
    if $game_player.y == y
      if $game_player.x >= min && $game_player.x <= max
        result = true
      end
    end
  end
  Script.tmp_s1 = $game_switches[11] ? false : result
end

def bg(name)
  $game_map.bg_name = name
end

def particles(type)
  $game_map.particles_type = type
end

def ambient(r, g, b, gray = 0)
  $game_map.ambient.set(r, g, b, gray)
end

def clear_ambient
  $game_map.ambient.set(0, 0, 0, 0)
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

def clamp_panorama
  $game_map.clamped_panorama = true
end

def wrap_map
  $game_map.wrapping = true
end

# Map specific settings
def green_ambient
  ambient -50, -50, -50
end

# Misc
def watcher_tell_time
  hour = Time.now.hour
  if hour >= 6 && hour < 12
    Script.tmp_v1 = 0
  elsif hour >= 12 && hour < 17
    Script.tmp_v1 = 1
  elsif hour >= 17
    Script.tmp_v1 = 2
  else
    Script.tmp_v1 = 3
  end
end

def plight_start_timer
  $game_oneshot.plight_timer = Time.now
end

def plight_update_timer
  Script.tmp_v1 = ((Time.now - $game_oneshot.plight_timer) / 60).to_i
end
