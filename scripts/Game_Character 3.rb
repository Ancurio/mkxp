#==============================================================================
# ** Game_Character (part 3)
#------------------------------------------------------------------------------
#  This class deals with characters. It's used as a superclass for the
#  Game_Player and Game_Event classes.
#==============================================================================

class Game_Character
  #--------------------------------------------------------------------------
  # * Move Down
  #     turn_enabled : a flag permits direction change on that spot
  #--------------------------------------------------------------------------
  def move_down(turn_enabled = true)
    # Turn down
    if turn_enabled
      turn_down
    end
    # If passable
    if passable?(@x, @y, 2)
      # Turn down
      turn_down
      # Update coordinates
      @y += 1
      # Increase steps
      increase_steps
    # If impassable
    else
      # Determine if touch event is triggered
      check_event_trigger_touch(@x, @y+1)
    end
  end
  #--------------------------------------------------------------------------
  # * Move Left
  #     turn_enabled : a flag permits direction change on that spot
  #--------------------------------------------------------------------------
  def move_left(turn_enabled = true)
    # Turn left
    if turn_enabled
      turn_left
    end
    # If passable
    if passable?(@x, @y, 4)
      # Turn left
      turn_left
      # Update coordinates
      @x -= 1
      # Increase steps
      increase_steps
    # If impassable
    else
      # Determine if touch event is triggered
      check_event_trigger_touch(@x-1, @y)
    end
  end
  #--------------------------------------------------------------------------
  # * Move Right
  #     turn_enabled : a flag permits direction change on that spot
  #--------------------------------------------------------------------------
  def move_right(turn_enabled = true)
    # Turn right
    if turn_enabled
      turn_right
    end
    # If passable
    if passable?(@x, @y, 6)
      # Turn right
      turn_right
      # Update coordinates
      @x += 1
      # Increase steps
      increase_steps
    # If impassable
    else
      # Determine if touch event is triggered
      check_event_trigger_touch(@x+1, @y)
    end
  end
  #--------------------------------------------------------------------------
  # * Move up
  #     turn_enabled : a flag permits direction change on that spot
  #--------------------------------------------------------------------------
  def move_up(turn_enabled = true)
    # Turn up
    if turn_enabled
      turn_up
    end
    # If passable
    if passable?(@x, @y, 8)
      # Turn up
      turn_up
      # Update coordinates
      @y -= 1
      # Increase steps
      increase_steps
    # If impassable
    else
      # Determine if touch event is triggered
      check_event_trigger_touch(@x, @y-1)
    end
  end
  #--------------------------------------------------------------------------
  # * Move Lower Left
  #--------------------------------------------------------------------------
  def move_lower_left
    # If no direction fix
    unless @direction_fix
      # Face down is facing right or up
      @direction = (@direction == 6 ? 4 : @direction == 8 ? 2 : @direction)
    end
    # When a down to left or a left to down course is passable
    if (passable?(@x, @y, 2) and passable?(@x, @y + 1, 4)) or
       (passable?(@x, @y, 4) and passable?(@x - 1, @y, 2))
      # Update coordinates
      @x -= 1
      @y += 1
      # Increase steps
      increase_steps
    end
  end
  #--------------------------------------------------------------------------
  # * Move Lower Right
  #--------------------------------------------------------------------------
  def move_lower_right
    # If no direction fix
    unless @direction_fix
      # Face right if facing left, and face down if facing up
      @direction = (@direction == 4 ? 6 : @direction == 8 ? 2 : @direction)
    end
    # When a down to right or a right to down course is passable
    if (passable?(@x, @y, 2) and passable?(@x, @y + 1, 6)) or
       (passable?(@x, @y, 6) and passable?(@x + 1, @y, 2))
      # Update coordinates
      @x += 1
      @y += 1
      # Increase steps
      increase_steps
    end
  end
  #--------------------------------------------------------------------------
  # * Move Upper Left
  #--------------------------------------------------------------------------
  def move_upper_left
    # If no direction fix
    unless @direction_fix
      # Face left if facing right, and face up if facing down
      @direction = (@direction == 6 ? 4 : @direction == 2 ? 8 : @direction)
    end
    # When an up to left or a left to up course is passable
    if (passable?(@x, @y, 8) and passable?(@x, @y - 1, 4)) or
       (passable?(@x, @y, 4) and passable?(@x - 1, @y, 8))
      # Update coordinates
      @x -= 1
      @y -= 1
      # Increase steps
      increase_steps
    end
  end
  #--------------------------------------------------------------------------
  # * Move Upper Right
  #--------------------------------------------------------------------------
  def move_upper_right
    # If no direction fix
    unless @direction_fix
      # Face right if facing left, and face up if facing down
      @direction = (@direction == 4 ? 6 : @direction == 2 ? 8 : @direction)
    end
    # When an up to right or a right to up course is passable
    if (passable?(@x, @y, 8) and passable?(@x, @y - 1, 6)) or
       (passable?(@x, @y, 6) and passable?(@x + 1, @y, 8))
      # Update coordinates
      @x += 1
      @y -= 1
      # Increase steps
      increase_steps
    end
  end
  #--------------------------------------------------------------------------
  # * Move at Random
  #--------------------------------------------------------------------------
  def move_random
    case rand(4)
    when 0  # Move down
      move_down(false)
    when 1  # Move left
      move_left(false)
    when 2  # Move right
      move_right(false)
    when 3  # Move up
      move_up(false)
    end
  end
  #--------------------------------------------------------------------------
  # * Move toward Player
  #--------------------------------------------------------------------------
  def move_toward_player
    # Get difference in player coordinates
    sx = @x - $game_player.x
    sy = @y - $game_player.y
    # If coordinates are equal
    if sx == 0 and sy == 0
      return
    end
    # Get absolute value of difference
    abs_sx = sx.abs
    abs_sy = sy.abs
    # If horizontal and vertical distances are equal
    if abs_sx == abs_sy
      # Increase one of them randomly by 1
      rand(2) == 0 ? abs_sx += 1 : abs_sy += 1
    end
    # If horizontal distance is longer
    if abs_sx > abs_sy
      # Move towards player, prioritize left and right directions
      sx > 0 ? move_left : move_right
      if not moving? and sy != 0
        sy > 0 ? move_up : move_down
      end
    # If vertical distance is longer
    else
      # Move towards player, prioritize up and down directions
      sy > 0 ? move_up : move_down
      if not moving? and sx != 0
        sx > 0 ? move_left : move_right
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Move away from Player
  #--------------------------------------------------------------------------
  def move_away_from_player
    # Get difference in player coordinates
    sx = @x - $game_player.x
    sy = @y - $game_player.y
    # If coordinates are equal
    if sx == 0 and sy == 0
      return
    end
    # Get absolute value of difference
    abs_sx = sx.abs
    abs_sy = sy.abs
    # If horizontal and vertical distances are equal
    if abs_sx == abs_sy
      # Increase one of them randomly by 1
      rand(2) == 0 ? abs_sx += 1 : abs_sy += 1
    end
    # If horizontal distance is longer
    if abs_sx > abs_sy
      # Move away from player, prioritize left and right directions
      sx > 0 ? move_right : move_left
      if not moving? and sy != 0
        sy > 0 ? move_down : move_up
      end
    # If vertical distance is longer
    else
      # Move away from player, prioritize up and down directions
      sy > 0 ? move_down : move_up
      if not moving? and sx != 0
        sx > 0 ? move_right : move_left
      end
    end
  end
  #--------------------------------------------------------------------------
  # * 1 Step Forward
  #--------------------------------------------------------------------------
  def move_forward
    case @direction
    when 2
      move_down(false)
    when 4
      move_left(false)
    when 6
      move_right(false)
    when 8
      move_up(false)
    end
  end
  #--------------------------------------------------------------------------
  # * 1 Step Backward
  #--------------------------------------------------------------------------
  def move_backward
    # Remember direction fix situation
    last_direction_fix = @direction_fix
    # Force directino fix
    @direction_fix = true
    # Branch by direction
    case @direction
    when 2  # Down
      move_up(false)
    when 4  # Left
      move_right(false)
    when 6  # Right
      move_left(false)
    when 8  # Up
      move_down(false)
    end
    # Return direction fix situation back to normal
    @direction_fix = last_direction_fix
  end
  #--------------------------------------------------------------------------
  # * Jump
  #     x_plus : x-coordinate plus value
  #     y_plus : y-coordinate plus value
  #--------------------------------------------------------------------------
  def jump(x_plus, y_plus)
    # If plus value is not (0,0)
    if x_plus != 0 or y_plus != 0
      # If horizontal distnace is longer
      if x_plus.abs > y_plus.abs
        # Change direction to left or right
        x_plus < 0 ? turn_left : turn_right
      # If vertical distance is longer, or equal
      else
        # Change direction to up or down
        y_plus < 0 ? turn_up : turn_down
      end
    end
    # Calculate new coordinates
    new_x = @x + x_plus
    new_y = @y + y_plus
    # If plus value is (0,0) or jump destination is passable
    if (x_plus == 0 and y_plus == 0) or passable?(new_x, new_y, 0)
      # Straighten position
      straighten
      # Update coordinates
      @x = new_x
      @y = new_y
      # Calculate distance
      distance = Math.sqrt(x_plus * x_plus + y_plus * y_plus).round
      # Set jump count
      @jump_peak = 10 + distance - @move_speed
      @jump_count = @jump_peak * 2
      # Clear stop count
      @stop_count = 0
    end
  end
  #--------------------------------------------------------------------------
  # * Turn Down
  #--------------------------------------------------------------------------
  def turn_down
    unless @direction_fix
      @direction = 2
      @stop_count = 0
    end
  end
  #--------------------------------------------------------------------------
  # * Turn Left
  #--------------------------------------------------------------------------
  def turn_left
    unless @direction_fix
      @direction = 4
      @stop_count = 0
    end
  end
  #--------------------------------------------------------------------------
  # * Turn Right
  #--------------------------------------------------------------------------
  def turn_right
    unless @direction_fix
      @direction = 6
      @stop_count = 0
    end
  end
  #--------------------------------------------------------------------------
  # * Turn Up
  #--------------------------------------------------------------------------
  def turn_up
    unless @direction_fix
      @direction = 8
      @stop_count = 0
    end
  end
  #--------------------------------------------------------------------------
  # * Turn 90° Right
  #--------------------------------------------------------------------------
  def turn_right_90
    case @direction
    when 2
      turn_left
    when 4
      turn_up
    when 6
      turn_down
    when 8
      turn_right
    end
  end
  #--------------------------------------------------------------------------
  # * Turn 90° Left
  #--------------------------------------------------------------------------
  def turn_left_90
    case @direction
    when 2
      turn_right
    when 4
      turn_down
    when 6
      turn_up
    when 8
      turn_left
    end
  end
  #--------------------------------------------------------------------------
  # * Turn 180°
  #--------------------------------------------------------------------------
  def turn_180
    case @direction
    when 2
      turn_up
    when 4
      turn_right
    when 6
      turn_left
    when 8
      turn_down
    end
  end
  #--------------------------------------------------------------------------
  # * Turn 90° Right or Left
  #--------------------------------------------------------------------------
  def turn_right_or_left_90
    if rand(2) == 0
      turn_right_90
    else
      turn_left_90
    end
  end
  #--------------------------------------------------------------------------
  # * Turn at Random
  #--------------------------------------------------------------------------
  def turn_random
    case rand(4)
    when 0
      turn_up
    when 1
      turn_right
    when 2
      turn_left
    when 3
      turn_down
    end
  end
  #--------------------------------------------------------------------------
  # * Turn Towards Player
  #--------------------------------------------------------------------------
  def turn_toward_player
    # Get difference in player coordinates
    sx = @x - $game_player.x
    sy = @y - $game_player.y
    # If coordinates are equal
    if sx == 0 and sy == 0
      return
    end
    # If horizontal distance is longer
    if sx.abs > sy.abs
      # Turn to the right or left towards player
      sx > 0 ? turn_left : turn_right
    # If vertical distance is longer
    else
      # Turn up or down towards player
      sy > 0 ? turn_up : turn_down
    end
  end
  #--------------------------------------------------------------------------
  # * Turn Away from Player
  #--------------------------------------------------------------------------
  def turn_away_from_player
    # Get difference in player coordinates
    sx = @x - $game_player.x
    sy = @y - $game_player.y
    # If coordinates are equal
    if sx == 0 and sy == 0
      return
    end
    # If horizontal distance is longer
    if sx.abs > sy.abs
      # Turn to the right or left away from player
      sx > 0 ? turn_right : turn_left
    # If vertical distance is longer
    else
      # Turn up or down away from player
      sy > 0 ? turn_down : turn_up
    end
  end
end
