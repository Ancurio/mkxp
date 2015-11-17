#==============================================================================
# ** Scene_Debug
#------------------------------------------------------------------------------
#  This class performs debug screen processing.
#==============================================================================

class Scene_Debug
  #--------------------------------------------------------------------------
  # * Main Processing
  #--------------------------------------------------------------------------
  def main
    # Make windows
    @left_window = Window_DebugLeft.new
    @right_window = Window_DebugRight.new
    @help_window = Window_Base.new(192, 352, 448, 128)
    @help_window.contents = Bitmap.new(406, 96)
    # Restore previously selected item
    @left_window.top_row = $game_temp.debug_top_row
    @left_window.index = $game_temp.debug_index
    @right_window.mode = @left_window.mode
    @right_window.top_id = @left_window.top_id
    # Execute transition
    Graphics.transition
    # Main loop
    loop do
      # Update game screen
      Graphics.update
      # Update input information
      Input.update
      # Frame update
      update
      # Abort loop if screen is changed
      if $scene != self
        break
      end
    end
    # Refresh map
    $game_map.refresh
    # Prepare for transition
    Graphics.freeze
    # Dispose of windows
    @left_window.dispose
    @right_window.dispose
    @help_window.dispose
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Update windows
    @right_window.mode = @left_window.mode
    @right_window.top_id = @left_window.top_id
    @left_window.update
    @right_window.update
    # Memorize selected item
    $game_temp.debug_top_row = @left_window.top_row
    $game_temp.debug_index = @left_window.index
    # If left window is active: call update_left
    if @left_window.active
      update_left
      return
    end
    # If right window is active: call update_right
    if @right_window.active
      update_right
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (when left window is active)
  #--------------------------------------------------------------------------
  def update_left
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Switch to map screen
      $scene = Scene_Map.new
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Play decision SE
      $game_system.se_play($data_system.decision_se)
      # Display help
      if @left_window.mode == 0
        text1 = "C (Enter) : ON / OFF"
        @help_window.contents.draw_text(4, 0, 406, 32, text1)
      else
        text1 = "Left : -1   Right : +1"
        text2 = "L (Pageup) : -10"
        text3 = "R (Pagedown) : +10"
        @help_window.contents.draw_text(4, 0, 406, 32, text1)
        @help_window.contents.draw_text(4, 32, 406, 32, text2)
        @help_window.contents.draw_text(4, 64, 406, 32, text3)
      end
      # Activate right window
      @left_window.active = false
      @right_window.active = true
      @right_window.index = 0
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (when right window is active)
  #--------------------------------------------------------------------------
  def update_right
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Activate left window
      @left_window.active = true
      @right_window.active = false
      @right_window.index = -1
      # Erase help
      @help_window.contents.clear
      return
    end
    # Get selected switch / variable ID
    current_id = @right_window.top_id + @right_window.index
    # If switch
    if @right_window.mode == 0
      # If C button was pressed
      if Input.trigger?(Input::ACTION)
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Reverse ON / OFF
        $game_switches[current_id] = (not $game_switches[current_id])
        @right_window.refresh
        return
      end
    end
    # If variable
    if @right_window.mode == 1
      # If right button was pressed
      if Input.repeat?(Input::RIGHT)
        # Play cursor SE
        $game_system.se_play($data_system.cursor_se)
        # Increase variables by 1
        $game_variables[current_id] += 1
        # Maximum limit check
        if $game_variables[current_id] > 99999999
          $game_variables[current_id] = 99999999
        end
        @right_window.refresh
        return
      end
      # If left button was pressed
      if Input.repeat?(Input::LEFT)
        # Play cursor SE
        $game_system.se_play($data_system.cursor_se)
        # Decrease variables by 1
        $game_variables[current_id] -= 1
        # Minimum limit check
        if $game_variables[current_id] < -99999999
          $game_variables[current_id] = -99999999
        end
        @right_window.refresh
        return
      end
      # If R button was pressed
      if Input.repeat?(Input::R)
        # Play cursor SE
        $game_system.se_play($data_system.cursor_se)
        # Increase variables by 10
        $game_variables[current_id] += 10
        # Maximum limit check
        if $game_variables[current_id] > 99999999
          $game_variables[current_id] = 99999999
        end
        @right_window.refresh
        return
      end
      # If L button was pressed
      if Input.repeat?(Input::L)
        # Play cursor SE
        $game_system.se_play($data_system.cursor_se)
        # Decrease variables by 10
        $game_variables[current_id] -= 10
        # Minimum limit check
        if $game_variables[current_id] < -99999999
          $game_variables[current_id] = -99999999
        end
        @right_window.refresh
        return
      end
    end
  end
end
