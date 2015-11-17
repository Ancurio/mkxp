#==============================================================================
# ** Game_Character (part 2)
#------------------------------------------------------------------------------
#  This class deals with characters. It's used as a superclass for the
#  Game_Player and Game_Event classes.
#==============================================================================

class Game_Character
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Branch with jumping, moving, and stopping
    if jumping?
      update_jump
    elsif moving?
      update_move
    else
      update_stop
    end
    # If animation count exceeds maximum value
    # * Maximum value is move speed * 1 taken from basic value 18
    if @anime_count > 18 - @move_speed * 2
      # If stop animation is OFF when stopping
      if not @step_anime and @stop_count > 0
        # Return to original pattern
        @pattern = @original_pattern
      # If stop animation is ON when moving
      else
        # Update pattern
        @pattern = (@pattern + 1) % 4
      end
      # Clear animation count
      @anime_count = 0
    end
    # If waiting
    if @wait_count > 0
      # Reduce wait count
      @wait_count -= 1
      return
    end
    # If move route is forced
    if @move_route_forcing
      # Custom move
      move_type_custom
      return
    end
    # When waiting for event execution or locked
    if @starting or lock?
      # Not moving by self
      return
    end
    # If stop count exceeds a certain value (computed from move frequency)
    if @stop_count > (40 - @move_frequency * 2) * (6 - @move_frequency)
      # Branch by move type
      case @move_type
      when 1  # Random
        move_type_random
      when 2  # Approach
        move_type_toward_player
      when 3  # Custom
        move_type_custom
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (jump)
  #--------------------------------------------------------------------------
  def update_jump
    # Reduce jump count by 1
    @jump_count -= 1
    # Calculate new coordinates
    @real_x = (@real_x * @jump_count + @x * 128) / (@jump_count + 1)
    @real_y = (@real_y * @jump_count + @y * 128) / (@jump_count + 1)
  end
  #--------------------------------------------------------------------------
  # * Update frame (move)
  #--------------------------------------------------------------------------
  def update_move
    # Convert map coordinates from map move speed into move distance
    distance = 2 ** @move_speed
    # If logical coordinates are further down than real coordinates
    if @y * 128 > @real_y
      # Move down
      @real_y = [@real_y + distance, @y * 128].min
    end
    # If logical coordinates are more to the left than real coordinates
    if @x * 128 < @real_x
      # Move left
      @real_x = [@real_x - distance, @x * 128].max
    end
    # If logical coordinates are more to the right than real coordinates
    if @x * 128 > @real_x
      # Move right
      @real_x = [@real_x + distance, @x * 128].min
    end
    # If logical coordinates are further up than real coordinates
    if @y * 128 < @real_y
      # Move up
      @real_y = [@real_y - distance, @y * 128].max
    end
    # If move animation is ON
    if @walk_anime
      # Increase animation count by 1.5
      @anime_count += 1.5
    # If move animation is OFF, and stop animation is ON
    elsif @step_anime
      # Increase animation count by 1
      @anime_count += 1
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (stop)
  #--------------------------------------------------------------------------
  def update_stop
    # If stop animation is ON
    if @step_anime
      # Increase animation count by 1
      @anime_count += 1
    # If stop animation is OFF, but current pattern is different from original
    elsif @pattern != @original_pattern
      # Increase animation count by 1.5
      @anime_count += 1.5
    end
    # When waiting for event execution, or not locked
    # * If lock deals with event execution coming to a halt
    unless @starting or lock?
      # Increase stop count by 1
      @stop_count += 1
    end
  end
  #--------------------------------------------------------------------------
  # * Move Type : Random
  #--------------------------------------------------------------------------
  def move_type_random
    # Branch by random numbers 0-5
    case rand(6)
    when 0..3  # Random
      move_random
    when 4  # 1 step forward
      move_forward
    when 5  # Temporary stop
      @stop_count = 0
    end
  end
  #--------------------------------------------------------------------------
  # * Move Type : Approach
  #--------------------------------------------------------------------------
  def move_type_toward_player
    # Get difference in player coordinates
    sx = @x - $game_player.x
    sy = @y - $game_player.y
    # Get absolute value of difference
    abs_sx = sx > 0 ? sx : -sx
    abs_sy = sy > 0 ? sy : -sy
    # If separated by 20 or more tiles matching up horizontally and vertically
    if sx + sy >= 20
      # Random
      move_random
      return
    end
    # Branch by random numbers 0-5
    case rand(6)
    when 0..3  # Approach player
      move_toward_player
    when 4  # random
      move_random
    when 5  # 1 step forward
      move_forward
    end
  end
  #--------------------------------------------------------------------------
  # * Move Type : Custom
  #--------------------------------------------------------------------------
  def move_type_custom
    # Interrupt if not stopping
    if jumping? or moving?
      return
    end
    # Loop until finally arriving at move command list
    while @move_route_index < @move_route.list.size
      # Acquiring move command
      command = @move_route.list[@move_route_index]
      # If command code is 0 (last part of list)
      if command.code == 0
        # If [repeat action] option is ON
        if @move_route.repeat
          # First return to the move route index
          @move_route_index = 0
        end
        # If [repeat action] option is OFF
        unless @move_route.repeat
          # If move route is forcing
          if @move_route_forcing and not @move_route.repeat
            # Release forced move route
            @move_route_forcing = false
            # Restore original move route
            @move_route = @original_move_route
            @move_route_index = @original_move_route_index
            @original_move_route = nil
          end
          # Clear stop count
          @stop_count = 0
        end
        return
      end
      # During move command (from move down to jump)
      if command.code <= 14
        # Branch by command code
        case command.code
        when 1  # Move down
          move_down
        when 2  # Move left
          move_left
        when 3  # Move right
          move_right
        when 4  # Move up
          move_up
        when 5  # Move lower left
          move_lower_left
        when 6  # Move lower right
          move_lower_right
        when 7  # Move upper left
          move_upper_left
        when 8  # Move upper right
          move_upper_right
        when 9  # Move at random
          move_random
        when 10  # Move toward player
          move_toward_player
        when 11  # Move away from player
          move_away_from_player
        when 12  # 1 step forward
          move_forward
        when 13  # 1 step backward
          move_backward
        when 14  # Jump
          jump(command.parameters[0], command.parameters[1])
        end
        # If movement failure occurs when [Ignore if can't move] option is OFF
        if not @move_route.skippable and not moving? and not jumping?
          return
        end
        @move_route_index += 1
        return
      end
      # If waiting
      if command.code == 15
        # Set wait count
        @wait_count = command.parameters[0] * 2 - 1
        @move_route_index += 1
        return
      end
      # If direction change command
      if command.code >= 16 and command.code <= 26
        # Branch by command code
        case command.code
        when 16  # Turn down
          turn_down
        when 17  # Turn left
          turn_left
        when 18  # Turn right
          turn_right
        when 19  # Turn up
          turn_up
        when 20  # Turn 90° right
          turn_right_90
        when 21  # Turn 90° left
          turn_left_90
        when 22  # Turn 180°
          turn_180
        when 23  # Turn 90° right or left
          turn_right_or_left_90
        when 24  # Turn at Random
          turn_random
        when 25  # Turn toward player
          turn_toward_player
        when 26  # Turn away from player
          turn_away_from_player
        end
        @move_route_index += 1
        return
      end
      # If other command
      if command.code >= 27
        # Branch by command code
        case command.code
        when 27  # Switch ON
          $game_switches[command.parameters[0]] = true
          $game_map.need_refresh = true
        when 28  # Switch OFF
          $game_switches[command.parameters[0]] = false
          $game_map.need_refresh = true
        when 29  # Change speed
          @move_speed = command.parameters[0]
        when 30  # Change freq
          @move_frequency = command.parameters[0]
        when 31  # Move animation ON
          @walk_anime = true
        when 32  # Move animation OFF
          @walk_anime = false
        when 33  # Stop animation ON
          @step_anime = true
        when 34  # Stop animation OFF
          @step_anime = false
        when 35  # Direction fix ON
          @direction_fix = true
        when 36  # Direction fix OFF
          @direction_fix = false
        when 37  # Through ON
          @through = true
        when 38  # Through OFF
          @through = false
        when 39  # Always on top ON
          @always_on_top = true
        when 40  # Always on top OFF
          @always_on_top = false
        when 41  # Change Graphic
          @tile_id = 0
          @character_name = command.parameters[0]
          @character_hue = command.parameters[1]
          if @original_direction != command.parameters[2]
            @direction = command.parameters[2]
            @original_direction = @direction
            @prelock_direction = 0
          end
          if @original_pattern != command.parameters[3]
            @pattern = command.parameters[3]
            @original_pattern = @pattern
          end
        when 42  # Change Opacity
          @opacity = command.parameters[0]
        when 43  # Change Blending
          @blend_type = command.parameters[0]
        when 44  # Play SE
          $game_system.se_play(command.parameters[0])
        when 45  # Script
          result = eval(command.parameters[0])
        end
        @move_route_index += 1
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Increase Steps
  #--------------------------------------------------------------------------
  def increase_steps
    # Clear stop count
    @stop_count = 0
  end
end
