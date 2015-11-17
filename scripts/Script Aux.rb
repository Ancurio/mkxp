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
