#==============================================================================
# ** Scene_End
#------------------------------------------------------------------------------
#  This class performs game end screen processing.
#==============================================================================

class Scene_End
  #--------------------------------------------------------------------------
  # * Main Processing
  #--------------------------------------------------------------------------
  def main
    # Make command window
    s1 = "To Title"
    s2 = "Shutdown"
    s3 = "Cancel"
    @command_window = Window_Command.new(192, [s1, s2, s3])
    @command_window.x = 320 - @command_window.width / 2
    @command_window.y = 240 - @command_window.height / 2
    # Execute transition
    Graphics.transition
    # Main loop
    loop do
      # Update game screen
      Graphics.update
      # Update input information
      Input.update
      # Frame Update
      update
      # Abort loop if screen is changed
      if $scene != self
        break
      end
    end
    # Prepare for transition
    Graphics.freeze
    # Dispose of window
    @command_window.dispose
    # If switching to title screen
    if $scene.is_a?(Scene_Title)
      # Fade out screen
      Graphics.transition
      Graphics.freeze
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Update command window
    @command_window.update
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Switch to menu screen
      $scene = Scene_Menu.new(5)
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Branch by command window cursor position
      case @command_window.index
      when 0  # to title
        command_to_title
      when 1  # shutdown
        command_shutdown
      when 2  # quit
        command_cancel
      end
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Process When Choosing [To Title] Command
  #--------------------------------------------------------------------------
  def command_to_title
    # Play decision SE
    $game_system.se_play($data_system.decision_se)
    # Fade out BGM, BGS, and ME
    Audio.bgm_fade(800)
    Audio.bgs_fade(800)
    Audio.me_fade(800)
    # Switch to title screen
    $scene = Scene_Title.new
  end
  #--------------------------------------------------------------------------
  # * Process When Choosing [Shutdown] Command
  #--------------------------------------------------------------------------
  def command_shutdown
    # Play decision SE
    $game_system.se_play($data_system.decision_se)
    # Fade out BGM, BGS, and ME
    Audio.bgm_fade(800)
    Audio.bgs_fade(800)
    Audio.me_fade(800)
    # Shutdown
    $scene = nil
  end
  #--------------------------------------------------------------------------
  # *  Process When Choosing [Cancel] Command
  #--------------------------------------------------------------------------
  def command_cancel
    # Play decision SE
    $game_system.se_play($data_system.decision_se)
    # Switch to menu screen
    $scene = Scene_Menu.new(5)
  end
end
