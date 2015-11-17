#==============================================================================
# ** Interpreter (part 5)
#------------------------------------------------------------------------------
#  This interpreter runs event commands. This class is used within the
#  Game_System class and the Game_Event class.
#==============================================================================

class Interpreter
  #--------------------------------------------------------------------------
  # * Transfer Player
  #--------------------------------------------------------------------------
  def command_201
    # If in battle
    if $game_temp.in_battle
      # Continue
      return true
    end
    # If transferring player, showing message, or processing transition
    if $game_temp.player_transferring or
       $game_temp.message_window_showing or
       $game_temp.transition_processing
      # End
      return false
    end
    # Set transferring player flag
    $game_temp.player_transferring = true
    # If appointment method is [direct appointment]
    if @parameters[0] == 0
      # Set player move destination
      $game_temp.player_new_map_id = @parameters[1]
      $game_temp.player_new_x = @parameters[2]
      $game_temp.player_new_y = @parameters[3]
      $game_temp.player_new_direction = @parameters[4]
    # If appointment method is [appoint with variables]
    else
      # Set player move destination
      $game_temp.player_new_map_id = $game_variables[@parameters[1]]
      $game_temp.player_new_x = $game_variables[@parameters[2]]
      $game_temp.player_new_y = $game_variables[@parameters[3]]
      $game_temp.player_new_direction = @parameters[4]
    end
    # Advance index
    @index += 1
    # If fade is set
    if @parameters[5] == 0
      # Prepare for transition
      Graphics.freeze
      # Set transition processing flag
      $game_temp.transition_processing = true
      $game_temp.transition_name = ""
    end
    # End
    return false
  end
  #--------------------------------------------------------------------------
  # * Set Event Location
  #--------------------------------------------------------------------------
  def command_202
    # If in battle
    if $game_temp.in_battle
      # Continue
      return true
    end
    # Get character
    character = get_character(@parameters[0])
    # If no character exists
    if character == nil
      # Continue
      return true
    end
    # If appointment method is [direct appointment]
    if @parameters[1] == 0
      # Set character position
      character.moveto(@parameters[2], @parameters[3])
    # If appointment method is [appoint with variables]
    elsif @parameters[1] == 1
      # Set character position
      character.moveto($game_variables[@parameters[2]],
        $game_variables[@parameters[3]])
    # If appointment method is [exchange with another event]
    else
      old_x = character.x
      old_y = character.y
      character2 = get_character(@parameters[2])
      if character2 != nil
        character.moveto(character2.x, character2.y)
        character2.moveto(old_x, old_y)
      end
    end
    # Set character direction
    case @parameters[4]
    when 8  # up
      character.turn_up
    when 6  # right
      character.turn_right
    when 2  # down
      character.turn_down
    when 4  # left
      character.turn_left
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Scroll Map
  #--------------------------------------------------------------------------
  def command_203
    # If in battle
    if $game_temp.in_battle
      # Continue
      return true
    end
    # If already scrolling
    if $game_map.scrolling?
      # End
      return false
    end
    # Start scroll
    $game_map.start_scroll(@parameters[0], @parameters[1], @parameters[2])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Map Settings
  #--------------------------------------------------------------------------
  def command_204
    case @parameters[0]
    when 0  # panorama
      $game_map.panorama_name = @parameters[1]
      $game_map.panorama_hue = @parameters[2]
    when 1  # fog
      $game_map.fog_name = @parameters[1]
      $game_map.fog_hue = @parameters[2]
      $game_map.fog_opacity = @parameters[3]
      $game_map.fog_blend_type = @parameters[4]
      $game_map.fog_zoom = @parameters[5]
      $game_map.fog_sx = @parameters[6]
      $game_map.fog_sy = @parameters[7]
    when 2  # battleback
      $game_map.battleback_name = @parameters[1]
      $game_temp.battleback_name = @parameters[1]
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Fog Color Tone
  #--------------------------------------------------------------------------
  def command_205
    # Start color tone change
    $game_map.start_fog_tone_change(@parameters[0], @parameters[1] * 2)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Fog Opacity
  #--------------------------------------------------------------------------
  def command_206
    # Start opacity level change
    $game_map.start_fog_opacity_change(@parameters[0], @parameters[1] * 2)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Show Animation
  #--------------------------------------------------------------------------
  def command_207
    # Get character
    character = get_character(@parameters[0])
    # If no character exists
    if character == nil
      # Continue
      return true
    end
    # Set animation ID
    character.animation_id = @parameters[1]
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Transparent Flag
  #--------------------------------------------------------------------------
  def command_208
    # Change player transparent flag
    $game_player.transparent = (@parameters[0] == 0)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Set Move Route
  #--------------------------------------------------------------------------
  def command_209
    # Get character
    character = get_character(@parameters[0])
    # If no character exists
    if character == nil
      # Continue
      return true
    end
    # Force move route
    character.force_move_route(@parameters[1])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Wait for Move's Completion
  #--------------------------------------------------------------------------
  def command_210
    # If not in battle
    unless $game_temp.in_battle
      # Set move route completion waiting flag
      @move_route_waiting = true
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Prepare for Transition
  #--------------------------------------------------------------------------
  def command_221
    # If showing message window
    if $game_temp.message_window_showing
      # End
      return false
    end
    # Prepare for transition
    Graphics.freeze
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Execute Transition
  #--------------------------------------------------------------------------
  def command_222
    # If transition processing flag is already set
    if $game_temp.transition_processing
      # End
      return false
    end
    # Set transition processing flag
    $game_temp.transition_processing = true
    $game_temp.transition_name = @parameters[0]
    # Advance index
    @index += 1
    # End
    return false
  end
  #--------------------------------------------------------------------------
  # * Change Screen Color Tone
  #--------------------------------------------------------------------------
  def command_223
    # Start changing color tone
    $game_screen.start_tone_change(@parameters[0], @parameters[1] * 2)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Screen Flash
  #--------------------------------------------------------------------------
  def command_224
    # Start flash
    $game_screen.start_flash(@parameters[0], @parameters[1] * 2)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Screen Shake
  #--------------------------------------------------------------------------
  def command_225
    # Start shake
    $game_screen.start_shake(@parameters[0], @parameters[1],
      @parameters[2] * 2)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Show Picture
  #--------------------------------------------------------------------------
  def command_231
    # Get picture number
    number = @parameters[0] + ($game_temp.in_battle ? 50 : 0)
    # If appointment method is [direct appointment]
    if @parameters[3] == 0
      x = @parameters[4]
      y = @parameters[5]
    # If appointment method is [appoint with variables]
    else
      x = $game_variables[@parameters[4]]
      y = $game_variables[@parameters[5]]
    end
    # Show picture
    $game_screen.pictures[number].show(@parameters[1], @parameters[2],
      x, y, @parameters[6], @parameters[7], @parameters[8], @parameters[9])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Move Picture
  #--------------------------------------------------------------------------
  def command_232
    # Get picture number
    number = @parameters[0] + ($game_temp.in_battle ? 50 : 0)
    # If appointment method is [direct appointment]
    if @parameters[3] == 0
      x = @parameters[4]
      y = @parameters[5]
    # If appointment method is [appoint with variables]
    else
      x = $game_variables[@parameters[4]]
      y = $game_variables[@parameters[5]]
    end
    # Move picture
    $game_screen.pictures[number].move(@parameters[1] * 2, @parameters[2],
      x, y, @parameters[6], @parameters[7], @parameters[8], @parameters[9])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Rotate Picture
  #--------------------------------------------------------------------------
  def command_233
    # Get picture number
    number = @parameters[0] + ($game_temp.in_battle ? 50 : 0)
    # Set rotation speed
    $game_screen.pictures[number].rotate(@parameters[1])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Picture Color Tone
  #--------------------------------------------------------------------------
  def command_234
    # Get picture number
    number = @parameters[0] + ($game_temp.in_battle ? 50 : 0)
    # Start changing color tone
    $game_screen.pictures[number].start_tone_change(@parameters[1],
      @parameters[2] * 2)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Erase Picture
  #--------------------------------------------------------------------------
  def command_235
    # Get picture number
    number = @parameters[0] + ($game_temp.in_battle ? 50 : 0)
    # Erase picture
    $game_screen.pictures[number].erase
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Set Weather Effects
  #--------------------------------------------------------------------------
  def command_236
    # Set Weather Effects
    $game_screen.weather(@parameters[0], @parameters[1], @parameters[2])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Play BGM
  #--------------------------------------------------------------------------
  def command_241
    # Play BGM
    $game_system.bgm_play(@parameters[0])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Fade Out BGM
  #--------------------------------------------------------------------------
  def command_242
    # Fade out BGM
    $game_system.bgm_fade(@parameters[0])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Play BGS
  #--------------------------------------------------------------------------
  def command_245
    # Play BGS
    $game_system.bgs_play(@parameters[0])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Fade Out BGS
  #--------------------------------------------------------------------------
  def command_246
    # Fade out BGS
    $game_system.bgs_fade(@parameters[0])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Memorize BGM/BGS
  #--------------------------------------------------------------------------
  def command_247
    # Memorize BGM/BGS
    $game_system.bgm_memorize
    $game_system.bgs_memorize
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Restore BGM/BGS
  #--------------------------------------------------------------------------
  def command_248
    # Restore BGM/BGS
    $game_system.bgm_restore
    $game_system.bgs_restore
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Play ME
  #--------------------------------------------------------------------------
  def command_249
    # Play ME
    $game_system.me_play(@parameters[0])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Play SE
  #--------------------------------------------------------------------------
  def command_250
    # Play SE
    $game_system.se_play(@parameters[0])
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Stop SE
  #--------------------------------------------------------------------------
  def command_251
    # Stop SE
    Audio.se_stop
    # Continue
    return true
  end
end
