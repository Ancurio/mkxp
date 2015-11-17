#==============================================================================
# ** Scene_Status
#------------------------------------------------------------------------------
#  This class performs status screen processing.
#==============================================================================

class Scene_Status
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     actor_index : actor index
  #--------------------------------------------------------------------------
  def initialize(actor_index = 0, equip_index = 0)
    @actor_index = actor_index
  end
  #--------------------------------------------------------------------------
  # * Main Processing
  #--------------------------------------------------------------------------
  def main
    # Get actor
    @actor = $game_party.actors[@actor_index]
    # Make status window
    @status_window = Window_Status.new(@actor)
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
    @status_window.dispose
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Switch to menu screen
      $scene = Scene_Menu.new(3)
      return
    end
    # If R button was pressed
    if Input.trigger?(Input::R)
      # Play cursor SE
      $game_system.se_play($data_system.cursor_se)
      # To next actor
      @actor_index += 1
      @actor_index %= $game_party.actors.size
      # Switch to different status screen
      $scene = Scene_Status.new(@actor_index)
      return
    end
    # If L button was pressed
    if Input.trigger?(Input::L)
      # Play cursor SE
      $game_system.se_play($data_system.cursor_se)
      # To previous actor
      @actor_index += $game_party.actors.size - 1
      @actor_index %= $game_party.actors.size
      # Switch to different status screen
      $scene = Scene_Status.new(@actor_index)
      return
    end
  end
end
