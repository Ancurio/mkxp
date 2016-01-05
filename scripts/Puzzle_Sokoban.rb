CORRECT_RAM_POSITIONS = [[80, 59], [79, 63], [80, 65], [83, 59], [85, 61]]

def ram_integrity_check
  result = true
  $game_map.events.each do |key, event|
    if event.name =~ /^sokoram [ABCDE]$/
      if !CORRECT_RAM_POSITIONS.include? [event.x, event.y]
        result = false
        break
      end
    end
  end
  Script.tmp_s1 = result
end
