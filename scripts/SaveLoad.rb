def save_file_name
  Oneshot::SAVE_PATH + '/Save.rxdata'
end

def save
  File.open(save_file_name, 'wb') do |file|
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
    Marshal.dump($game_map, file)
    Marshal.dump($game_player, file)
    Marshal.dump($game_oneshot, file)
  end
end

def load
  return false unless FileTest.exist?(save_file_name)
  File.open(save_file_name, 'rb') do |file|
    # Read frame count for measuring play time
    Graphics.frame_count = Marshal.load(file)
    # Read each type of game object
    $game_system        = Marshal.load(file)
    $game_switches      = Marshal.load(file)
    $game_variables     = Marshal.load(file)
    $game_self_switches = Marshal.load(file)
    $game_screen        = Marshal.load(file)
    $game_actors        = Marshal.load(file)
    $game_party         = Marshal.load(file)
    $game_map           = Marshal.load(file)
    $game_player        = Marshal.load(file)
    $game_oneshot       = Marshal.load(file)
    # If magic number is different from when saving
    # (if editing was added with editor)
    if $game_system.magic_number != $data_system.magic_number
      # Load map
      $game_map.setup($game_map.map_id)
      $game_player.center($game_player.x, $game_player.y)
    end
    # Refresh party members
    $game_party.refresh
  end
  return true
end
