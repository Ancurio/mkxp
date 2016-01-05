#==============================================================================
# ** Scene_Name
#------------------------------------------------------------------------------
#  This class performs name input screen processing.
#==============================================================================

class Scene_Name
  #--------------------------------------------------------------------------
  # * Main Processing
  #--------------------------------------------------------------------------
  def main
    # Make windows
    @edit_window = Window_NameEdit.new
    @input_window = Window_NameInput.new
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
    # Prepare for transition
    Graphics.freeze
    # Dispose of windows
    @edit_window.dispose
    @input_window.dispose
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Update windows
    @edit_window.update
    @input_window.update
    # If B button was pressed
    if Input.repeat?(Input::CANCEL)
      # If cursor position is at 0
      if @edit_window.index == 0
        return
      end
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Delete text
      @edit_window.back
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # If cursor position is at [OK]
      if @input_window.character == nil
        # If name is empty
        if @edit_window.name == ""
          # Return to default name
          @edit_window.restore_default
          # If name is empty
          if @edit_window.name == ""
            # Play buzzer SE
            $game_system.se_play($data_system.buzzer_se)
            return
          end
          # Play decision SE
          $game_system.se_play($data_system.decision_se)
          return
        end
        # Change actor name
        $game_oneshot.player_name = @edit_window.name
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Switch to map screen
        $scene = Scene_Map.new
        return
      end
      # If cursor position is at maximum
      if @edit_window.index == @edit_window.max_char
        # Play buzzer SE
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      # If text character is empty
      if @input_window.character == ""
        # Play buzzer SE
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      # Play decision SE
      $game_system.se_play($data_system.decision_se)
      # Add text character
      @edit_window.add(@input_window.character)
      return
    end
  end
end
