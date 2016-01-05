#==============================================================================
# ** Scene_Save
#------------------------------------------------------------------------------
#  This class performs save screen processing.
#==============================================================================

class Scene_Save < Scene_File
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    super("Which file would you like to save to?")
  end
  #--------------------------------------------------------------------------
  # * Decision Processing
  #--------------------------------------------------------------------------
  def on_decision(filename)
    # Play save SE
    $game_system.se_play($data_system.save_se)
    # Write save data
    file = File.open(filename, "wb")
    write_save_data(file)
    file.close
    # If called from event
    if $game_temp.save_calling
      # Clear save call flag
      $game_temp.save_calling = false
      # Switch to map screen
      $scene = Scene_Map.new
      return
    end
    # Switch to menu screen
    $scene = Scene_Menu.new(4)
  end
  #--------------------------------------------------------------------------
  # * Cancel Processing
  #--------------------------------------------------------------------------
  def on_cancel
    # Play cancel SE
    $game_system.se_play($data_system.cancel_se)
    # If called from event
    if $game_temp.save_calling
      # Clear save call flag
      $game_temp.save_calling = false
      # Switch to map screen
      $scene = Scene_Map.new
      return
    end
    # Switch to menu screen
    $scene = Scene_Menu.new(4)
  end
  #--------------------------------------------------------------------------
  # * Write Save Data
  #     file : write file object (opened)
  #--------------------------------------------------------------------------
  def write_save_data(file)
    # Make character data for drawing save file
    characters = []
    for i in 0...$game_party.actors.size
      actor = $game_party.actors[i]
      characters.push([actor.character_name, actor.character_hue])
    end
    # Write character data for drawing save file
    Marshal.dump(characters, file)
    # Wrire frame count for measuring play time
    Marshal.dump(Graphics.frame_count, file)
    # Increase save count by 1
    $game_system.save_count += 1
    # Save magic number
    # (A random value will be written each time saving with editor)
    $game_system.magic_number = $data_system.magic_number
    # Write each type of game object
    Marshal.dump($game_system, file)
    Marshal.dump($game_switches, file)
    Marshal.dump($game_variables, file)
    Marshal.dump($game_self_switches, file)
    Marshal.dump($game_screen, file)
    Marshal.dump($game_actors, file)
    Marshal.dump($game_party, file)
    Marshal.dump($game_troop, file)
    Marshal.dump($game_map, file)
    Marshal.dump($game_player, file)
    Marshal.dump($game_oneshot, file)
  end
end
