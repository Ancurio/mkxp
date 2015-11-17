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
