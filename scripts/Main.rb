#==============================================================================
# ** Main
#------------------------------------------------------------------------------
#  After defining each class, actual processing begins here.
#==============================================================================

begin
  $DEBUG = true
  $DEMO = false

  Graphics.frame_rate = 60
  Font.default_size = 20

  # Prepare for transition
  Graphics.freeze
  # Make scene object (title screen)
  $scene = Scene_Title.new
  # Call main method as long as $scene is effective
  while $scene != nil
    $scene.main
  end
  # Fade out
  Graphics.transition(20)
rescue Errno::ENOENT
  # Supplement Errno::ENOENT exception
  # If unable to open file, display message and end
  filename = $!.message.sub("No such file or directory - ", "")
  print("Unable to find file #{filename}.")
end
